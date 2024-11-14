#pragma once

#include <winsock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Bthprops.lib")

int StartBluetoothServer() {
    // Step 1: Initialize Winsock
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        std::cerr << "WSAStartup failed with error: " << res << std::endl;
        return 1;
    }

    // Step 2: Create a Bluetooth socket
    SOCKET serverSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Step 3: Bind the socket to a Bluetooth port (e.g., channel 3)
    SOCKADDR_BTH sa = { 0 };
    sa.addressFamily = AF_BTH;
    sa.port = BT_PORT_ANY;  // Dynamically assigns an available port

    if (bind(serverSocket, (SOCKADDR*)&sa, sizeof(sa)) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "listening..." << std::endl;
    SOCKET clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Client connected!" << std::endl;
    char recvBuffer[1024];
    int bytesReceived = recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
    if (bytesReceived > 0) {
        std::cout << "Received data: " << std::string(recvBuffer, bytesReceived) << std::endl;

        const char* sendData = "HellofromBluetoothServer!";
        send(clientSocket, sendData, strlen(sendData), 0);
    }
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}