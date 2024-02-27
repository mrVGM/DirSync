#include "UDPClient.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"
#include "Jobs.h"

#include "BaseObjectContainer.h"

#include <WinSock2.h>
#include <iostream>

namespace
{
    struct ClientSocket
    {
        SOCKET m_socket;
        sockaddr_in ClientAddr;

        ~ClientSocket()
        {
            if (m_socket != INVALID_SOCKET)
            {
                closesocket(m_socket);
            }
            m_socket = INVALID_SOCKET;
        }
    };

    udp::UDPClientMeta m_instance;
    udp::UDPClientJSMeta m_udpClientJSMeta;
    udp::FileDownloadersJSMeta m_downloadersJSMeta;
    udp::PingServerJSMeta m_pingServerJSMeta;

    jobs::JobSystem* m_udpClientJS = nullptr;


    void InitJS(int downloaderThreads)
    {
        using namespace udp;

        BaseObjectContainer& container = BaseObjectContainer::GetInstance();

        {
            BaseObject* js = container.GetObjectOfClass(UDPClientJSMeta::GetInstance());
            if (!js)
            {
                js = new jobs::JobSystem(UDPClientJSMeta::GetInstance(), 1);
            }

            m_udpClientJS = static_cast<jobs::JobSystem*>(js);
        }

        {
            BaseObject* js = container.GetObjectOfClass(FileDownloadersJSMeta::GetInstance());
            if (!js)
            {
                new jobs::JobSystem(FileDownloadersJSMeta::GetInstance(), downloaderThreads);
            }
        }

        {
            BaseObject* js = container.GetObjectOfClass(PingServerJSMeta::GetInstance());
            if (!js)
            {
                new jobs::JobSystem(PingServerJSMeta::GetInstance(), downloaderThreads);
            }
        }
    }
}


udp::UDPClientMeta::UDPClientMeta() :
    BaseObjectMeta(nullptr)
{
}

const udp::UDPClientMeta& udp::UDPClientMeta::GetInstance()
{
    return m_instance;
}






udp::UDPClientJSMeta::UDPClientJSMeta() :
    BaseObjectMeta(&jobs::JobSystemMeta::GetInstance())
{
}

const udp::UDPClientJSMeta& udp::UDPClientJSMeta::GetInstance()
{
    return m_udpClientJSMeta;
}

udp::FileDownloadersJSMeta::FileDownloadersJSMeta() :
    BaseObjectMeta(&jobs::JobSystemMeta::GetInstance())
{
}

const udp::FileDownloadersJSMeta& udp::FileDownloadersJSMeta::GetInstance()
{
    return m_downloadersJSMeta;
}

udp::PingServerJSMeta::PingServerJSMeta() :
    BaseObjectMeta(&jobs::JobSystemMeta::GetInstance())
{
}

const udp::PingServerJSMeta& udp::PingServerJSMeta::GetInstance()
{
    return m_pingServerJSMeta;
}


udp::UDPClientObject::UDPClientObject() :
    BaseObject(UDPClientMeta::GetInstance())
{
    m_clientSock = new ClientSocket();

    ClientSocket* clientSock = static_cast<ClientSocket*>(m_clientSock);

    SOCKET& SendSocket = clientSock->m_socket;
    sockaddr_in& ClientAddr = clientSock->ClientAddr;

    SendSocket = INVALID_SOCKET;
    SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (SendSocket == INVALID_SOCKET) {
        std::cout << "socket failed with error " << SendSocket << std::endl;
        return;
    }

    InitJS(5);

    m_udpClientJS->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        UDPRes res;

        struct sockaddr_in SenderAddr;
        int SenderAddrSize = sizeof(SenderAddr);

        while (true)
        {
            int bytes_received = recvfrom(SendSocket, reinterpret_cast<char*>(&res), sizeof(res), 0 /* no flags*/, (SOCKADDR*)&SenderAddr, &SenderAddrSize);

            if (bytes_received == SOCKET_ERROR)
            {
                std::cout << "recvfrom failed with error" << WSAGetLastError() << std::endl;
                continue;
            }

            UDPResponseBucket* bucket = GetBucket(res.m_fileId);
            if (!bucket)
            {
                continue;
            }
            bucket->PushUDPRes(res);
        }
    }));
}

udp::UDPClientObject::~UDPClientObject()
{
    if (m_clientSock)
    {
        delete m_clientSock;
    }

    for (auto it = m_buckets.begin(); it != m_buckets.end(); ++it)
    {
        delete it->second;
    }
}

udp::UDPResponseBucket* udp::UDPClientObject::UDPClientObject::GetBucket(int id)
{
    udp::UDPResponseBucket* res = nullptr;

    m_bucketMutex.lock();

    auto it = m_buckets.find(id);
    if (it != m_buckets.end())
    {
        res = it->second;
    }

    m_bucketMutex.unlock();

    return res;
}

udp::UDPResponseBucket& udp::UDPClientObject::GetOrCreateBucket(int id)
{
    udp::UDPResponseBucket* res = nullptr;

    m_bucketMutex.lock();

    auto it = m_buckets.find(id);
    if (it == m_buckets.end())
    {
        res = new UDPResponseBucket();
        m_buckets[id] = res;
    }
    else
    {
        res = it->second;
    }

    m_bucketMutex.unlock();

    return *res;
}

void* udp::UDPClientObject::GetClientSock()
{
    return m_clientSock;
}