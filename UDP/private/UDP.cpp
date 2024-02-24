#include "UDP.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"
#include "Jobs.h"

#include "FileManager.h"

#include "UDP.h"

#include "BaseObjectContainer.h"

#include <WinSock2.h>
#include <iostream>

namespace
{
    bool m_udpInitialized = false;
    
    udp::UDPServerMeta m_instance;
    udp::UDPServerJSMeta m_udpServerJSMeta;

    jobs::JobSystem* m_udpServerJS = nullptr;
    udp::FileManagerObject* m_fileManager = nullptr;
}

udp::UDPServerJSMeta::UDPServerJSMeta() :
    BaseObjectMeta(&jobs::JobSystemMeta::GetInstance())
{
}

const udp::UDPServerJSMeta& udp::UDPServerJSMeta::GetInstance()
{
    return m_udpServerJSMeta;
}

udp::UDPServerMeta::UDPServerMeta() :
    BaseObjectMeta(nullptr)
{
}

const udp::UDPServerMeta& udp::UDPServerMeta::GetInstance()
{
    return m_instance;
}


udp::UDPServerObject::UDPServerObject() :
    BaseObject(UDPServerMeta::GetInstance())
{
    if (!m_fileManager)
    {
        BaseObjectContainer& container = BaseObjectContainer::GetInstance();
        BaseObject* obj = container.GetObjectOfClass(FileManagerMeta::GetInstance());

        m_fileManager = static_cast<udp::FileManagerObject*>(obj);
    }

    udp::Init();

    if (!m_udpServerJS)
    {
        m_udpServerJS = new jobs::JobSystem(udp::UDPServerJSMeta::GetInstance(), 1);
    }

    m_udpServerJS->ScheduleJob(jobs::Job::CreateFromLambda([]() {
        SOCKET serverSocket = INVALID_SOCKET;
        serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (serverSocket == INVALID_SOCKET) {
            std::cout << "socket failed with error " << serverSocket << std::endl;
            return;
        }

        struct sockaddr_in serverAddr;
        short port = 27015;

        // Bind the socket to any address and the specified port.
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        // OR, you can do serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr))) {
            std::cout << "bind failed with error " << WSAGetLastError() << std::endl;
            return;
        }

        udp::UDPReq req;
        int bytes_received;

        // Keep a seperate address struct to store sender information. 
        struct sockaddr_in SenderAddr;
        int SenderAddrSize = sizeof(SenderAddr);
        std::cout << "Receiving datagrams on " << "127.0.0.1" << std::endl;

        while (true)
        {
            bytes_received = recvfrom(serverSocket, reinterpret_cast<char*>(&req), sizeof(req), 0 /* no flags*/, (SOCKADDR*)&SenderAddr, &SenderAddrSize);
            if (bytes_received == SOCKET_ERROR) {
                std::cout << "recvfrom failed with error" << WSAGetLastError();
                continue;
            }

            if (!req.m_shouldContinue)
            {
                break;
            }

            jobs::RunAsync(jobs::Job::CreateFromLambda([=]() {
                FileEntry* fileEntry = m_fileManager->GetFile(req.m_fileId);

                for (int i = 0; i < FileChunk::m_chunkKBSize; ++i)
                {
                    if (!req.GetBitState(i))
                    {
                        continue;
                    }
                    udp::UDPRes res;
                    res.m_offset = req.m_offset + i;

                    bool gottenKB = fileEntry->GetKB(req.m_offset + i, res.m_data);
                    if (!gottenKB)
                    {
                        continue;
                    }
                    res.m_state = UDPResState::m_full;

                    int sendResult = sendto(serverSocket,
                        reinterpret_cast<char*>(&res), sizeof(res), 0, (SOCKADDR*)&SenderAddr, SenderAddrSize);

                    if (sendResult == SOCKET_ERROR) {
                        std::cout << "Sending back response got an error: " << WSAGetLastError();
                    }
                }
            }));
        }

    }));
}

udp::UDPServerObject::~UDPServerObject()
{
}


void udp::Init()
{
    if (m_udpInitialized)
    {
        return;
    }

    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != NO_ERROR) {
        std::cout << "WSAStartup failed with error " << res << std::endl;
    }

    m_udpInitialized = true;
}

void udp::UDPServer()
{
    
}

void udp::UDPReq::UpBit(unsigned int bitNumber)
{
    unsigned int byte = bitNumber / 8;
    unsigned int bitOffset = bitNumber % 8;

    unsigned char byteMask = 1;
    byteMask = byteMask << bitOffset;

    m_mask[byte] |= byteMask;
}

bool udp::UDPReq::GetBitState(unsigned int bitNumber) const
{
    unsigned int byte = bitNumber / 8;
    unsigned int bitOffset = bitNumber % 8;

    unsigned char byteMask = 1;
    byteMask = byteMask << bitOffset;

    return (m_mask[byte] & byteMask);
}

size_t udp::FileChunk::m_chunkKBSize = 8 * 1024;


udp::FileChunk::~FileChunk()
{
    if (m_data)
    {
        delete[] m_data;
    }

    m_data = nullptr;
}

bool udp::FileChunk::GetKB(size_t globalKBPos, KB& outKB)
{
    size_t startingKB = m_startingByte / sizeof(KB);

    if (globalKBPos < startingKB)
    {
        return false;
    }

    if (globalKBPos >= startingKB + m_chunkKBSize)
    {
        return false;
    }

    outKB = GetData()[globalKBPos - startingKB];
}

udp::FileChunk::KBPos udp::FileChunk::GetKBPos(size_t globalKBPos)
{
    size_t startingKBPos = m_startingByte / sizeof(KB);
    if (globalKBPos < startingKBPos)
    {
        return FileChunk::KBPos::Left;
    }

    if (globalKBPos >= startingKBPos + m_chunkKBSize)
    {
        return FileChunk::KBPos::Right;
    }

    return udp::FileChunk::Inside;
}

udp::KB* udp::FileChunk::GetData()
{
    if (!m_data)
    {
        m_data = new KB[m_chunkKBSize];
    }

    return m_data;
}

udp::UDPResState udp::UDPResState::m_empty = udp::UDPResState('e');
udp::UDPResState udp::UDPResState::m_full = udp::UDPResState('f');
udp::UDPResState udp::UDPResState::m_blank = udp::UDPResState('b');

udp::UDPResState::UDPResState(char state) :
    m_state(state)
{
}

bool udp::UDPResState::Equals(const UDPResState& other)
{
    return m_state == other.m_state;
}


