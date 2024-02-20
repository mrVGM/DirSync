#include "UDP.h"

#include <WinSock2.h>

#include <iostream>

void udp::Init()
{
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != NO_ERROR) {
        std::cout << "WSAStartup failed with error " << res << std::endl;
    }
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


    int bytes_received;
    char serverBuf[1025];
    int serverBufLen = 1024;

    // Keep a seperate address struct to store sender information. 
    struct sockaddr_in SenderAddr;
    int SenderAddrSize = sizeof(SenderAddr);

    std::cout << "Receiving datagrams on " << "127.0.0.1" << std::endl;
    bytes_received = recvfrom(serverSocket, serverBuf, serverBufLen, 0 /* no flags*/, (SOCKADDR*)&SenderAddr, &SenderAddrSize);
    if (bytes_received == SOCKET_ERROR) {
        std::cout << "recvfrom failed with error" << WSAGetLastError();
    }
    serverBuf[bytes_received] = '\0';


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

    char SendBuf[1024];
    int BufLen = (int)(sizeof(SendBuf) - 1);
    const char* toSend = "foobar";
    strcpy(SendBuf, toSend);

    struct sockaddr_in ClientAddr;
    int clientAddrSize = (int)sizeof(ClientAddr);
    short port = 27015;
    const char* local_host = "127.0.0.1";
    ClientAddr.sin_family = AF_INET;
    ClientAddr.sin_port = htons(port);
    ClientAddr.sin_addr.s_addr = inet_addr(local_host);
    puts("Sending a datagram to the receiver...");

    int clientResult = sendto(SendSocket,
        SendBuf, BufLen, 0, (SOCKADDR*)&ClientAddr, clientAddrSize);

    char buff[1025];
    int buffLen = 1024;

    struct sockaddr_in SenderAddr;
    int SenderAddrSize = sizeof(SenderAddr);
    int bytes_received = recvfrom(SendSocket, buff, buffLen, 0 /* no flags*/, (SOCKADDR*)&SenderAddr, &SenderAddrSize);

    bool t = true;
}