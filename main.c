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
            json_t *parent = json_create(body, g_pool, MAX_FIELDS);
            if (parent == NULL) {
                close(new_socket);
                continue;
            }

            json_t *inline_query = json_getProperty(parent, "inline_query");
            if (!inline_query)
            {
                puts("no inline_query found.");
                close(new_socket);
                continue;
            }

            char *query = json_getPropertyValue(inline_query, "query");
            if (!query)
            {
                puts("no query found.");
                close(new_socket);
                continue;
            }

            char *inline_query_id = json_getPropertyValue(inline_query, "id");
            if (!inline_query_id)
            {
                puts("no query_id found.");
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

                    int k;
                    for (k = 0; k < 3; k++)
                    {
                        if (big_score[k] == 0)
                            continue;
                        char text[1024];
                        int text_length = 0;
                        int cur_idx = big_index[k];

                        int hidx;
                        char *header = "*Jadwal Sholat Untuk Wilayah ";
                        for (hidx = 0; hidx < strlen(header); hidx++)
                        {
                            text[text_length] = header[hidx];
                            text_length += 1;
                        }
                        for (hidx = 0; hidx < strlen(g_kota[cur_idx]); hidx++)
                        {
                            text[text_length] = g_kota[cur_idx][hidx];
                            text_length += 1;
                        }

                        text[text_length] = '*';
                        text_length += 1;
                        text[text_length] = '\n';
                        text_length += 1;
                        text[text_length] = '\n';
                        text_length += 1;

                        text[text_length] = '`';
                        text_length += 1;
                        text[text_length] = '`';
                        text_length += 1;
                        text[text_length] = '`';
                        text_length += 1;
                        text[text_length] = '\n';
                        text_length += 1;

                        int col_idx;
                        for (col_idx = 0; col_idx < 9; col_idx++)
                        {
                            // Write column name
                            int tidx;
                            for (tidx = 0; tidx < strlen(g_colname[col_idx]); tidx++)
                            {
                                text[text_length] = g_colname[col_idx][tidx];
                                text_length += 1;
                            }

                            // Write data
                            for (tidx = 0; tidx < strlen(g_jadwal[cur_idx][day_idx][col_idx]); tidx++)
                            {
                                text[text_length] = g_jadwal[cur_idx][day_idx][col_idx][tidx];
                                text_length += 1;
                            }
                            text[text_length] = '\n';
                            text_length += 1;
                        }

                        text[text_length] = '`';
                        text_length += 1;
                        text[text_length] = '`';
                        text_length += 1;
                        text[text_length] = '`';
                        text_length += 1;
                        text[text_length] = '\0';

                        jsonb_object(&b, buf, sizeof(buf));
                        {
                            jsonb_key(&b, buf, sizeof(buf), "type", strlen("type"));
                            jsonb_string(&b, buf, sizeof(buf), "article", strlen("article"));
                            jsonb_key(&b, buf, sizeof(buf), "id", strlen("id"));
                            jsonb_number(&b, buf, sizeof(buf), cur_idx);
                            jsonb_key(&b, buf, sizeof(buf), "title", strlen("title"));
                            jsonb_string(&b, buf, sizeof(buf), g_kota[cur_idx], strlen(g_kota[cur_idx]));
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
