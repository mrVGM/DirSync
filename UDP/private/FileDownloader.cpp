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


udp::FileDownloaderObject::FileDownloaderObject(int fileId, size_t fileSize, const std::string& path, const std::function<void()>& downloadFinished) :
    BaseObject(FileDownloaderMeta::GetInstance()),
    m_fileId(fileId),
    m_fileSize(fileSize),
    m_path(path),
    m_downloadFinished(downloadFinished)
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
                downloadFinished();
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
                req.m_offset = m_self.m_fileKBOffset;
                req.m_fileId = m_self.m_fileId;

                bool missing = false;
                for (int i = 0; i < FileChunk::m_chunkKBSize; ++i)
                {
                    if (m_self.m_dataReceived[i].m_state.Equals(UDPResState::m_empty))
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

        size_t numKB = ceil((double)m_fileSize / sizeof(KB));
        size_t numChunks = ceil((double)numKB / FileChunk::m_chunkKBSize);

        FILE* f = nullptr;
        fopen_s(&f, m_path.c_str(), "wb");

        for (size_t i = 0; i < numChunks; ++i)
        {
            std::cout << i << std::endl;

            for (int j = 0; j < FileChunk::m_chunkKBSize; ++j)
            {
                m_dataReceived[j].m_state = UDPResState::m_empty;
            }

            size_t startKB = i * FileChunk::m_chunkKBSize;
            m_fileKBOffset = startKB;
            for (int j = numKB - startKB; j < FileChunk::m_chunkKBSize; ++j)
            {
                m_dataReceived[j].m_state = UDPResState::m_blank;
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
                    if (res.m_offset >= startKB)
                    {
                        m_dataReceived[res.m_offset - startKB] = res;
                    }
                }

                size_t received = i * FileChunk::m_chunkKBSize * sizeof(KB);
                missing = false;
                for (int i = 0; i < FileChunk::m_chunkKBSize; ++i)
                {
                    if (m_dataReceived[i].m_state.Equals(UDPResState::m_empty))
                    {
                        missing = true;
                    }
                    else
                    {
                        received += sizeof(KB);
                    }
                }
                m_bytesReceived = min(received, m_fileSize);
            }

            size_t startByte = i * FileChunk::m_chunkKBSize * sizeof(KB);
            for (size_t j = 0; j < FileChunk::m_chunkKBSize; ++j)
            {
                size_t cur = startByte + j * sizeof(KB);
                if (cur >= m_fileSize)
                {
                    break;
                }

                size_t endByte = min(m_fileSize, cur + sizeof(KB));
                fwrite(&m_dataReceived[j].m_data, sizeof(char), endByte - cur, f);
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


int udp::FileDownloaderObject::GetFileId() const
{
    return m_fileId;
}

void udp::FileDownloaderObject::GetProgress(size_t& finished, size_t& all) const
{
    finished = m_bytesReceived;
    all = m_fileSize;
}