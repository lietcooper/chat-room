#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

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
            printf("\nDisconnected from server\n");
            break;
        }
        else {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                break;
            }
        }
        Sleep(100);
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char username[50];
    HANDLE receive_thread;

    printf("=== Chat Client (Windows) ===\n\n");
    printf("Enter username: ");
    fflush(stdout);

    if (fgets(username, sizeof(username), stdin) == NULL) return 1;
    username[strcspn(username, "\r\n")] = 0;

    if (strlen(username) == 0) {
        strcpy(username, "Anonymous");
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connection failed\n");
        WSACleanup();
        return 1;
    }

    send(client_socket, username, (int)strlen(username), 0);
    printf("Connected! Type 'quit' to exit.\n\n");

    u_long mode = 1;
    ioctlsocket(client_socket, FIONBIO, &mode);

    receive_thread = (HANDLE)_beginthreadex(NULL, 0, ReceiveThread, &client_socket, 0, NULL);

    while (1) {
        printf("> ");
        fflush(stdout);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strlen(buffer) == 0) continue;

        if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
            send(client_socket, buffer, (int)strlen(buffer), 0);
            break;
        }

        send(client_socket, buffer, (int)strlen(buffer), 0);
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
