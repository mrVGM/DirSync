#include "FileDownloader.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"
#include "Jobs.h"

#include "ChunkDownloader.h"

#include "FileManager.h"

#include "JSPool.h"

#include <queue>
#include <iostream>
#include <WinSock2.h>

namespace
{
	udp::FileDownloaderJSMeta m_downloaderJSMeta;
	udp::FileDownloaderMeta m_downloaderMeta;

    udp::JSPool* m_jsPool = nullptr;
    udp::JSPool* m_chunkJSPool = nullptr;
}

const udp::FileDownloaderJSMeta& udp::FileDownloaderJSMeta::GetInstance()
{
	return m_downloaderJSMeta;
}

udp::FileDownloaderJSMeta::FileDownloaderJSMeta() :
	BaseObjectMeta(&jobs::JobSystemMeta::GetInstance())
{
}

const udp::FileDownloaderMeta& udp::FileDownloaderMeta::GetInstance()
{
	return m_downloaderMeta;
}

udp::FileDownloaderMeta::FileDownloaderMeta() :
	BaseObjectMeta(nullptr)
{
}

udp::FileDownloaderObject::FileDownloaderObject(
    int serverPort,
    const std::string& serverIP,
    unsigned int fileId,
    ull fileSize,
    const std::string& path,
    int numWorkers,
    int downloadWindow,
    int pingDelay,
    jobs::Job* done) :

    BaseObject(FileDownloaderMeta::GetInstance()),
    m_fileId(fileId),
    m_serverPort(serverPort),
    m_serverIP(serverIP),
    m_fileSize(fileSize),
    m_path(path),
    m_numWorkers(numWorkers),
    m_downloadWindow(downloadWindow),
    m_pingDelay(pingDelay),
    m_writer(*this),
    m_done(done)
{
    static JSPool pool(4, 4);
    m_jsPool = &pool;

    static JSPool chunkPool(20, 2);
    m_chunkJSPool = &chunkPool;

    m_numWorkers = 5;
}

udp::FileDownloaderObject::~FileDownloaderObject()
{
}

void udp::FileDownloaderObject::Init()
{
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) {
        std::cout << "socket failed with error " << m_socket << std::endl;
        return;
    }

    jobs::JobSystem* js = m_jsPool->GetJS();

    struct ChunkManager : public ChunkDownloader::Tracker
    {
        FileDownloaderObject& m_downloader;

        std::mutex m_mutex;

        ull m_covered = 0;
        ull windowStart = 0;

        std::set<ChunkDownloader*> m_active;
        std::set<ChunkDownloader*> m_inactive;

        ull GetKBSize() const
        {
            ull res = static_cast<ull>(ceil(static_cast<double>(m_downloader.m_fileSize) / sizeof(KB)));
            return res;
        }

        bool TryCreateWorker()
        {
            m_mutex.lock();

            bool res = TryCreateWorkerInternal();

            m_mutex.unlock();

            return res;
        }
        bool TryCreateWorkerInternal()
        {
            ull numKBs = GetKBSize();
            if (m_covered >= numKBs)
            {
                return false;
            }

            if (m_inactive.empty())
            {
                return false;
            }

            ull minActive = 0;
            for (auto it = m_active.begin(); it != m_active.end(); ++it)
            {
                ChunkDownloader* cur = *it;
                if (minActive == 0)
                {
                    minActive = cur->GetOffset();
                    continue;
                }

                minActive = min(minActive, cur->GetOffset());
            }
            windowStart = max(windowStart, minActive);

            ull winSize = (m_covered - windowStart) / FileChunk::m_chunkSizeInKBs;

            if (winSize >= m_downloader.m_downloadWindow)
            {
                return false;
            }


            auto it = m_inactive.begin();
            ChunkDownloader* downloader = *it;
            m_inactive.erase(it);

            m_active.insert(downloader);
            jobs::JobSystem* js = m_chunkJSPool->GetJS();
            downloader->DownloadChunk(m_covered, *this, js);
            m_covered += FileChunk::m_chunkSizeInKBs;

            return true;
        }

        void Finished(ChunkDownloader* downloader, Chunk* chunk, jobs::JobSystem* js) override
        {
            m_chunkJSPool->ReleaseJS(*js);

            m_mutex.lock();

            m_active.erase(downloader);
            m_inactive.insert(downloader);

            m_mutex.unlock();

            while (TryCreateWorker())
            {
            }

            m_downloader.m_writer.PushChunk(chunk);
        }

        void IncrementProgress() override
        {
            m_downloader.m_received += sizeof(KB);
        }

        ChunkManager(FileDownloaderObject& downloader) :
            m_downloader(downloader)
        {
            ull numKB = GetKBSize();

            m_mutex.lock();
            for (int i = 0; i < m_downloader.m_numWorkers; ++i)
            {
                m_inactive.insert(new ChunkDownloader(m_downloader));
            }
            m_mutex.unlock();

            while (TryCreateWorker())
            {
            }
        }

        ~ChunkManager()
        {
            m_mutex.lock();
            for (auto it = m_active.begin(); it != m_active.end(); ++it)
            {
                delete (*it);
            }

            for (auto it = m_inactive.begin(); it != m_inactive.end(); ++it)
            {
                delete (*it);
            }
            m_mutex.unlock();
        }
    };

#pragma region Allocations

    ChunkManager* chunkManager = new ChunkManager(*this);
    
    bool* fileWritten = new bool;
    *fileWritten = false;

    int* waiting = new int;
    *waiting = 2;

#pragma endregion

    auto isFileWritten = [=]() {
        return *fileWritten;
    };

    auto jobDone = [=]() {
        jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
            int& cnt = *waiting;
            --cnt;

            if (cnt > 0)
            {
                return;
            }

            m_jsPool->ReleaseJS(*js);
            jobs::RunSync(m_done);

            delete chunkManager;
            delete fileWritten;
            delete waiting;

            delete this;
        }));
    };

    js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        while (!isFileWritten())
        {
            Packet pkt;
            sockaddr_in from;
            int size = sizeof(from);
            int bytes_received = recvfrom(m_socket, reinterpret_cast<char*>(&pkt), sizeof(Packet), 0, reinterpret_cast<sockaddr*>(&from), &size);

            if (bytes_received == SOCKET_ERROR) {
                std::cout << "recvfrom failed with error " << WSAGetLastError() << std::endl;
                continue;
            }

            ull bucketId = pkt.m_chunkId;
            m_buckets.PushPacket(bucketId, pkt);
        }

        jobDone();
    }));

    js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        m_writer.Start();

        *fileWritten = true;

        closesocket(m_socket);
        jobDone();
    }));
}


udp::Chunk::Chunk(ull offset, const FileDownloaderObject& downloader) :
    m_offset(offset)
{
    m_packets = new Packet[FileChunk::m_chunkSizeInKBs];

    ull numKB = static_cast<ull>(ceil(static_cast<double>(downloader.m_fileSize) / sizeof(KB)));

    ull startKB = m_offset;
    ull endKB = min(startKB + FileChunk::m_chunkSizeInKBs, numKB);

    for (size_t i = 0; i < FileChunk::m_chunkSizeInKBs; ++i)
    {
        size_t index = startKB + i;
        if (index < endKB)
        {
            m_packets[i].m_packetType = PacketType::m_empty;
        }
        else
        {
            m_packets[i].m_packetType = PacketType::m_blank;
        }
    }
}

udp::Chunk::~Chunk()
{
    delete[] m_packets;
}

int udp::Chunk::RecordPacket(const Packet& pkt)
{
    if (pkt.m_offset < m_offset)
    {
        return -1;
    }

    if (pkt.m_offset >= m_offset + FileChunk::m_chunkSizeInKBs)
    {
        return -1;
    }

    if (m_packets[pkt.m_offset - m_offset].m_packetType.GetPacketType() == EPacketType::Empty)
    {
        m_packets[pkt.m_offset - m_offset] = pkt;
        return 1;
    }

    return 0;
}

bool udp::Chunk::IsComplete()
{
    for (size_t i = 0; i < FileChunk::m_chunkSizeInKBs; ++i)
    {
        if (m_packets[i].m_packetType.GetPacketType() == EPacketType::Empty)
        {
            return false;
        }
    }

    return true;
}

void udp::Chunk::CreateDataMask(KB& outMask)
{
    for (size_t i = 0; i < FileChunk::m_chunkSizeInKBs; ++i)
    {
        if (m_packets[i].m_packetType.GetPacketType() == EPacketType::Empty)
        {
            outMask.UpBit(i);
        }
    }
}

size_t udp::Chunk::GetFull() const
{
    size_t res = 0;
    for (size_t i = 0; i < FileChunk::m_chunkSizeInKBs; ++i)
    {
        if (m_packets[i].m_packetType.GetPacketType() == EPacketType::Full)
        {
            ++res;
        }
    }

    return res;
}


udp::FileWriter::FileWriter(FileDownloaderObject& downloader) :
    m_downloader(downloader)
{
    m_curBuff = &m_buff1;
}

void udp::FileWriter::PushChunk(Chunk* chunk)
{
    m_mutex.lock();

    m_curBuff->push_back(chunk);

    m_getChunksSemaphore.release();

    m_mutex.unlock();
}

std::list<udp::Chunk*>& udp::FileWriter::GetReceived()
{
    m_getChunksSemaphore.acquire();

    m_mutex.lock();

    std::list<Chunk*>& res = *m_curBuff;
    m_curBuff = m_curBuff == &m_buff1 ? &m_buff2 : &m_buff1;
    m_curBuff->clear();

    m_mutex.unlock();

    return res;
}

void udp::FileWriter::Start()
{
    HANDLE f = CreateFile(
        m_downloader.m_path.c_str(),
        GENERIC_WRITE,
        NULL,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    ull written = 0;
    ull KBWritten = 0;

    std::list<Chunk*> toWrite;

    while (written < m_downloader.m_fileSize)
    {
        std::list<Chunk*>& chunks = GetReceived();

        for (auto it = chunks.begin(); it != chunks.end(); ++it)
        {
            Chunk* cur = *it;
            if (cur->m_offset < KBWritten)
            {
                delete cur;
                continue;
            }

            auto wIt = toWrite.end();
            for (wIt = toWrite.begin(); wIt != toWrite.end(); ++wIt)
            {
                if (cur->m_offset <= (*wIt)->m_offset)
                {
                    break;
                }
            }

            if (wIt != toWrite.end() && cur->m_offset == (*wIt)->m_offset)
            {
                delete cur;
                continue;
            }

            toWrite.insert(wIt, cur);
        }

        while (!toWrite.empty())
        {
            Chunk* cur = toWrite.front();
            if (cur->m_offset != KBWritten)
            {
                break;
            }

            toWrite.pop_front();

            for (int i = 0; i < FileChunk::m_chunkSizeInKBs; ++i)
            {
                Packet& pak = cur->m_packets[i];
                if (pak.m_packetType.GetPacketType() != EPacketType::Full)
                {
                    continue;
                }

                ull size = min(m_downloader.m_fileSize - written, sizeof(pak.m_payload));

                DWORD tmpWritten;
                WriteFile(
                    f,
                    &pak.m_payload,
                    size,
                    &tmpWritten,
                    NULL);
                
                written += tmpWritten;
                ++KBWritten;
            }

            delete cur;
        }

    }

    CloseHandle(f);
}

void udp::FileDownloaderObject::Send(const Packet& packet)
{
    struct sockaddr_in serverAddr;
    short port = static_cast<short>(m_serverPort);
    const char* local_host = m_serverIP.c_str();
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(local_host);

    sendto(
        m_socket,
        reinterpret_cast<const char*>(&packet),
        sizeof(Packet),
        0 /* no flags*/,
        reinterpret_cast<sockaddr*>(&serverAddr),
        sizeof(sockaddr_in));
}
