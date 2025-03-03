#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <time.h>
#include "tiny-json.h"
#include "fuzzy_match.h"
#include "main.h"
#include "json-build.h"

#define PORT 8080
#define BUFFER_SIZE 4096

enum
{
    MAX_FIELDS = 32
};
json_t g_pool[MAX_FIELDS];
jsonb b;

// Function to extract body from HTTP request
char *extract_body(char *request)
{
    char *body_start = strstr(request, "\r\n\r\n");
    if (body_start)
    {
        return body_start + 4; // Skip the \r\n\r\n delimiter
    }
    return NULL; // No body found
}

void send_response(int socket, const char *content_type, const char *body)
{
    char response[BUFFER_SIZE];
    sprintf(response,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %lu\r\n\r\n"
            "%s",
            content_type, strlen(body), body);

    write(socket, response, strlen(response));
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        return 1;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        return 1;
    }

    // Configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        return 1;
    }

    // Listen for connections
    if (listen(server_fd, 10) < 0)
    {
        perror("listen failed");
        return 1;
    }

    printf("HTTP Server running on port %d...\n", PORT);

    while (1)
    {
        printf("Waiting for connections...\n");

        // Accept incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept failed");
            return 1;
        }

        // Read client request
        read(new_socket, buffer, BUFFER_SIZE);
        char *body = extract_body(buffer);
        char buf[2048];
        if (body && strlen(body) > 0)
        {
            printf("Extracted body:\n%s\n", body);
            json_t const *parent = json_create(body, g_pool, MAX_FIELDS);
            if (parent == NULL)
                close(new_socket);
                continue;

            json_t const *inline_query = json_getProperty(parent, "inline_query");
            if (!inline_query)
            {
                puts("Error, the last name property is not found.");
                close(new_socket);
                continue;
            }

            const char *query = json_getPropertyValue(inline_query, "query");
            if (!query)
            {
                puts("Error, the last name property is not found.");
                close(new_socket);
                continue;
            }

            const char *inline_query_id = json_getPropertyValue(inline_query, "id");
            if (!inline_query_id)
            {
                puts("Error, the last name property is not found.");
                close(new_socket);
                continue;
            }

            // Get current time
            time_t now = time(NULL);
            time_t gmt7_seconds = now + (7 * 3600); // Add 7 hours in seconds
            struct tm *gmt7_time = gmtime(&gmt7_seconds);

            // Extract day of month (1-31)
            int day_idx = (gmt7_time->tm_mday) - 1;

            jsonb_init(&b);
            jsonb_object(&b, buf, sizeof(buf));
            {
                jsonb_key(&b, buf, sizeof(buf), "method", strlen("method"));
                jsonb_string(&b, buf, sizeof(buf), "answerInlineQuery", strlen("answerInlineQuery"));
                jsonb_key(&b, buf, sizeof(buf), "inline_query_id", strlen("inline_query_id"));
                jsonb_string(&b, buf, sizeof(buf), inline_query_id, strlen(inline_query_id));
                jsonb_key(&b, buf, sizeof(buf), "results", strlen("results"));
                jsonb_array(&b, buf, sizeof(buf));
                {
                    int big_score[3] = {0};
                    int big_index[3] = {0};
                    for (int i = 0; i < sizeof(g_kota) / sizeof(char *); i++)
                    {
                        char *entry = g_kota[i];
                        int32_t score = fuzzy_match(query, entry);
                        if (score < 0)
                            continue;
                        if (score > big_score[0])
                        {
                            big_score[2] = big_score[1];
                            big_score[1] = big_score[0];
                            big_score[0] = score;
                            big_index[2] = big_index[1];
                            big_index[1] = big_index[0];
                            big_index[0] = i;
                        }
                        else if (score > big_score[1])
                        {
                            big_score[2] = big_score[1];
                            big_score[1] = score;
                            big_index[2] = big_index[1];
                            big_index[1] = i;
                        }
                        else if (score > big_score[2])
                        {
                            big_score[2] = score;
                            big_index[2] = i;
                        }
                    }
                    int cc;
                    for (cc = 0; cc < 3; cc++)
                    {
                        if (big_score[cc] == 0)
                            continue;
                        char text[10240];
                        int old_length = 0;

                        char *header = "*Jadwal Sholat Untuk Wilayah ";
                        int bc;
                        for (bc = 0; bc < strlen(header); bc++)
                        {
                            text[old_length] = header[bc];
                            old_length += 1;
                        }
                        for (bc = 0; bc < strlen(g_kota[big_index[cc]]); bc++)
                        {
                            text[old_length] = g_kota[big_index[cc]][bc];
                            old_length += 1;
                        }
                        text[old_length] = '*';
                        old_length += 1;
                        text[old_length] = '\n';
                        old_length += 1;
                        text[old_length] = '\n';
                        old_length += 1;

                        text[old_length] = '`';
                        old_length += 1;
                        text[old_length] = '`';
                        old_length += 1;
                        text[old_length] = '`';
                        old_length += 1;
                        text[old_length] = '\n';
                        old_length += 1;
                        for (int xb = 0; xb < 9; xb++)
                        {
                            int bb;
                            for (bb = 0; bb < strlen(g_colname[xb]); bb++)
                            {
                                text[old_length] = g_colname[xb][bb];
                                old_length += 1;
                            }
                            for (bb = 0; bb < strlen(g_jadwal[big_index[cc]][day_idx][xb]); bb++)
                            {
                                text[old_length] = g_jadwal[big_index[cc]][day_idx][xb][bb];
                                old_length += 1;
                            }
                            text[old_length] = '\n';
                            old_length += 1;
                        }
                        text[old_length] = '`';
                        old_length += 1;
                        text[old_length] = '`';
                        old_length += 1;
                        text[old_length] = '`';
                        old_length += 1;
                        text[old_length] = '\0';

                        jsonb_object(&b, buf, sizeof(buf));
                        {
                            jsonb_key(&b, buf, sizeof(buf), "type", strlen("type"));
                            jsonb_string(&b, buf, sizeof(buf), "article", strlen("article"));
                            jsonb_key(&b, buf, sizeof(buf), "id", strlen("id"));
                            jsonb_number(&b, buf, sizeof(buf), big_index[cc]);
                            jsonb_key(&b, buf, sizeof(buf), "title", strlen("title"));
                            jsonb_string(&b, buf, sizeof(buf), g_kota[big_index[cc]], strlen(g_kota[big_index[cc]]));
                            jsonb_key(&b, buf, sizeof(buf), "input_message_content", strlen("input_message_content"));
                            jsonb_object(&b, buf, sizeof(buf));
                            {
                                jsonb_key(&b, buf, sizeof(buf), "message_text", strlen("message_text"));
                                jsonb_string(&b, buf, sizeof(buf), text, strlen(text));
                                jsonb_key(&b, buf, sizeof(buf), "parse_mode", strlen("parse_mode"));
                                jsonb_string(&b, buf, sizeof(buf), "Markdown", strlen("Markdown"));
                                jsonb_object_pop(&b, buf, sizeof(buf));
                            }
                            jsonb_object_pop(&b, buf, sizeof(buf));
                        }
                    }
                    jsonb_array_pop(&b, buf, sizeof(buf));
                }
                jsonb_object_pop(&b, buf, sizeof(buf));
            }

            printf("JSON: %s\n", buf);
        } // End of body check

        // Send HTTP response
        send_response(new_socket, "application/json", buf);
        printf("Response sent.\n");

        // Close the connection
        close(new_socket);
    }

    return 0;
}
