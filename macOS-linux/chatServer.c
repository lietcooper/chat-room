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

    printf("=== Chat Server (macOS/Linux) ===\n");
    printf("Initializing...\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    listen(server_socket, MAX_CLIENTS);
    printf("Server listening on port %d\n", PORT);

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

        select(max_sd + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(server_socket, &read_fds)) {
            client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
            int index = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == -1) {
                    client_sockets[i] = client_socket;
                    index = i;
                    break;
                }
            }

            int n = recv(client_socket, client_names[index], 49, 0);
            if (n > 0) {
                client_names[index][n] = '\0';
                client_names[index][strcspn(client_names[index], "\r\n")] = 0;
            } else {
                strcpy(client_names[index], "Anonymous");
            }

            printf("New connection: %s\n", client_names[index]);
            char welcome[BUFFER_SIZE];
            sprintf(welcome, "Welcome %s!\n", client_names[index]);
            send(client_socket, welcome, strlen(welcome), 0);
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != -1 && FD_ISSET(client_sockets[i], &read_fds)) {
                int n = recv(client_sockets[i], buffer, BUFFER_SIZE - 1, 0);
                if (n <= 0) {
                    printf("%s disconnected\n", client_names[i]);
                    close(client_sockets[i]);
                    client_sockets[i] = -1;
                } else {
                    buffer[n] = '\0';
                    buffer[strcspn(buffer, "\r\n")] = 0;
                    printf("%s: %s\n", client_names[i], buffer);

                    char msg[BUFFER_SIZE + 100];
                    sprintf(msg, "%s: %s\n", client_names[i], buffer);
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_sockets[j] != -1 && j != i) {
                            send(client_sockets[j], msg, strlen(msg), 0);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
