#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// 接收消息的线程函数
void* ReceiveThread(void* socket_ptr) {
    int client_socket = *(int*)socket_ptr;
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
            // 在非阻塞模式下，需要忽略 EAGAIN 和 EWOULDBLOCK
            usleep(100000); // 100ms
        }
    }

    return NULL;
}

int GetConsoleInput(char* buffer, int buffer_size) {
    if (fgets(buffer, buffer_size, stdin) == NULL) {
        return 0;
    }

    int len = (int)strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }
    if (len > 0 && buffer[len - 1] == '\r') {
        buffer[len - 1] = '\0';
        len--;
    }

    return len;
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char username[50];
    pthread_t receive_thread;

    printf("=== 聊天客户端 (macOS) ===\n\n");

    printf("请输入您的用户名: ");
    fflush(stdout);

    int username_len = GetConsoleInput(username, sizeof(username));

    if (username_len == 0) {
        strncpy(username, "匿名用户", 50);
    }

    printf("正在连接到服务器...\n");

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("创建socket失败");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("连接服务器失败");
        close(client_socket);
        return 1;
    }

    // 立即发送用户名
    send(client_socket, username, strlen(username), 0);

    printf("连接成功！\n");
    printf("输入消息开始聊天，输入 'quit' 退出\n\n");

    // 设置为非阻塞模式
    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

    // 创建接收线程
    if (pthread_create(&receive_thread, NULL, ReceiveThread, &client_socket) != 0) {
        perror("创建接收线程失败");
        close(client_socket);
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

        if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
            send(client_socket, buffer, strlen(buffer), 0);
            break;
        }

        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("发送失败");
            break;
        }
    }

    close(client_socket);
    printf("客户端已退出\n");
    return 0;
}
