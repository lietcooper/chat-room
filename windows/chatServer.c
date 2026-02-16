#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

int main() {
    WSADATA wsaData;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    SOCKET client_sockets[MAX_CLIENTS] = { 0 };
    char client_names[MAX_CLIENTS][50] = { 0 };
    int client_count = 0;

    printf("=== Chat Server (Windows) ===\n");
    printf("Initializing...\n");

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Start listening
    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Server started successfully, listening on port: %d\n", PORT);
    printf("Waiting for connections...\n\n");

    // Initialize client socket array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = INVALID_SOCKET;
    }

    // Main loop
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != INVALID_SOCKET) {
                FD_SET(client_sockets[i], &read_fds);
            }
        }

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(0, &read_fds, NULL, NULL, &timeout);

        if (activity == SOCKET_ERROR) {
            printf("Select error: %d\n", WSAGetLastError());
            break;
        }

        // Check for new connection
        if (FD_ISSET(server_socket, &read_fds)) {
            client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_socket == INVALID_SOCKET) {
                printf("Accept failed: %d\n", WSAGetLastError());
                continue;
            }

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
                printf("Server full, rejecting connection.\n");
                const char* msg = "Server is full, please try again later.\n";
                send(client_socket, msg, (int)strlen(msg), 0);
                closesocket(client_socket);
                continue;
            }

            // Receive username
            int name_len = recv(client_socket, client_names[client_index], sizeof(client_names[client_index]) - 1, 0);
            if (name_len > 0) {
                client_names[client_index][name_len] = '\0';
                // Remove newline characters
                client_names[client_index][strcspn(client_names[client_index], "\r\n")] = 0;
            }
            else {
                strcpy(client_names[client_index], "Anonymous");
            }

            printf("New client connected: %s (IP: %s)\n", client_names[client_index], inet_ntoa(client_addr.sin_addr));

            char welcome_msg[BUFFER_SIZE];
            _snprintf(welcome_msg, sizeof(welcome_msg), "Welcome %s! Online users: %d\n", client_names[client_index], client_count);
            send(client_socket, welcome_msg, (int)strlen(welcome_msg), 0);

            char join_msg[BUFFER_SIZE];
            _snprintf(join_msg, sizeof(join_msg), "System: %s has joined the chat\n", client_names[client_index]);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] != INVALID_SOCKET && i != client_index) {
                    send(client_sockets[i], join_msg, (int)strlen(join_msg), 0);
                }
            }
        }

        // Check for client messages
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == INVALID_SOCKET) continue;

            if (FD_ISSET(client_sockets[i], &read_fds)) {
                int bytes_received = recv(client_sockets[i], buffer, BUFFER_SIZE - 1, 0);

                if (bytes_received <= 0) {
                    printf("Client disconnected: %s\n", client_names[i]);
                    char leave_msg[BUFFER_SIZE];
                    _snprintf(leave_msg, sizeof(leave_msg), "System: %s has left the chat\n", client_names[i]);

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

                buffer[bytes_received] = '\0';
                buffer[strcspn(buffer, "\r\n")] = 0;

                printf("%s: %s\n", client_names[i], buffer);

                if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
                    const char* goodbye = "Goodbye!\n";
                    send(client_sockets[i], goodbye, (int)strlen(goodbye), 0);
                    closesocket(client_sockets[i]);
                    client_sockets[i] = INVALID_SOCKET;
                    client_count--;
                    continue;
                }

                char broadcast_msg[BUFFER_SIZE + 100];
                _snprintf(broadcast_msg, sizeof(broadcast_msg), "%s: %s\n", client_names[i], buffer);

                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (client_sockets[j] != INVALID_SOCKET && j != i) {
                        send(client_sockets[j], broadcast_msg, (int)strlen(broadcast_msg), 0);
                    }
                }
            }
        }
        Sleep(10);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
