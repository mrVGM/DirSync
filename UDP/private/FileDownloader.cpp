#include "FileDownloader.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"
#include "Jobs.h"
#include "JSPool.h"

#include "FileManager.h"

#include "BaseObjectContainer.h"

#include <queue>
#include <iostream>
#include <WinSock2.h>

namespace
{
	udp::FileDownloaderJSMeta m_downloaderJSMeta;
	udp::FileDownloaderMeta m_downloaderMeta;
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
    BaseObjectContainer& container = BaseObjectContainer::GetInstance();

    BaseObject* tmp = container.GetObjectOfClass(FileDownloaderJSMeta::GetInstance());
    if (!tmp)
    {
        new jobs::JobSystem(FileDownloaderJSMeta::GetInstance(), statics::MAX_PARALLEL_DOWNLOADS);
    }

    tmp = container.GetObjectOfClass(JSPoolMeta::GetInstance());

    if (!tmp)
    {
        new JSPool(2 * statics::MAX_PARALLEL_DOWNLOADS, 3);
    }
}

udp::FileDownloaderObject::~FileDownloaderObject()
{
}

void udp::FileDownloaderObject::Init()
{
    class ChunkManagerContext
    {
    private:
        ull m_windowStart = 0;
        std::mutex m_mutex;
        std::list<ull> m_activeChunks;

    public:
        ull GetWindowStart()
        {
            m_mutex.lock();
            ull res = m_windowStart;
            m_mutex.unlock();
            return res;
        }

        void StartChunk(ull offset)
        {
            m_mutex.lock();

            auto it = m_activeChunks.begin();
            for (; it != m_activeChunks.end(); ++it)
            {
                if (offset < *it)
                {
                    break;
                }
            }
            m_activeChunks.insert(it, offset);

            if (!m_activeChunks.empty())
            {
                m_windowStart = m_activeChunks.front();
            }

            m_mutex.unlock();
        }

        void ChunkFinished(ull offset)
        {
            m_mutex.lock();

            for (auto it = m_activeChunks.begin(); it != m_activeChunks.end(); ++it)
            {
                if (offset == *it)
                {
                    m_activeChunks.erase(it);
                    break;
                }
            }

            if (!m_activeChunks.empty())
            {
                m_windowStart = m_activeChunks.front();
            }

            m_mutex.unlock();
        }
    };

    ChunkManagerContext* cmCTX = new ChunkManagerContext();

    struct ChunkManager
    {
        ChunkManagerContext& m_cmCTX;
        int m_reminder;

        Bucket m_bucket;

        FileDownloaderObject& m_downloader;
        std::function<void()> m_done;

        std::mutex pingMutex = std::mutex();
        std::list<Packet> masks = std::list<Packet>();
        ull counter = 0;
        ull fence = 0;
        bool finished = false;

        int waiting = 2;

        ull m_covered = 0;
        bool m_fullyCovered = false;

        std::list<Chunk*> m_workers;

        ull GetKBSize() const
        {
            ull res = static_cast<ull>(ceil(static_cast<double>(m_downloader.m_fileSize) / sizeof(KB)));
            return res;
        }

        bool TryCreateWorker()
        {
            ull numKBs = GetKBSize();
            if (m_covered >= numKBs)
            {
                m_fullyCovered = true;
                return false;
            }

            if (m_workers.size() >= m_downloader.m_numWorkers)
            {
                return false;
            }

            if (m_workers.empty())
            {
                Chunk* c = new Chunk(m_covered, m_downloader);
                m_workers.push_back(c);
                m_cmCTX.StartChunk(m_covered);
                m_covered += 2 * FileChunk::m_chunkSizeInKBs;
                return true;
            }

            ull windowStart = m_cmCTX.GetWindowStart();
            ull winSize = (m_covered - windowStart) / FileChunk::m_chunkSizeInKBs;

            if (winSize >= m_downloader.m_downloadWindow)
            {
                return false;
            }

            Chunk* c = new Chunk(m_covered, m_downloader);
            m_workers.push_back(c);
            m_cmCTX.StartChunk(m_covered);
            m_covered += 2 * FileChunk::m_chunkSizeInKBs;
            return true;
        }

        ChunkManager(
            ChunkManagerContext& cmCTX,
            int reminder,
            FileDownloaderObject& downloader,
            const std::function<void()>& done) :
            m_cmCTX(cmCTX),
            m_reminder(reminder),
            m_bucket(0),
            m_downloader(downloader),
            m_done(done)
        {
            m_covered = m_reminder * FileChunk::m_chunkSizeInKBs;
            ull numKB = GetKBSize();

            for (int i = 0; i < m_downloader.m_numWorkers; ++i)
            {
                if (!TryCreateWorker())
                {
                    break;
                }
            }
        }

        ull GetPacketID() const
        {
            return 2 * m_downloader.m_fileId + m_reminder;
        }

        void RecordPacket(const Packet& pkt)
        {
            for (auto it = m_workers.begin(); it != m_workers.end(); ++it)
            {
                Chunk* c = *it;
                int res = c->RecordPacket(pkt);

                if (res >= 0)
                {
                    m_downloader.m_received += sizeof(KB);
                    break;
                }
            }
        }

        void MoveToReady()
        {
            auto it = m_workers.begin();
            auto end = m_workers.end();

            while (it != end)
            {
                auto cur = it++;
                Chunk* curChunk = *cur;

                if (curChunk->IsComplete())
                {
                    m_downloader.m_writer.PushChunk(curChunk);
                    m_workers.erase(cur);
                    m_cmCTX.ChunkFinished(curChunk->m_offset);
                }
            }

            while (TryCreateWorker())
            {
            }
        }

        void Start(JSPool* pool)
        {
            SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sock == INVALID_SOCKET) {
                std::cout << "socket failed with error " << sock << std::endl;
                return;
            }

            jobs::JobSystem* js = pool->AcquireJS();

            auto itemDone = [=]() {
                --waiting;
                if (waiting > 0)
                {
                    return;
                }

                pool->ReleaseJS(js);

                m_done();
            };

            js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
                struct sockaddr_in serverAddr;
                short port = static_cast<short>(m_downloader.m_serverPort);
                const char* local_host = m_downloader.m_serverIP.c_str();
                serverAddr.sin_family = AF_INET;
                serverAddr.sin_port = htons(port);
                serverAddr.sin_addr.s_addr = inet_addr(local_host);

                Packet pkt;
                pkt.m_packetType = PacketType::m_ping;
                pkt.m_id = GetPacketID();

                while (!finished)
                {
                    std::list<Packet> toSend;
                    pingMutex.lock();

                    toSend = masks;
                    masks.clear();
                    pkt.m_offset = counter++;

                    pingMutex.unlock();

                    if (toSend.empty())
                    {
                        toSend.push_back(pkt);
                    }

                    for (auto it = toSend.begin(); it != toSend.end(); ++it)
                    {
                        sendto(
                            sock,
                            reinterpret_cast<char*>(&(*it)),
                            sizeof(Packet),
                            0 /* no flags*/,
                            reinterpret_cast<sockaddr*>(&serverAddr),
                            sizeof(sockaddr_in));
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(m_downloader.m_pingDelay));
                }

                jobs::RunSync(jobs::Job::CreateFromLambda(itemDone));
            }));


            js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {

                ull curReq = 0;

                {
                    pingMutex.lock();

                    for (auto it = m_workers.begin(); it != m_workers.end(); ++it)
                    {
                        Packet& pkt = masks.emplace_back();
                        pkt.m_packetType = PacketType::m_bitmask;
                        pkt.m_id = GetPacketID();
                        pkt.m_offset = (*it)->m_offset;
                        pkt.m_payload = {};
                        (*it)->CreateDataMask(pkt.m_payload);
                    }

                    pingMutex.unlock();
                }

                while (!m_fullyCovered || !m_workers.empty())
                {
                    std::list<Packet>& packets = m_bucket.GetAccumulated();

                    for (auto it = packets.begin(); it != packets.end(); ++it)
                    {
                        Packet& pkt = *it;

                        switch (pkt.m_packetType.GetPacketType())
                        {
                        case EPacketType::Full:
                            RecordPacket(pkt);
                            break;
                        case EPacketType::Ping:
                            fence = max(fence, pkt.m_offset);
                            break;
                        }
                    }

                    MoveToReady();

                    if (!m_workers.empty() && curReq < fence)
                    {
                        pingMutex.lock();

                        curReq = counter++;

                        for (auto it = m_workers.begin(); it != m_workers.end(); ++it)
                        {
                            Packet& pkt = masks.emplace_back();
                            pkt.m_packetType = PacketType::m_bitmask;
                            pkt.m_id = GetPacketID();
                            pkt.m_offset = (*it)->m_offset;
                            pkt.m_payload = {};
                            (*it)->CreateDataMask(pkt.m_payload);
                        }

                        pingMutex.unlock();
                    }

                    packets.clear();
                }

                finished = true;
                closesocket(sock);
            }));


            js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
                while (!finished)
                {
                    Packet pkt;
                    sockaddr_in from;
                    int size = sizeof(from);
                    int bytes_received = recvfrom(sock, reinterpret_cast<char*>(&pkt), sizeof(Packet), 0, reinterpret_cast<sockaddr*>(&from), &size);

                    if (bytes_received == SOCKET_ERROR) {
                        std::cout << "recvfrom failed with error " << WSAGetLastError() << std::endl;
                        continue;
                    }

                    m_bucket.PushPacket(pkt);
                }

                jobs::RunSync(jobs::Job::CreateFromLambda(itemDone));
            }));
        }
    };

#pragma region Allocations

    ChunkManager* chunkManager = nullptr;
    ChunkManager* chunkManager1 = nullptr;

    int* waiting = new int;
    *waiting = 3;

#pragma endregion

    auto itemDone = [=]() {
        --(*waiting);
        if (*waiting > 0)
        {
            return;
        }

        jobs::RunSync(m_done);

        delete waiting;
        delete cmCTX;
        delete chunkManager;
        delete chunkManager1;

        delete this;
    };

    chunkManager = new ChunkManager(*cmCTX , 0, *this, [=]() {
        jobs::RunSync(jobs::Job::CreateFromLambda(itemDone));
    });

    chunkManager1 = new ChunkManager(*cmCTX, 1, *this, [=]() {
        jobs::RunSync(jobs::Job::CreateFromLambda(itemDone));
    });

    jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
        BaseObjectContainer& container = BaseObjectContainer::GetInstance();
        BaseObject* tmp = container.GetObjectOfClass(JSPoolMeta::GetInstance());

        JSPool* pool = static_cast<JSPool*>(tmp);

        chunkManager->Start(pool);
        chunkManager1->Start(pool);

        tmp = container.GetObjectOfClass(FileDownloaderJSMeta::GetInstance());
        jobs::JobSystem* js = static_cast<jobs::JobSystem*>(tmp);

        js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
            m_writer.Start();
            jobs::RunSync(jobs::Job::CreateFromLambda(itemDone));
        }));
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
