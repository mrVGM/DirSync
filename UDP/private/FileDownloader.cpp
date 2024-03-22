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

        void RecordPacket(const Packet& pkt)
        {
            if (pkt.m_offset < m_offset)
            {
                return;
            }

            if (pkt.m_offset >= m_offset + FileChunk::m_chunkSizeInKBs)
            {
                return;
            }

            m_packets[pkt.m_offset - m_offset] = pkt;
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

#pragma region Allocations

    std::binary_semaphore* semaphore = new std::binary_semaphore{ 1 };
    std::mutex* writeMutex = new std::mutex();
    std::queue<Chunk*>* writeQueue = new std::queue<Chunk*>();

    std::mutex* pingMutex = new std::mutex();
    Chunk** curChunk = new Chunk*;
    *curChunk = new Chunk(0, *this);
    Packet** mask = new Packet*;
    *mask = nullptr;

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
            delete curChunk;
            delete mask;

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
            pingMutex->lock();
            
            Packet* maskPack = *mask;
            *mask = nullptr;
            pkt.m_offset = (*counter)++;

            pingMutex->unlock();

            Packet* toSend = &pkt;
            if (maskPack)
            {
                toSend = maskPack;
            }

            sendto(
                m_socket,
                reinterpret_cast<char*>(toSend),
                sizeof(Packet),
                0 /* no flags*/,
                reinterpret_cast<sockaddr*>(&serverAddr),
                sizeof(sockaddr_in));

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
        Packet pkt;
        pkt.m_packetType = PacketType::m_bitmask;
        pkt.m_id = m_fileId;
        pkt.m_offset = (*curChunk)->m_offset;

        ull curReq = 0;

        {
            pingMutex->lock();

            pkt.m_payload = {};
            (*curChunk)->CreateDataMask(pkt.m_payload);
            *mask = &pkt;

            pingMutex->unlock();
        }

        while (*curChunk)
        {
            {
                Chunk* tmp = *curChunk;
                m_received = tmp->m_offset * sizeof(KB) + tmp->GetFull();
            }

            std::list<Packet>& packets = m_bucket.GetAccumulated();

            for (auto it = packets.begin(); it != packets.end(); ++it)
            {
                Packet& pkt = *it;
                
                switch (pkt.m_packetType.GetPacketType())
                {
                case EPacketType::Full:
                    (*curChunk)->RecordPacket(pkt);
                    break;
                case EPacketType::Ping:
                    *fence = max(*fence, pkt.m_offset);
                    break;
                }
            }

            bool isCurComplete = (*curChunk)->IsComplete();
            if (isCurComplete)
            {
                ull numKB = static_cast<ull>(ceil(static_cast<double>(m_fileSize) / sizeof(KB)));
                ull startKB = (*curChunk)->m_offset + FileChunk::m_chunkSizeInKBs;

                writeMutex->lock();
                
                writeQueue->push(*curChunk);
                semaphore->release();
                
                writeMutex->unlock();

                *curChunk = nullptr;

                if (startKB < numKB) {
                    *curChunk = new Chunk(startKB, *this);
                    pkt.m_offset = startKB;
                }
            }

            if (*curChunk && curReq < *fence)
            {
                pingMutex->lock();

                curReq = *fence;
                pkt.m_payload = {};
                (*curChunk)->CreateDataMask(pkt.m_payload);
                *mask = &pkt;

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
