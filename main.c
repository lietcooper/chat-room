
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <locale.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// 彻底解决中文编码问题
void SetupConsole() {
    // 使用GBK编码（中文Windows默认）
    SetConsoleOutputCP(936);
    SetConsoleCP(936);
    setlocale(LC_ALL, "chs");
}
int main() {
    SetupConsole();

    WSADATA wsaData;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    SOCKET client_sockets[MAX_CLIENTS] = { 0 };
    char client_names[MAX_CLIENTS][50] = { 0 };
    int client_count = 0;

    printf("=== 聊天服务器 ===\n");
    printf("正在初始化...\n");

    // 初始化Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup失败: %d\n", WSAGetLastError());
        return 1;
    }

    // 创建服务器socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("创建socket失败: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("绑定失败: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // 开始监听
    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("监听失败: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("服务器启动成功，监听端口: %d\n", PORT);
    printf("等待客户端连接...\n\n");

    // 初始化客户端socket数组
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = INVALID_SOCKET;
    }

    // 主循环
    while (1) {
        // 清空socket集合
        FD_ZERO(&read_fds);

        // 添加服务器socket
        FD_SET(server_socket, &read_fds);

        // 添加客户端sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != INVALID_SOCKET) {
                FD_SET(client_sockets[i], &read_fds);
            }
        }

        // 设置超时时间
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // 等待socket活动
        int activity = select(0, &read_fds, NULL, NULL, &timeout);

        if (activity == SOCKET_ERROR) {
            printf("select错误: %d\n", WSAGetLastError());
            break;
        }

        // 检查新连接
        if (FD_ISSET(server_socket, &read_fds)) {
            client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_socket == INVALID_SOCKET) {
                printf("接受连接失败: %d\n", WSAGetLastError());
                continue;
            }

            // 查找空闲位置存放新客户端
            int client_index = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == INVALID_SOCKET) {
                    client_sockets[i] = client_socket;
                    client_index = i;
                    client_count++;
                    break;
                }
            }

            if (client_index == -1) {
                printf("服务器已满，拒绝新连接\n");
                const char* msg = "服务器已满，请稍后重试\n";
                send(client_socket, msg, (int)strlen(msg), 0);
                closesocket(client_socket);
                continue;
            }

            // 接收客户端用户名
            int name_len = recv(client_socket, client_names[client_index], sizeof(client_names[client_index]) - 1, 0);
            if (name_len > 0) {
                client_names[client_index][name_len] = '\0';

                // 尝试修复中文用户名乱码
                // 如果用户名看起来是乱码，尝试转换编码
                int has_chinese = 0;
                for (int j = 0; j < name_len; j++) {
                    if ((unsigned char)client_names[client_index][j] > 127) {
                        has_chinese = 1;
                        break;
                    }
                }

                if (has_chinese) {
                    // 简单处理：直接使用接收到的数据，假设客户端和服务端编码一致
                    // 在实际应用中，这里应该进行编码转换
                }
            }
            else {
                strcpy_s(client_names[client_index], sizeof(client_names[client_index]), "匿名用户");
            }

            printf("新客户端连接: %s (IP: %s)\n", client_names[client_index], inet_ntoa(client_addr.sin_addr));

            // 发送欢迎消息
            char welcome_msg[BUFFER_SIZE];
            _snprintf_s(welcome_msg, sizeof(welcome_msg), _TRUNCATE,
                "欢迎 %s 加入聊天室！当前在线用户: %d\n",
                client_names[client_index], client_count);
            send(client_socket, welcome_msg, (int)strlen(welcome_msg), 0);

            // 广播新用户加入消息
            char join_msg[BUFFER_SIZE];
            _snprintf_s(join_msg, sizeof(join_msg), _TRUNCATE,
                "系统: %s 加入了聊天室\n", client_names[client_index]);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] != INVALID_SOCKET && i != client_index) {
                    send(client_sockets[i], join_msg, (int)strlen(join_msg), 0);
                }
            }
        }

        // 检查客户端消息
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == INVALID_SOCKET) continue;

            if (FD_ISSET(client_sockets[i], &read_fds)) {
                int bytes_received = recv(client_sockets[i], buffer, BUFFER_SIZE - 1, 0);

                if (bytes_received <= 0) {
                    // 客户端断开连接
                    printf("客户端断开: %s\n", client_names[i]);

                    // 广播离开消息
                    char leave_msg[BUFFER_SIZE];
                    _snprintf_s(leave_msg, sizeof(leave_msg), _TRUNCATE,
                        "系统: %s 离开了聊天室\n", client_names[i]);

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_sockets[j] != INVALID_SOCKET && j != i) {
                            send(client_sockets[j], leave_msg, (int)strlen(leave_msg), 0);
                        }
                    }

                    closesocket(client_sockets[i]);
                    client_sockets[i] = INVALID_SOCKET;
                    client_count--;
                    continue;
                }

                // 处理收到的消息
                buffer[bytes_received] = '\0';

                // 移除换行符
                if (bytes_received > 0 && buffer[bytes_received - 1] == '\n') {
                    buffer[bytes_received - 1] = '\0';
                }

                // 显示消息
                printf("%s: %s\n", client_names[i], buffer);

                // 检查退出命令
                if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
                    const char* goodbye = "再见！\n";
                    send(client_sockets[i], goodbye, (int)strlen(goodbye), 0);
                    closesocket(client_sockets[i]);
                    client_sockets[i] = INVALID_SOCKET;
                    client_count--;
                    continue;
                }

                // 广播消息给所有其他客户端
                char broadcast_msg[BUFFER_SIZE + 100];
                _snprintf_s(broadcast_msg, sizeof(broadcast_msg), _TRUNCATE,
                    "%s: %s\n", client_names[i], buffer);

                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (client_sockets[j] != INVALID_SOCKET && j != i) {
                        send(client_sockets[j], broadcast_msg, (int)strlen(broadcast_msg), 0);
                    }
                }
            }
        }

        // 减少CPU占用
        Sleep(10);
    }

    // 清理资源
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != INVALID_SOCKET) {
            closesocket(client_sockets[i]);
        }
    }

    closesocket(server_socket);
    WSACleanup();

    printf("服务器已关闭\n");
    return 0;
}