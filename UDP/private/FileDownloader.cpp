#include "FileDownloader.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"

#include "UDP.h"

#include <WinSock2.h>

#include <iostream>

namespace
{   
    udp::FileDownloaderMeta m_instance;
    udp::FileDownloaderJSMeta m_fileDownloaderJSMeta;

    jobs::JobSystem* m_fileDownloaderJS = nullptr;
}

udp::FileDownloaderJSMeta::FileDownloaderJSMeta() :
    BaseObjectMeta(&jobs::JobSystemMeta::GetInstance())
{
}

const udp::FileDownloaderJSMeta& udp::FileDownloaderJSMeta::GetInstance()
{
    return m_fileDownloaderJSMeta;
}

udp::FileDownloaderMeta::FileDownloaderMeta() :
    BaseObjectMeta(nullptr)
{
}

const udp::FileDownloaderMeta& udp::FileDownloaderMeta::GetInstance()
{
    return m_instance;
}


udp::FileDownloaderObject::FileDownloaderObject() :
    BaseObject(FileDownloaderMeta::GetInstance())
{
    udp::Init();

    if (!m_fileDownloaderJS)
    {
        m_fileDownloaderJS = new jobs::JobSystem(udp::FileDownloaderJSMeta::GetInstance(), 1);
    }

    m_fileDownloaderJS->ScheduleJob(jobs::Job::CreateFromLambda([]() {
        SOCKET SendSocket = INVALID_SOCKET;
        SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (SendSocket == INVALID_SOCKET) {
            std::cout << "socket failed with error " << SendSocket << std::endl;
            return;
        }

        udp::UDPReq req;

        struct sockaddr_in ClientAddr;
        int clientAddrSize = (int)sizeof(ClientAddr);
        short port = 27015;
        const char* local_host = "127.0.0.1";
        ClientAddr.sin_family = AF_INET;
        ClientAddr.sin_port = htons(port);
        ClientAddr.sin_addr.s_addr = inet_addr(local_host);

        int clientResult = sendto(SendSocket,
            reinterpret_cast<char*>(&req), sizeof(req), 0, (SOCKADDR*)&ClientAddr, clientAddrSize);

        char buff[1025];
        int buffLen = 1024;

        struct sockaddr_in SenderAddr;
        int SenderAddrSize = sizeof(SenderAddr);
        int bytes_received = recvfrom(SendSocket, buff, buffLen, 0 /* no flags*/, (SOCKADDR*)&SenderAddr, &SenderAddrSize);

        bool t = true;
    }));

}

udp::FileDownloaderObject::~FileDownloaderObject()
{
}
