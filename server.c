#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 4444
#define BUF_SIZE 1024

void send_message(const char *msg) {
    MessageBoxA(NULL, msg, "Remote Message", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
}

void shutdown_pc(int seconds) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "shutdown /s /t %d /f", seconds);
    system(cmd);
}

void run_command(const char *cmd) {
    system(cmd);
}

int main() {
    WSADATA wsaData;
    SOCKET listenSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    char buffer[BUF_SIZE];
    int recvLen;

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, 1) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            printf("Accept failed\n");
            continue;
        }

        recvLen = recv(clientSocket, buffer, BUF_SIZE - 1, 0);
        if (recvLen > 0) {
            buffer[recvLen] = '\0';

            if (strncmp(buffer, "MSG ", 4) == 0) {
                send_message(buffer + 4);
                send(clientSocket, "OK\n", 3, 0);
            } else if (strncmp(buffer, "SHUTDOWN ", 9) == 0) {
                int secs = atoi(buffer + 9);
                shutdown_pc(secs);
                send(clientSocket, "OK\n", 3, 0);
            } else if (strncmp(buffer, "RUNCMD ", 7) == 0) {
                run_command(buffer + 7);
                send(clientSocket, "OK\n", 3, 0);
            } else {
                send(clientSocket, "UNKNOWN COMMAND\n", 16, 0);
            }
        }

        closesocket(clientSocket);
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
