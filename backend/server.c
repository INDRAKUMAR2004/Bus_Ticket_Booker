#include "server.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

int main() {
    int port = 8080;
    char* env_port = getenv("PORT");
    if (env_port != NULL) {
        port = atoi(env_port);
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock.\n");
        return 1;
    }
#endif

    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
#ifdef _WIN32
    int client_addr_len = sizeof(client_addr);
#else
    socklen_t client_addr_len = sizeof(client_addr);
#endif

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket.\n");
        return 1;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        closesocket(server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    listen(server_socket, 10);
    printf("Server listening on port %d\n", port);

#ifdef _WIN32
    CreateDirectoryA("data", NULL);
    CreateDirectoryA("tickets", NULL);
#endif

// On Linux (Render), these folders need to exist or be created using POSIX mkdir:
#ifndef _WIN32
    system("mkdir -p data tickets");
#endif

    while ((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len)) != INVALID_SOCKET) {
        handle_client(client_socket);
    }

    closesocket(server_socket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    int recv_size;

    if ((recv_size = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) == SOCKET_ERROR) {
#ifdef _WIN32
        printf("recv failed: %d\n", WSAGetLastError());
#endif
        closesocket(client_socket);
        return;
    }

    if(recv_size > 0) {
        buffer[recv_size] = '\0';
#ifdef _WIN32
        char* request_copy = _strdup(buffer);
#else
        char* request_copy = strdup(buffer);
#endif
        if(request_copy) {
            route_request(client_socket, request_copy);
            free(request_copy);
        }
    }

    closesocket(client_socket);
}
