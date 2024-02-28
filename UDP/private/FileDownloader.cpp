#include "FileDownloader.h"

#include "UDPClient.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"
#include "Jobs.h"

#include "FileWriter.h"
#include "UDP.h"

#include "BaseObjectContainer.h"

#include <WinSock2.h>
#include <chrono>
#include <thread>
#include <iostream>

namespace
{
    udp::FileDownloaderMeta m_instance;

    jobs::JobSystem* m_downloaderJS = nullptr;
    jobs::JobSystem* m_pingJS = nullptr;

    struct ClientSocket
    {
        SOCKET m_socket;
        sockaddr_in ClientAddr;
    };

    void CacheJS()
    {
        using namespace udp;

        BaseObjectContainer& container = BaseObjectContainer::GetInstance();

        {
            BaseObject* js = container.GetObjectOfClass(FileDownloadersJSMeta::GetInstance());
            if (!js)
            {
                js = new jobs::JobSystem(UDPClientJSMeta::GetInstance(), 1);
            }

            m_downloaderJS = static_cast<jobs::JobSystem*>(js);
        }

        {
            BaseObject* js = container.GetObjectOfClass(PingServerJSMeta::GetInstance());
            if (!js)
            {
                js = new jobs::JobSystem(PingServerJSMeta::GetInstance(), 1);
            }

            m_pingJS = static_cast<jobs::JobSystem*>(js);
        }
    }
}

udp::FileDownloaderMeta::FileDownloaderMeta() :
    BaseObjectMeta(nullptr)
{
}

const udp::FileDownloaderMeta& udp::FileDownloaderMeta::GetInstance()
{
    return m_instance;
}


udp::FileDownloaderObject::FileDownloaderObject(
    UDPClientObject& udpClient,
    const std::string& ipAddr,
    int fileId,
    ull fileSize,
    const std::string& path,
    const std::function<void()>& downloadFinished) :

    m_udpClient(udpClient),
    BaseObject(FileDownloaderMeta::GetInstance()),
    m_fileId(fileId),
    m_fileSize(fileSize),
    m_path(path),
    m_downloadFinished(downloadFinished)
{
    udp::Init();
    CacheJS();

    ClientSocket* clientSock = static_cast<ClientSocket*>(m_udpClient.GetClientSock());

    m_downloaderJS->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
        SOCKET& SendSocket = clientSock->m_socket;
        sockaddr_in& ClientAddr = clientSock->ClientAddr;

        short port = 27015;
        const char* local_host = ipAddr.c_str();
        ClientAddr.sin_family = AF_INET;
        ClientAddr.sin_port = htons(port);
        ClientAddr.sin_addr.s_addr = inet_addr(local_host);

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
                    if (!m_self.m_dataReceived)
                    {
                        break;
                    }

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

                ClientSocket* clientSock = static_cast<ClientSocket*>(m_self.m_udpClient.GetClientSock());

                SOCKET& SendSocket = clientSock->m_socket;
                sockaddr_in& ClientAddr = clientSock->ClientAddr;

                int clientAddrSize = (int)sizeof(ClientAddr);
                int clientResult = sendto(SendSocket,
                    reinterpret_cast<char*>(&req), sizeof(req), 0, (SOCKADDR*)&ClientAddr, clientAddrSize);

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                jobs::RunAsync(new PingServer(m_self, m_done));
            }
        };
        jobs::RunAsync(new PingServer(*this, jobs::Job::CreateFromLambda(itemFinished)));


        ull numKB = ceil((double)m_fileSize / sizeof(KB));
        ull numChunks = ceil((double)numKB / FileChunk::m_chunkKBSize);

        FileWriter* fileWriter = nullptr;
        for (ull i = 0; i < numChunks; ++i)
        {
            if (!fileWriter)
            {
                fileWriter = new FileWriter(m_path, m_fileSize);
            }

            ull startKB = i * FileChunk::m_chunkKBSize;
            m_fileKBOffset = startKB;

            m_dataReceived = new udp::UDPRes[FileChunk::m_chunkKBSize];
            for (int j = 0; j < FileChunk::m_chunkKBSize; ++j)
            {
                m_dataReceived[j].m_state = UDPResState::m_empty;
            }

            for (int j = numKB - startKB; j < FileChunk::m_chunkKBSize; ++j)
            {
                m_dataReceived[j].m_state = UDPResState::m_blank;
            }

            UDPResponseBucket& bucket = m_udpClient.GetOrCreateBucket(m_fileId);

            bool missing = true;
            while (missing)
            {
                std::list<UDPRes>& resList = bucket.GetAccumulated();

                for (auto it = resList.begin(); it != resList.end(); ++it)
                {
                    const UDPRes& res = *it;

                    if (res.m_fileId == m_fileId && res.m_offset >= startKB)
                    {
                        m_dataReceived[res.m_offset - startKB] = res;
                    }

                    ull received = i * FileChunk::m_chunkKBSize * sizeof(KB);
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

                    if (!missing)
                    {
                        break;
                    }
                }

                resList.clear();
            }

            fileWriter->PushToQueue(m_dataReceived);
        }
        m_done = true;

        if (fileWriter)
        {
            fileWriter->Finalize(jobs::Job::CreateFromLambda([=]() {
                itemFinished();
                delete fileWriter;
            }));
        }
        else
        {
            jobs::RunSync(jobs::Job::CreateFromLambda(itemFinished));
        }

        jobs::RunSync(jobs::Job::CreateFromLambda(itemFinished));
    }));
}


int udp::FileDownloaderObject::GetFileId() const
{
    return m_fileId;
}

void udp::FileDownloaderObject::GetProgress(ull& finished, ull& all) const
{
    finished = m_bytesReceived;
    all = m_fileSize;
}

udp::FileDownloaderObject::~FileDownloaderObject()
{
}