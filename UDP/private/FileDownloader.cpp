#include "FileDownloader.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"
#include "Jobs.h"

#include "UDP.h"

#include <WinSock2.h>
#include <iostream>
#include <chrono>
#include <thread>

namespace
{
    udp::FileDownloaderMeta m_instance;
    udp::FileDownloaderJSMeta m_fileDownloaderJSMeta;

    jobs::JobSystem* m_fileDownloaderJS = nullptr;


    struct ClientSocket
    {
        SOCKET m_socket;
        sockaddr_in ClientAddr;
    };
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


udp::FileDownloaderObject::FileDownloaderObject(int fileId, size_t fileSize, const std::string& path) :
    BaseObject(FileDownloaderMeta::GetInstance()),
    m_fileId(fileId),
    m_fileSize(fileSize),
    m_path(path)
{
    udp::Init();

    m_clientSock = new ClientSocket();

    if (!m_fileDownloaderJS)
    {
        m_fileDownloaderJS = new jobs::JobSystem(udp::FileDownloaderJSMeta::GetInstance(), 1);
    }

    m_fileDownloaderJS->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        ClientSocket* clientSock = static_cast<ClientSocket*>(m_clientSock);
        
        SOCKET& SendSocket = clientSock->m_socket;
        sockaddr_in& ClientAddr = clientSock->ClientAddr;

        SendSocket = INVALID_SOCKET;
        SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (SendSocket == INVALID_SOCKET) {
            std::cout << "socket failed with error " << SendSocket << std::endl;
            return;
        }

        class PingServer : public jobs::Job
        {
        private:
            FileDownloaderObject& m_self;
        public:
            PingServer(FileDownloaderObject& self) :
                m_self(self)
            {
            }

            void Do() override
            {
                ClientSocket* clientSock = static_cast<ClientSocket*>(m_self.m_clientSock);

                SOCKET& SendSocket = clientSock->m_socket;
                sockaddr_in& ClientAddr = clientSock->ClientAddr;

                udp::UDPReq req;

                int clientAddrSize = (int)sizeof(ClientAddr);
                short port = 27015;
                const char* local_host = "127.0.0.1";
                ClientAddr.sin_family = AF_INET;
                ClientAddr.sin_port = htons(port);
                ClientAddr.sin_addr.s_addr = inet_addr(local_host);

                int clientResult = sendto(SendSocket,
                    reinterpret_cast<char*>(&req), sizeof(req), 0, (SOCKADDR*)&ClientAddr, clientAddrSize);

                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                jobs::RunAsync(new PingServer(m_self));
            }
        };
        jobs::RunAsync(new PingServer(*this));

        char buff[1025];
        int buffLen = 1024;

        while (true)
        {
            struct sockaddr_in SenderAddr;
            int SenderAddrSize = sizeof(SenderAddr);
            int bytes_received = recvfrom(SendSocket, buff, buffLen, 0 /* no flags*/, (SOCKADDR*)&SenderAddr, &SenderAddrSize);

            bool t = true;
        }

    }));

}

udp::FileDownloaderObject::~FileDownloaderObject()
{
    delete m_clientSock;
}
