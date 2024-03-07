#include "FileServer.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"

#include <WinSock2.h>
#include <iostream>

namespace
{
	udp::FileServerJSMeta m_serverJSMeta;
	udp::FileServerHandlersJSMeta m_serverHandlersJSMeta;
	udp::FileServerMeta m_serverMeta;

    jobs::JobSystem* m_serverJS = nullptr;
    jobs::JobSystem* m_serverHandlersJS = nullptr;
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
}

udp::FileServerObject::FileServerObject() :
	BaseObject(FileServerMeta::GetInstance())
{
    if (m_serverJS)
    {
        throw "Can't create another File Server!";
    }

    m_serverJS = new jobs::JobSystem(FileServerJSMeta::GetInstance(), 1);
    m_serverHandlersJS = new jobs::JobSystem(FileServerHandlersJSMeta::GetInstance(), 1);
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
    short port = 27015;

    // Bind the socket to any address and the specified port.
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    // OR, you can do serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // or 0.0.0.0 for tests over the net

    if (bind(m_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr))) {
        std::cout << "bind failed with error " << WSAGetLastError() << std::endl;
        return;
    }

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

            bool justCreated;
            Bucket* bucket = m_bucketManager.GetOrCreateBucket(pkt.m_id, justCreated);
            bucket->PushPacket(pkt);

            if (justCreated)
            {
                sockaddr_in* senderTmp = new sockaddr_in();
                *senderTmp = SenderAddr;
                m_serverHandlersJS->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
                    sockaddr_in sender = *senderTmp;
                    delete senderTmp;

                    while (true)
                    {
                        std::list<Packet>& packets = bucket->GetAccumulated();

                        const sockaddr* addr = reinterpret_cast<const sockaddr*>(&sender);

                        for (auto it = packets.begin(); it != packets.end(); ++it)
                        {
                            Packet& cur = *it;
                            sendto(m_socket, reinterpret_cast<const char*>(&cur), sizeof(Packet), 0, addr, sizeof(sockaddr_in));
                        }

                        packets.clear();
                    }
                }));
            }
        }
    }));
}
