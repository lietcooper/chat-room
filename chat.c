#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <windows.h>
#include <process.h>
#include <locale.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

// 彻底解决中文编码问题
void SetupConsole() {
    // 使用GBK编码（中文Windows默认）
    SetConsoleOutputCP(936);
    SetConsoleCP(936);
    setlocale(LC_ALL, "chs");
}

// 接收消息的线程函数
unsigned __stdcall ReceiveThread(void* socket_ptr) {
    SOCKET client_socket = *(SOCKET*)socket_ptr;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("%s", buffer);
            printf("> ");
            fflush(stdout);
        }
        else if (bytes_received == 0) {
            printf("\n服务器连接已断开\n");
            break;
        }
        else {
            int error = WSAGetLastError();
            if (error == WSAECONNRESET) {
                printf("\n连接被服务器重置\n");
                break;
            }
            else if (error != WSAEWOULDBLOCK) {
                printf("\n接收错误: %d\n", error);
                break;
            }
        }

        Sleep(100);
    }

    return 0;
}

// 获取控制台输入，处理中文
int GetConsoleInput(char* buffer, int buffer_size) {
    if (fgets(buffer, buffer_size, stdin) == NULL) {
        return 0;
    }

    // 移除换行符
    int len = (int)strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }

    return len;
}

int main() {
    SetupConsole();

    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char username[50];
    HANDLE receive_thread;

    printf("=== 聊天客户端 ===\n\n");

    // 获取用户名 - 使用专门的函数处理
    printf("请输入您的用户名: ");
    fflush(stdout);

    int username_len = GetConsoleInput(username, sizeof(username));

    // 默认用户名
    if (username_len == 0) {
        strcpy_s(username, sizeof(username), "匿名用户");
        username_len = (int)strlen(username);
    }

    printf("正在连接到服务器...\n");

    // 初始化Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup失败: %d\n", WSAGetLastError());
        return 1;
    }

    // 创建客户端socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        printf("创建socket失败: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 本地服务器

    // 连接到服务器
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("连接服务器失败: %d\n", WSAGetLastError());
        printf("请确保服务器正在运行\n");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // 立即发送用户名到服务器 - 这是关键修复
    // 在连接建立后立即发送用户名，避免与其他数据混淆
    send(client_socket, username, (int)strlen(username), 0);

    printf("连接成功！\n");
    printf("输入消息开始聊天，输入 'quit' 退出\n\n");

    // 设置socket为非阻塞模式
    u_long mode = 1;
    ioctlsocket(client_socket, FIONBIO, &mode);

    // 创建接收线程
    receive_thread = (HANDLE)_beginthreadex(NULL, 0, ReceiveThread, &client_socket, 0, NULL);
    if (receive_thread == NULL) {
        printf("创建接收线程失败\n");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // 主循环 - 发送消息
    while (1) {
        printf("> ");
        fflush(stdout);

        int input_len = GetConsoleInput(buffer, sizeof(buffer));

        if (input_len == 0) {
            continue;
        }

        // 检查退出命令
        if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
            send(client_socket, buffer, (int)strlen(buffer), 0);
            break;
        }

        // 发送消息
        if (send(client_socket, buffer, (int)strlen(buffer), 0) == SOCKET_ERROR) {
            printf("发送失败: %d\n", WSAGetLastError());
            break;
        }
    }

    // 清理资源
    WaitForSingleObject(receive_thread, 2000);
    CloseHandle(receive_thread);
    closesocket(client_socket);
    WSACleanup();

    printf("客户端已退出\n");
    return 0;
}
