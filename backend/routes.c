#include "server.h"

void parse_body(char* request, char* body) {
    char* body_ptr = strstr(request, "\r\n\r\n");
    if (body_ptr) {
        strcpy(body, body_ptr + 4);
    } else {
        body[0] = '\0';
    }
}

void route_request(SOCKET client_socket, char* request) {
    char method[10] = {0};
    char path[255] = {0};
    char body[BUFFER_SIZE] = {0};
    char query_string[255] = {0};

    sscanf(request, "%s %s", method, path);
    parse_body(request, body);

    char* query = strchr(path, '?');
    if (query) {
        *query = '\0';
        strcpy(query_string, query + 1);
    }

    printf("Request: %s %s\n", method, path);

    // Handle Preflight OPTIONS for CORS (Crucial for Netlify -> Render interaction)
    if (strcmp(method, "OPTIONS") == 0) {
        char* response = "HTTP/1.1 204 No Content\r\n"
                         "Access-Control-Allow-Origin: *\r\n"
                         "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                         "Access-Control-Allow-Headers: Content-Type\r\n"
                         "Connection: close\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0) {
            char* response = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\n\r\n{\"status\":\"server running\", \"message\":\"Backend API is active!\"}";
            send(client_socket, response, strlen(response), 0);
        } else if (strcmp(path, "/booking") == 0) {
             list_bookings(client_socket);
        } else if (strcmp(path, "/ticket") == 0) {
             get_ticket(client_socket, query_string);
        } else {
            char* response = "HTTP/1.1 404 Not Found\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Not Found\"}";
            send(client_socket, response, strlen(response), 0);
        }
    } else if (strcmp(method, "POST") == 0) {
        if (strcmp(path, "/book") == 0) {
            process_booking(client_socket, body);
        } else {
             char* response = "HTTP/1.1 404 Not Found\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Not Found\"}";
             send(client_socket, response, strlen(response), 0);
        }
    } else {
        char* response = "HTTP/1.1 405 Method Not Allowed\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
    }
}
