#include "FileServer.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"

#include "FileManager.h"

#include <WinSock2.h>
#include <iostream>

namespace
{
	udp::FileServerJSMeta m_serverJSMeta;
	udp::FileServerHandlersJSMeta m_serverHandlersJSMeta;
	udp::FileServerMeta m_serverMeta;

    jobs::JobSystem* m_serverJS = nullptr;
    jobs::JobSystem* m_serverHandlersJS = nullptr;

    udp::FileManagerObject* m_fileManger = nullptr;
}

const udp::FileServerJSMeta& udp::FileServerJSMeta::GetInstance()
{
	return m_serverJSMeta;
}

udp::FileServerJSMeta::FileServerJSMeta() :
	BaseObjectMeta(&jobs::JobSystemMeta::GetInstance())
{
}

udp::FileServerHandlersJSMeta::FileServerHandlersJSMeta() :
    BaseObjectMeta(&jobs::JobSystemMeta::GetInstance())
{
}

const udp::FileServerHandlersJSMeta& udp::FileServerHandlersJSMeta::GetInstance()
{
    return m_serverHandlersJSMeta;
}


const udp::FileServerMeta& udp::FileServerMeta::GetInstance()
{
	return m_serverMeta;
}

udp::FileServerMeta::FileServerMeta() :
	BaseObjectMeta(nullptr)
{
    if (!m_fileManger)
    {
        m_fileManger = new FileManagerObject();
    }
}

void udp::FileServerObject::StartBucket(ull bucketID)
{
    m_checkWorkingBuckets.lock();

    m_workingBuckets.insert(bucketID);

    m_checkWorkingBuckets.unlock();
}

void udp::FileServerObject::StopBucket(ull bucketID)
{
    m_checkWorkingBuckets.lock();

    m_workingBuckets.erase(bucketID);

    m_checkWorkingBuckets.unlock();

    bool tmp;
    Bucket* bucket = m_bucketManager.GetOrCreateBucket(bucketID, tmp);
    {
        Packet packet;
        packet.m_packetType = PacketType::m_ping;
        bucket->PushPacket(packet);
    }
}

int udp::FileServerObject::GetPort() const
{
    return m_port;
}

bool udp::FileServerObject::CheckBucket(ull bucketID)
{
    m_checkWorkingBuckets.lock();

    bool res = m_workingBuckets.contains(bucketID);

    m_checkWorkingBuckets.unlock();

    return res;
}

udp::FileServerObject::FileServerObject() :
	BaseObject(FileServerMeta::GetInstance())
{
    if (m_serverJS)
    {
        throw "Can't create another File Server!";
    }

    m_serverJS = new jobs::JobSystem(FileServerJSMeta::GetInstance(), 1);
    m_serverHandlersJS = new jobs::JobSystem(FileServerHandlersJSMeta::GetInstance(), 5);
}

udp::FileServerObject::~FileServerObject()
{
}

void udp::FileServerObject::Init()
{
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) {
        std::cout << "socket failed with error " << m_socket << std::endl;
        return;
    }

    struct sockaddr_in serverAddr;
    int serverAddrSize = sizeof(sockaddr_in);
    
    // Bind the socket to any address and the specified port.
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = 0;
    // OR, you can do serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(m_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr))) {
        std::cout << "bind failed with error " << WSAGetLastError() << std::endl;
        return;
    }

    getsockname(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), &serverAddrSize);

    m_port = static_cast<unsigned short>(htons(serverAddr.sin_port));

    m_serverJS->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        while (true)
        {
            Packet pkt;
            struct sockaddr_in SenderAddr;
            int senderAddrSize = sizeof(SenderAddr);

            int bytes_received = recvfrom(m_socket, reinterpret_cast<char*>(&pkt), sizeof(pkt), 0 /* no flags*/, (SOCKADDR*)&SenderAddr, &senderAddrSize);
            if (bytes_received == SOCKET_ERROR) {
                std::cout << "recvfrom failed with error" << WSAGetLastError();
                continue;
            }

            ull bucketId = pkt.m_id;
            bool justCreated;
            Bucket* bucket = m_bucketManager.GetOrCreateBucket(bucketId, justCreated);
            bucket->PushPacket(pkt);

            FileEntry* file = m_fileManger->GetFile(bucketId);

            if (justCreated)
            {
                StartBucket(bucketId);

                sockaddr_in* senderTmp = new sockaddr_in();
                *senderTmp = SenderAddr;
                m_serverHandlersJS->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
                    sockaddr_in sender = *senderTmp;
                    delete senderTmp;

                    while (CheckBucket(bucketId))
                    {
                        std::list<Packet>& packets = bucket->GetAccumulated();

                        const sockaddr* addr = reinterpret_cast<const sockaddr*>(&sender);

                        for (auto it = packets.begin(); it != packets.end(); ++it)
                        {
                            Packet& cur = *it;
                            switch (cur.m_packetType.GetPacketType())
                            {
                            case EPacketType::Ping:
                                sendto(m_socket, reinterpret_cast<const char*>(&cur), sizeof(Packet), 0, addr, sizeof(sockaddr_in));
                                break;

                            case EPacketType::Bitmask:
                                {
                                    for (size_t i = 0; i < 8 * sizeof(KB); ++i)
                                    {
                                        Packet toSend = cur;
                                        toSend.m_offset = cur.m_offset + i;
                                        toSend.m_packetType = PacketType::m_full;

                                        if (cur.m_payload.GetBitState(i))
                                        {
                                            file->GetKB(toSend.m_payload, cur.m_offset + i);
                                            sendto(m_socket, reinterpret_cast<const char*>(&toSend), sizeof(Packet), 0, addr, sizeof(sockaddr_in));
                                        }
                                    }
                                }
                                break;
                            }
                        }

                        packets.clear();
                    }

                    m_bucketManager.DestroyBucket(bucketId);
                    file->UnloadData();
                }));
            }
        }
    }));
}
