#include "FileDownloader.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"

#include <iostream>
#include <WinSock2.h>

namespace
{
	udp::FileDownloaderJSMeta m_downloaderJSMeta;
	udp::FileDownloaderMeta m_downloaderMeta;

    jobs::JobSystem* m_downloaderJS = nullptr;
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

udp::FileDownloaderObject::FileDownloaderObject() :
    BaseObject(FileDownloaderMeta::GetInstance()),
    m_bucket(0)
{
    if (!m_downloaderJS)
    {
        m_downloaderJS = new jobs::JobSystem(FileDownloaderJSMeta::GetInstance(), 3 * 5);
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

    m_downloaderJS->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        struct sockaddr_in serverAddr;
        short port = 27015;
        const char* local_host = "127.0.0.1";
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr(local_host);

        while (true)
        {
            Packet pkt;
            sendto(m_socket, reinterpret_cast<char*>(&pkt), sizeof(pkt), 0 /* no flags*/, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(sockaddr_in));

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }));

    m_downloaderJS->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        while (true)
        {
            Packet pkt;
            sockaddr_in from;
            int size = sizeof(from);
            int bytes_received = recvfrom(m_socket, reinterpret_cast<char*>(&pkt), sizeof(Packet), 0, reinterpret_cast<sockaddr*>(&from), &size);

            if (bytes_received == SOCKET_ERROR) {
                std::cout << "recvfrom failed with error" << WSAGetLastError();
                continue;
            }

            m_bucket.PushPacket(pkt);
        }
    }));

    m_downloaderJS->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        while (true)
        {
            std::list<Packet>& packets = m_bucket.GetAccumulated();

            for (auto it = packets.begin(); it != packets.end(); ++it)
            {
                std::cout << "Packet Received" << std::endl;
            }

            packets.clear();
        }
    }));
}
