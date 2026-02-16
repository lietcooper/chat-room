#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void* ReceiveThread(void* socket_ptr) {
    int client_socket = *(int*)socket_ptr;
    char buffer[BUFFER_SIZE];

    while (1) {
        int n = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            printf("\r%s> ", buffer);
            fflush(stdout);
        } else if (n == 0) {
            printf("\nServer disconnected\n");
            break;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            break;
        }
        usleep(100000);
    }
    return NULL;
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE], username[50];
    pthread_t thread;

    printf("=== Chat Client (macOS/Linux) ===\n");
    printf("Enter username: ");
    fgets(username, 50, stdin);
    username[strcspn(username, "\r\n")] = 0;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        return 1;
    }

    send(client_socket, username, strlen(username), 0);
    
    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

    pthread_create(&thread, NULL, ReceiveThread, &client_socket);

    while (1) {
        printf("> ");
        fflush(stdout);
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strcmp(buffer, "quit") == 0) break;
        send(client_socket, buffer, strlen(buffer), 0);
    }

    close(client_socket);
    return 0;
}
