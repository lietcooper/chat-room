#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    int client_sockets[MAX_CLIENTS];
    char client_names[MAX_CLIENTS][50];
    int client_count = 0;

    printf("=== 聊天服务器 (macOS) ===\n");
    printf("正在初始化...\n");

    // 初始化客户端socket数组
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1;
        memset(client_names[i], 0, 50);
    }

    // 创建服务器socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("创建socket失败");
        return 1;
    }

    // 设置地址重用，方便调试时重启服务器
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("绑定失败");
        close(server_socket);
        return 1;
    }

    // 开始监听
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("监听失败");
        close(server_socket);
        return 1;
    }

    printf("服务器启动成功，监听端口: %d\n", PORT);
    printf("等待客户端连接...\n\n");

    // 主循环
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);
        int max_sd = server_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != -1) {
                FD_SET(client_sockets[i], &read_fds);
                if (client_sockets[i] > max_sd) max_sd = client_sockets[i];
            }
        }

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(max_sd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity < 0 && errno != EINTR) {
            perror("select错误");
            break;
        }

        // 检查新连接
        if (FD_ISSET(server_socket, &read_fds)) {
            client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_socket < 0) {
                perror("接受连接失败");
                continue;
            }

            int client_index = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == -1) {
                    client_sockets[i] = client_socket;
                    client_index = i;
                    client_count++;
                    break;
                }
            }

            if (client_index == -1) {
                printf("服务器已满，拒绝新连接\n");
                const char* msg = "服务器已满，请稍后重试\n";
                send(client_socket, msg, strlen(msg), 0);
                close(client_socket);
                continue;
            }

            // 接收用户名
            int name_len = recv(client_socket, client_names[client_index], sizeof(client_names[client_index]) - 1, 0);
            if (name_len > 0) {
                client_names[client_index][name_len] = '\0';
                // 移除可能的换行符
                if (client_names[client_index][name_len - 1] == '\n' || client_names[client_index][name_len - 1] == '\r') {
                    client_names[client_index][name_len - 1] = '\0';
                }
            } else {
                strncpy(client_names[client_index], "匿名用户", 50);
            }

            printf("新客户端连接: %s (IP: %s)\n", client_names[client_index], inet_ntoa(client_addr.sin_addr));

            char welcome_msg[BUFFER_SIZE];
            snprintf(welcome_msg, sizeof(welcome_msg), "欢迎 %s 加入聊天室！当前在线用户: %d\n", client_names[client_index], client_count);
            send(client_socket, welcome_msg, strlen(welcome_msg), 0);

            char join_msg[BUFFER_SIZE];
            snprintf(join_msg, sizeof(join_msg), "系统: %s 加入了聊天室\n", client_names[client_index]);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] != -1 && i != client_index) {
                    send(client_sockets[i], join_msg, strlen(join_msg), 0);
                }
            }
        }

        // 检查客户端消息
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == -1) continue;

            if (FD_ISSET(client_sockets[i], &read_fds)) {
                int bytes_received = recv(client_sockets[i], buffer, BUFFER_SIZE - 1, 0);

                if (bytes_received <= 0) {
                    printf("客户端断开: %s\n", client_names[i]);
                    char leave_msg[BUFFER_SIZE];
                    snprintf(leave_msg, sizeof(leave_msg), "系统: %s 离开了聊天室\n", client_names[i]);

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_sockets[j] != -1 && j != i) {
                            send(client_sockets[j], leave_msg, strlen(leave_msg), 0);
                        }
                    }

                    close(client_sockets[i]);
                    client_sockets[i] = -1;
                    client_count--;
                    continue;
                }

                buffer[bytes_received] = '\0';
                if (buffer[bytes_received - 1] == '\n') buffer[bytes_received - 1] = '\0';
                if (buffer[bytes_received - 1] == '\r') buffer[bytes_received - 1] = '\0';

                printf("%s: %s\n", client_names[i], buffer);

                if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
                    const char* goodbye = "再见！\n";
                    send(client_sockets[i], goodbye, strlen(goodbye), 0);
                    close(client_sockets[i]);
                    client_sockets[i] = -1;
                    client_count--;
                    continue;
                }

                char broadcast_msg[BUFFER_SIZE + 100];
                snprintf(broadcast_msg, sizeof(broadcast_msg), "%s: %s\n", client_names[i], buffer);
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (client_sockets[j] != -1 && j != i) {
                        send(client_sockets[j], broadcast_msg, strlen(broadcast_msg), 0);
                    }
                }
            }
        }
    }

    close(server_socket);
    return 0;
}
