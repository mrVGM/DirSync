#include "UDP.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"
#include "Job.h"

#include <WinSock2.h>

#include <iostream>

namespace
{
    bool m_udpInitialized = false;
    
    udp::UDPServerMeta m_instance;
    udp::UDPServerJSMeta m_udpServerJSMeta;

    jobs::JobSystem* m_udpServerJS = nullptr;
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
    udp::Init();

    if (!m_udpServerJS)
    {
        m_udpServerJS = new jobs::JobSystem(udp::UDPServerJSMeta::GetInstance(), 1);
    }

    m_udpServerJS->ScheduleJob(jobs::Job::CreateFromLambda([]() {
        udp::UDPServer();
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
        }

        if (!req.m_shouldContinue)
        {
            break;
        }
    }

    char sendBuf[] = { 'h', 'e', 'l', 'l', 'o', '\0' };
    int sendBufLen = (int)(sizeof(sendBuf) - 1);
    int sendResult = sendto(serverSocket,
        sendBuf, sendBufLen, 0, (SOCKADDR*)&SenderAddr, SenderAddrSize);
    if (sendResult == SOCKET_ERROR) {
        std::cout << "Sending back response got an error: " << WSAGetLastError();
    }
}


void udp::UDPClient()
{
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
}

void udp::UDPReq::UpBit(unsigned int bitNumber)
{
    unsigned int byte = bitNumber / 8;
    unsigned int bitOffset = bitNumber % 8;

    unsigned char byteMask = 1;
    byteMask = byteMask << bitOffset;

    m_mask[byte] |= byteMask;
}

bool udp::UDPReq::GetBitState(unsigned int bitNumber)
{
    unsigned int byte = bitNumber / 8;
    unsigned int bitOffset = bitNumber % 8;

    unsigned char byteMask = 1;
    byteMask = byteMask << bitOffset;

    return (m_mask[byte] & byteMask);
}