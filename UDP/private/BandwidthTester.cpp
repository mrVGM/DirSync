#include "BandwidthTester.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"
#include "Jobs.h"

#include "BaseObjectContainer.h"

#include <queue>
#include <iostream>
#include <WinSock2.h>

namespace
{
	udp::BandWidthTesterJSMeta m_jsMeta;
	udp::BandWidthTesterMeta m_testerMeta;
}

const udp::BandWidthTesterJSMeta& udp::BandWidthTesterJSMeta::GetInstance()
{
	return m_jsMeta;
}

udp::BandWidthTesterJSMeta::BandWidthTesterJSMeta() :
	BaseObjectMeta(&jobs::JobSystemMeta::GetInstance())
{
}

const udp::BandWidthTesterMeta& udp::BandWidthTesterMeta::GetInstance()
{
    return m_testerMeta;
}

udp::BandWidthTesterMeta::BandWidthTesterMeta() :
    BaseObjectMeta(nullptr)
{
}


udp::BandWidthTesterObject::BandWidthTesterObject(
    int serverPort,
    const std::string& serverIP) :
    BaseObject(BandWidthTesterMeta::GetInstance()),
    m_serverPort(serverPort),
    m_serverIP(serverIP)
{
    BaseObjectContainer& container = BaseObjectContainer::GetInstance();

    BaseObject* tmp = container.GetObjectOfClass(BandWidthTesterJSMeta::GetInstance());
    if (!tmp)
    {
        new jobs::JobSystem(BandWidthTesterJSMeta::GetInstance(), 2);
    }
}

udp::BandWidthTesterObject::~BandWidthTesterObject()
{
}

void udp::BandWidthTesterObject::Init()
{
}

void udp::BandWidthTesterObject::StartTest(size_t numPackets, size_t delay, jobs::Job* done)
{
    m_received = 0;

    struct Context
    {
        bool m_running = true;
    };
    Context* ctx = new Context();

    jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
        BaseObjectContainer& container = BaseObjectContainer::GetInstance();
        BaseObject* tmp = container.GetObjectOfClass(BandWidthTesterJSMeta::GetInstance());

        jobs::JobSystem* js = static_cast<jobs::JobSystem*>(tmp);


        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) {
            std::cout << "socket failed with error " << sock << std::endl;
            return;
        }

        struct sockaddr_in serverAddr;
        short port = static_cast<short>(m_serverPort);
        const char* local_host = m_serverIP.c_str();
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr(local_host);

        Packet pkt;
        pkt.m_packetType = PacketType::m_test;
        pkt.m_offset = numPackets;

        sendto(
            sock,
            reinterpret_cast<char*>(&pkt),
            sizeof(Packet),
            0 /* no flags*/,
            reinterpret_cast<sockaddr*>(&serverAddr),
            sizeof(sockaddr_in));

        js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
            
            Packet tmp;
            struct sockaddr_in addr;
            int size = sizeof(addr);

            while (ctx->m_running)
            {
                recvfrom(
                    sock,
                    reinterpret_cast<char*>(&tmp),
                    sizeof(tmp),
                    0,
                    reinterpret_cast<sockaddr*>(&addr),
                    &size
                );

                ++m_received;
            }

            delete ctx;

            jobs::RunSync(done);
        }));

        js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            ctx->m_running = false;
            closesocket(sock);
        }));
    }));
}
