#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

#define BUFFER_SIZE 4096

typedef struct {
    char name[100];
    char phone[20];
    char from[100];
    char to[100];
    char date[20];
    int seats;
    char seat_numbers[100];
} BookingData;

// Function prototypes
void handle_client(SOCKET client_socket);
void route_request(SOCKET client_socket, char* request);
void serve_file(SOCKET client_socket, const char* filepath, const char* content_type);
void process_booking(SOCKET client_socket, char* body);
void get_ticket(SOCKET client_socket, char* query_string);
void list_bookings(SOCKET client_socket);
void extract_json_string(const char* json, const char* key, char* out);

#endif
