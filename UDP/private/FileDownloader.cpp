#include "FileDownloader.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"
#include "Jobs.h"

#include "FileManager.h"

#include <queue>
#include <iostream>
#include <WinSock2.h>

namespace
{
	udp::FileDownloaderJSMeta m_downloaderJSMeta;
	udp::FileDownloaderMeta m_downloaderMeta;

    jobs::JobSystem* m_js = nullptr;
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
    jobs::Job* done) :

    BaseObject(FileDownloaderMeta::GetInstance()),
    m_bucket(0),
    m_fileId(fileId),
    m_serverPort(serverPort),
    m_serverIP(serverIP),
    m_fileSize(fileSize),
    m_path(path),
    m_done(done)
{
    if (!m_js)
    {
        m_js = new jobs::JobSystem(FileDownloaderJSMeta::GetInstance(), 3 * 5);
    }
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

    struct Chunk
    {
        ull m_offset = 0;
        Packet* m_packets = nullptr;

        Chunk(ull offset, const FileDownloaderObject& downloader) :
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

        ~Chunk()
        {
            delete[] m_packets;
        }

        int RecordPacket(const Packet& pkt)
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

        bool IsComplete()
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

        void CreateDataMask(KB& outMask)
        {
            for (size_t i = 0; i < FileChunk::m_chunkSizeInKBs; ++i)
            {
                if (m_packets[i].m_packetType.GetPacketType() == EPacketType::Empty)
                {
                    outMask.UpBit(i);
                }
            }
        }

        size_t GetFull() const
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
    };

    struct ChunkManager
    {
        FileDownloaderObject& m_downloader;

        const int m_numWorkers = 2;
        const int m_maxWindow = 4;

        ull m_written = 0;
        ull m_covered = 0;
        ull m_passedToWriter = 0;

        std::list<Chunk*> m_workers;
        std::list<Chunk*> m_toWrite;

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
                return false;
            }

            if (m_workers.size() >= m_numWorkers)
            {
                return false;
            }

            if (m_workers.empty())
            {
                Chunk* c = new Chunk(m_covered, m_downloader);
                m_workers.push_back(c);
                m_covered += FileChunk::m_chunkSizeInKBs;
                return true;
            }

            ull windowStart = m_workers.front()->m_offset;
            ull winSize = (m_covered - windowStart) / FileChunk::m_chunkSizeInKBs;

            if (winSize >= m_maxWindow)
            {
                return false;
            }

            Chunk* c = new Chunk(m_covered, m_downloader);
            m_workers.push_back(c);
            m_covered += FileChunk::m_chunkSizeInKBs;
            return true;
        }

        ChunkManager(FileDownloaderObject& downloader) :
            m_downloader(downloader)
        {
            ull numKB = GetKBSize();

            for (int i = 0; i < m_numWorkers; ++i)
            {
                if (!TryCreateWorker())
                {
                    break;
                }
            }
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
                    auto readyIt = m_toWrite.begin();
                    for (; readyIt != m_toWrite.end(); ++readyIt)
                    {
                        if (curChunk->m_offset < (*readyIt)->m_offset)
                        {
                            break;
                        }
                    }

                    m_toWrite.insert(readyIt, curChunk);
                    m_workers.erase(cur);
                }
            }

            while (TryCreateWorker())
            {
            }

            if (m_workers.empty())
            {
                bool t = true;
            }
        }

        void PassToWriter(std::queue<Chunk*>& wq)
        {
            auto it = m_toWrite.begin();
            while (it != m_toWrite.end())
            {
                auto curIt = it++;
                Chunk* cur = *curIt;

                if (cur->m_offset > m_passedToWriter)
                {
                    break;
                }

                m_toWrite.erase(curIt);
                if (cur->m_offset < m_passedToWriter)
                {
                    continue;
                }

                wq.push(cur);
                m_passedToWriter += FileChunk::m_chunkSizeInKBs;
            }
        }
    };

#pragma region Allocations

    std::binary_semaphore* semaphore = new std::binary_semaphore{ 1 };
    std::mutex* writeMutex = new std::mutex();
    std::queue<Chunk*>* writeQueue = new std::queue<Chunk*>();

    std::mutex* pingMutex = new std::mutex();
    ChunkManager* chunkManager = new ChunkManager(*this);
    std::list<Packet>* masks = new std::list<Packet>();
    
    ull* counter = new ull;
    *counter = 0;
    ull* fence = new ull;
    *fence = 0;

    bool* fileWritten = new bool;
    *fileWritten = false;

    int* waiting = new int;
    *waiting = 4;

    bool* receivingPackets = new bool;
    *receivingPackets = true;

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

            jobs::RunSync(m_done);

            delete semaphore;
            delete writeMutex;
            delete writeQueue;

            delete pingMutex;
            delete chunkManager;
            delete masks;

            delete counter;
            delete fence;

            delete fileWritten;

            delete waiting;
            delete receivingPackets;

            delete this;
        }));
    };

    m_js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        struct sockaddr_in serverAddr;
        short port = static_cast<short>(m_serverPort);
        const char* local_host = m_serverIP.c_str();
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr(local_host);

        Packet pkt;
        pkt.m_packetType = PacketType::m_ping;
        pkt.m_id = m_fileId;

        while (*receivingPackets)
        {
            std::list<Packet> toSend;
            pingMutex->lock();
            
            toSend = *masks;
            masks->clear();
            pkt.m_offset = (*counter)++;

            pingMutex->unlock();

            if (toSend.empty())
            {
                toSend.push_back(pkt);
            }

            for (auto it = toSend.begin(); it != toSend.end(); ++it)
            {
                sendto(
                    m_socket,
                    reinterpret_cast<char*>(&(*it)),
                    sizeof(Packet),
                    0 /* no flags*/,
                    reinterpret_cast<sockaddr*>(&serverAddr),
                    sizeof(sockaddr_in));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        jobDone();
    }));

    m_js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
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

            m_bucket.PushPacket(pkt);
        }
        *receivingPackets = false;

        jobDone();
    }));

    m_js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {

        ull curReq = 0;

        {
            pingMutex->lock();

            for (auto it = chunkManager->m_workers.begin(); it != chunkManager->m_workers.end(); ++it)
            {
                Packet& pkt = masks->emplace_back();
                pkt.m_packetType = PacketType::m_bitmask;
                pkt.m_id = m_fileId;
                pkt.m_offset = (*it)->m_offset;
                pkt.m_payload = {};
                (*it)->CreateDataMask(pkt.m_payload);
            }

            pingMutex->unlock();
        }

        while (!chunkManager->m_workers.empty())
        {
            {
           //      Chunk* tmp = *curChunk;
           //     m_received = tmp->m_offset * sizeof(KB) + tmp->GetFull();
            }

            std::list<Packet>& packets = m_bucket.GetAccumulated();

            for (auto it = packets.begin(); it != packets.end(); ++it)
            {
                Packet& pkt = *it;
                
                switch (pkt.m_packetType.GetPacketType())
                {
                case EPacketType::Full:
                    chunkManager->RecordPacket(pkt);
                    break;
                case EPacketType::Ping:
                    *fence = max(*fence, pkt.m_offset);
                    break;
                }
            }

            chunkManager->MoveToReady();

            writeMutex->lock();
                
            chunkManager->PassToWriter(*writeQueue);
            semaphore->release();
                
            writeMutex->unlock();


            if (!chunkManager->m_workers.empty() && curReq < *fence)
            {
                pingMutex->lock();

                curReq = *counter;

                for (auto it = chunkManager->m_workers.begin(); it != chunkManager->m_workers.end(); ++it)
                {
                    Packet& pkt = masks->emplace_back();
                    pkt.m_packetType = PacketType::m_bitmask;
                    pkt.m_id = m_fileId;
                    pkt.m_offset = (*it)->m_offset;
                    pkt.m_payload = {};
                    (*it)->CreateDataMask(pkt.m_payload);
                }

                pingMutex->unlock();
            }


            packets.clear();
        }

        jobDone();
    }));

    m_js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        HANDLE f = CreateFile(
            m_path.c_str(),
            GENERIC_WRITE,
            NULL,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        ull written = 0;
        while (written < m_fileSize)
        {
            semaphore->acquire();

            std::queue<Chunk*> local;

            writeMutex->lock();

            while (!writeQueue->empty())
            {
                Chunk* toWrite = writeQueue->front();
                writeQueue->pop();
                local.push(toWrite);
            }

            writeMutex->unlock();

            while (!local.empty())
            {
                Chunk* toWrite = local.front();
                local.pop();

                for (size_t i = 0; i < FileChunk::m_chunkSizeInKBs; ++i)
                {
                    Packet& cur = toWrite->m_packets[i];

                    size_t bytes = min(sizeof(KB), m_fileSize - written);
                    DWORD tmp = 0;
                    WriteFile(
                        f,
                        &cur.m_payload,
                        bytes,
                        &tmp,
                        NULL
                    );

                    written += tmp;
                }

                delete toWrite;
            }
        }

        CloseHandle(f);

        *fileWritten = true;
        jobDone();
    }));
}
