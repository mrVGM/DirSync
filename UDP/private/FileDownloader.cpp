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
    m_dataReceived = new udp::UDPRes[8 * 1024];

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

        auto itemFinished = [=]() {
            --m_toFinish;

            if (m_toFinish > 0)
            {
                return;
            }

            jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
                delete this;
            }));
        };

        class PingServer : public jobs::Job
        {
        private:
            FileDownloaderObject& m_self;
            jobs::Job* m_done = nullptr;
        public:
            PingServer(FileDownloaderObject& self, jobs::Job* done) :
                m_self(self),
                m_done(done)
            {
            }

            void Do() override
            {
                if (m_self.m_done)
                {
                    jobs::RunSync(m_done);
                    return;
                }

                udp::UDPReq req;
                req.m_offset = m_self.m_fileOffset;
                req.m_fileId = m_self.m_fileId;

                bool missing = false;
                for (int i = 0; i < 8 * 1024; ++i)
                {
                    if (!m_self.m_dataReceived[i].m_valid)
                    {
                        req.UpBit(i);
                        missing = true;
                    }
                }

                if (!missing)
                {
                    jobs::RunAsync(new PingServer(m_self, m_done));
                    return;
                }

                ClientSocket* clientSock = static_cast<ClientSocket*>(m_self.m_clientSock);

                SOCKET& SendSocket = clientSock->m_socket;
                sockaddr_in& ClientAddr = clientSock->ClientAddr;


                int clientAddrSize = (int)sizeof(ClientAddr);
                short port = 27015;
                const char* local_host = "127.0.0.1";
                ClientAddr.sin_family = AF_INET;
                ClientAddr.sin_port = htons(port);
                ClientAddr.sin_addr.s_addr = inet_addr(local_host);

                int clientResult = sendto(SendSocket,
                    reinterpret_cast<char*>(&req), sizeof(req), 0, (SOCKADDR*)&ClientAddr, clientAddrSize);

                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                jobs::RunAsync(new PingServer(m_self, m_done));
            }
        };
        jobs::RunAsync(new PingServer(*this, jobs::Job::CreateFromLambda(itemFinished)));

        udp::UDPRes res;

        size_t numKB = m_fileSize / 1024 + 1;
        size_t numChunks = numKB / (8 * 1024) + 1;

        FILE* f = nullptr;
        fopen_s(&f, m_path.c_str(), "wb");

        for (size_t i = 0; i < numChunks; ++i)
        {
            size_t startKB = i * 8 * 1024;
            m_fileOffset = startKB;
            if (numKB - startKB < 8 * 1024)
            {
                for (int j = numKB - startKB; j < 1024; ++j)
                {
                    m_dataReceived[j].m_valid = true;
                }
            }

            bool missing = true;
            while (missing)
            {
                struct sockaddr_in SenderAddr;
                int SenderAddrSize = sizeof(SenderAddr);
                int bytes_received = recvfrom(SendSocket, reinterpret_cast<char*>(&res), sizeof(res), 0 /* no flags*/, (SOCKADDR*)&SenderAddr, &SenderAddrSize);

                if (bytes_received == SOCKET_ERROR)
                {
                    std::cout << "recvfrom failed with error" << WSAGetLastError();
                }
                else
                {
                    m_dataReceived[res.m_offset - startKB] = res;
                }

                missing = false;
                for (int i = 0; i < 8 * 1024; ++i)
                {
                    if (!m_dataReceived[i].m_valid)
                    {
                        missing = true;
                        break;
                    }
                }
            }

            size_t startByte = i * 8 * 1024 * 1024;
            for (size_t j = 0; j < 8 * 1024; ++j)
            {
                size_t cur = startByte + j * 1024;
                if (cur >= m_fileSize)
                {
                    break;
                }

                size_t endByte = min(m_fileSize, cur + 1024);
                fwrite(m_dataReceived[j].m_data, sizeof(char), endByte - cur, f);
            }

        }
        fclose(f);

        m_done = true;
        jobs::RunSync(jobs::Job::CreateFromLambda(itemFinished));
    }));
}

udp::FileDownloaderObject::~FileDownloaderObject()
{
    delete m_clientSock;
    delete[] m_dataReceived;
}
