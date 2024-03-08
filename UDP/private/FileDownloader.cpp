#include "FileDownloader.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"

#include "FileManager.h"

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
    const std::string& serverIP,
    unsigned int fileId,
    ull fileSize,
    const std::string& path) :

    BaseObject(FileDownloaderMeta::GetInstance()),
    m_bucket(0),
    m_fileId(fileId),
    m_serverIP(serverIP),
    m_fileSize(fileSize),
    m_path(path)
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

            ull numKB = static_cast<ull>(static_cast<double>(downloader.m_fileSize) / sizeof(KB));

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
                if (m_packets->m_packetType.GetPacketType() == EPacketType::Empty)
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
                if (m_packets->m_packetType.GetPacketType() == EPacketType::Empty)
                {
                    return outMask.UpBit(i);
                }
            }
        }
    };

    std::mutex* pingMutex = new std::mutex();
    Chunk** curChunk = new Chunk*;
    *curChunk = new Chunk(0, *this);
    Packet** mask = new Packet*;
    *mask = nullptr;

    ull* counter = new ull;
    *counter = 0;
    ull* fence = new ull;
    *fence = 0;

    m_js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        struct sockaddr_in serverAddr;
        short port = 27015;
        const char* local_host = m_serverIP.c_str();
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr(local_host);

        Packet pkt;
        pkt.m_packetType = PacketType::m_ping;
        pkt.m_id = m_fileId;

        while (true)
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
    }));

    m_js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        while (true)
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
                ull numKB = static_cast<ull>(static_cast<double>(m_fileSize) / sizeof(KB));
                ull startKB = (*curChunk)->m_offset + FileChunk::m_chunkSizeInKBs;
                delete* curChunk;
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
                
                std::cout << ((*curChunk)->IsComplete() ? "Complete" : "Incomplete") << std::endl;
            }

            packets.clear();
        }

        bool t = true;
    }));
}
