#include "server.h"

// Basic manual JSON parsing utility for extracting specific string fields
// Not meant as fully featured JSON parser, purely for flat single-level string key-value json requests.
void extract_json_string(const char* json, const char* key, char* out) {
    char search_key[100];
    sprintf(search_key, "\"%s\":\"", key);
    char* start = strstr(json, search_key);
    if (!start) {
        // Try exact numeric unquoted mapping
        sprintf(search_key, "\"%s\":", key);
        start = strstr(json, search_key);
        if(!start) {
            out[0] = '\0';
            return;
        }
        start += strlen(search_key);
        while(*start == ' ' || *start == '"') start++;
    } else {
        start += strlen(search_key);
    }
    
    int i = 0;
    while(start[i] != '\0' && start[i] != '"' && start[i] != ',' && start[i] != '}' && start[i] != '\n' && start[i] != '\r') {
        out[i] = start[i];
        i++;
    }
    out[i] = '\0';
    
    // Simple sanitization - strip | which is our delimiter
    for(int j=0; out[j] != '\0'; j++) {
        if(out[j] == '|') out[j] = ' ';
    }
}

void process_booking(SOCKET client_socket, char* body) {
    BookingData booking = {"", "", "", "", "", 0, ""};

    extract_json_string(body, "name", booking.name);
    extract_json_string(body, "phone", booking.phone);
    extract_json_string(body, "from", booking.from);
    extract_json_string(body, "to", booking.to);
    extract_json_string(body, "date", booking.date);
    
    char seats_str[20];
    extract_json_string(body, "seats", seats_str);
    booking.seats = atoi(seats_str);

    if (strlen(booking.name) == 0 || strlen(booking.phone) < 10 || strlen(booking.from) == 0 || strlen(booking.to) == 0 || strlen(booking.date) == 0 || booking.seats <= 0) {
        char* response = "HTTP/1.1 400 Bad Request\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\n\r\n{\"status\":\"error\", \"message\":\"Invalid booking data\"}";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char booking_id[20];
    strftime(booking_id, 20, "T%Y%m%d%H%M%S", timeinfo);

    char filepath[200];
    sprintf(filepath, "tickets/%s.txt", booking_id);
    FILE* ticket_file = fopen(filepath, "w");
    if (ticket_file != NULL) {
        fprintf(ticket_file, "------------------------------\n");
        fprintf(ticket_file, "BUS TICKET\n");
        fprintf(ticket_file, "------------------------------\n");
        fprintf(ticket_file, "Booking ID: %s\n", booking_id);
        fprintf(ticket_file, "Name: %s\n", booking.name);
        fprintf(ticket_file, "Phone: %s\n", booking.phone);
        fprintf(ticket_file, "From: %s\n", booking.from);
        fprintf(ticket_file, "To: %s\n", booking.to);
        fprintf(ticket_file, "Date: %s\n", booking.date);
        fprintf(ticket_file, "Seats: %d\n", booking.seats);
        fprintf(ticket_file, "------------------------------\n");
        fclose(ticket_file);
    } // Errors ignored intentionally for basic implementation

    FILE* data_file = fopen("data/bookings.txt", "a");
    if (data_file != NULL) {
        fprintf(data_file, "%s|%s|%s|%s|%s|%s|%d\n", booking_id, booking.name, booking.phone, booking.from, booking.to, booking.date, booking.seats);
        fclose(data_file);
    }

    char response[1024];
    sprintf(response, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\n\r\n{\"bookingId\":\"%s\",\"name\":\"%s\",\"from\":\"%s\",\"to\":\"%s\",\"date\":\"%s\",\"seats\":\"%d\",\"ticketUrl\":\"/ticket?id=%s\"}", 
        booking_id, booking.name, booking.from, booking.to, booking.date, booking.seats, booking_id);
    send(client_socket, response, strlen(response), 0);
}

void get_ticket(SOCKET client_socket, char* query_string) {
    char key[50] = {0};
    char booking_id[50] = {0};
    if (query_string != NULL && strlen(query_string) > 0) {
        sscanf(query_string, "%[^=]=%s", key, booking_id);
    }

    if (strcmp(key, "id") == 0) {
        char filepath[200];
        sprintf(filepath, "tickets/%s.txt", booking_id);
        FILE* file = fopen(filepath, "rb"); // Explicit binary read
        
        if (file != NULL) {
            fseek(file, 0, SEEK_END);
            long fsize = ftell(file);
            fseek(file, 0, SEEK_SET);

            char* buffer = malloc(fsize + 1);
            if (buffer) {
                size_t read_bytes = fread(buffer, 1, fsize, file);
                buffer[read_bytes] = '\0';
                
                char header[256];
                sprintf(header, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\n\r\n");
                
                char* ticket_details = malloc(read_bytes * 2 + 1024);
                sprintf(ticket_details, "{\"status\":\"success\", \"ticket\":\"");
                
                int len = strlen(ticket_details);
                for(int i = 0; buffer[i] != '\0'; i++) {
                    if(buffer[i] == '\n') {
                        ticket_details[len++] = '\\';
                        ticket_details[len++] = 'n';
                    } else if(buffer[i] == '\r') {
                        // Skip carriage literal or we just do nothing
                    } else if(buffer[i] == '"') {
                        ticket_details[len++] = '\\';
                        ticket_details[len++] = '"';
                    } else {
                        ticket_details[len++] = buffer[i];
                    }
                }
                ticket_details[len++] = '"';
                ticket_details[len++] = '}';
                ticket_details[len++] = '\0';

                char* full_response = malloc(strlen(header) + strlen(ticket_details) + 1);
                if (full_response) {
                    sprintf(full_response, "%s%s", header, ticket_details);
                    send(client_socket, full_response, strlen(full_response), 0);
                    free(full_response);
                }
                
                free(ticket_details);
                free(buffer);
            }
            fclose(file);
        } else {
            char* response = "HTTP/1.1 404 Not Found\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\n\r\n{\"status\":\"error\", \"message\":\"Ticket not found\"}";
            send(client_socket, response, strlen(response), 0);
        }
    } else {
        char* response = "HTTP/1.1 400 Bad Request\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\n\r\n{\"status\":\"error\", \"message\":\"Missing ticket ID\"}";
        send(client_socket, response, strlen(response), 0);
    }
}
void list_bookings(SOCKET client_socket) {
    FILE* file = fopen("data/bookings.txt", "r");
    char* response_body = malloc(10000); // Plenty for demo
    strcpy(response_body, "{\"bookings\":[");

    if (file != NULL) {
        char line[512];
        int first = 1;
        while (fgets(line, sizeof(line), file)) {
            if (!first) strcat(response_body, ",");
            first = 0;

            char id[50], name[100], phone[20], from[100], to[100], date[20];
            int seats;
            sscanf(line, "%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%d", id, name, phone, from, to, date, &seats);

            char item[1024];
            sprintf(item, "{\"id\":\"%s\",\"name\":\"%s\",\"phone\":\"%s\",\"from\":\"%s\",\"to\":\"%s\",\"date\":\"%s\",\"seats\":%d}", id, name, phone, from, to, date, seats);
            strcat(response_body, item);
        }
        fclose(file);
    }

    strcat(response_body, "]}");

    char header[256];
    sprintf(header, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\n\r\n");

    send(client_socket, header, strlen(header), 0);
    send(client_socket, response_body, strlen(response_body), 0);
    free(response_body);
}
