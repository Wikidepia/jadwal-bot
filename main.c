#define _GNU_SOURCE
#include "fuzzy_match.h"
#include "json-build.h"
#include "main.h"
#include "tiny-json.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 4096

enum { MAX_FIELDS = 32 };
json_t g_pool[MAX_FIELDS];
jsonb b;

// Function to extract body from HTTP request
char *extract_body(char *request) {
  char *body_start = strstr(request, "\r\n\r\n");
  if (body_start) {
    return body_start + 4; // Skip the \r\n\r\n delimiter
  }
  return NULL; // No body found
}

void send_response(int socket, const char *content_type, const char *body) {
  char response[BUFFER_SIZE];
  sprintf(response,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: %s\r\n"
          "Content-Length: %lu\r\n\r\n"
          "%s",
          content_type, strlen(body), body);

  write(socket, response, strlen(response));
}

void minutesToHHMM(int minutes, char *timeStr) {
  // Calculate hours and remaining minutes
  int hours = minutes / 60;
  int mins = minutes % 60;

  // Format the time as hh:mm
  sprintf(timeStr, "%02d:%02d", hours, mins);
}

const char *get_weekday(int year, int month, int day) {
  /* using C99 compound literals in a single line: notice the splicing */
  return ((const char *[]){
      "Senin", "Selasa", "Rabu", "Kamis", "Jum'at", "Sabtu",
      "Ahad"})[(day + ((153 * (month + 12 * ((14 - month) / 12) - 3) + 2) / 5) +
                (365 * (year + 4800 - ((14 - month) / 12))) +
                ((year + 4800 - ((14 - month) / 12)) / 4) -
                ((year + 4800 - ((14 - month) / 12)) / 100) +
                ((year + 4800 - ((14 - month) / 12)) / 400) - 32045) %
               7];
}

int main() {
  int server_fd, new_socket;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[BUFFER_SIZE] = {0};

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    return 1;
  }

  // Set socket options
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("setsockopt failed");
    return 1;
  }

  // Configure server address
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Bind socket to the port
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    return 1;
  }

  // Listen for connections
  if (listen(server_fd, 10) < 0) {
    perror("listen failed");
    return 1;
  }

  printf("HTTP Server running on port %d...\n", PORT);

  while (1) {
    printf("Waiting for connections...\n");

    // Accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
      perror("accept failed");
      return 1;
    }

    // Read client request
    read(new_socket, buffer, BUFFER_SIZE);
    char *body = extract_body(buffer);
    char buf[2048] = {0};
    if (body && strlen(body) > 0) {
      const json_t *parent = json_create(body, g_pool, MAX_FIELDS);
      if (parent == NULL) {
        close(new_socket);
        continue;
      }

      const json_t *inline_query = json_getProperty(parent, "inline_query");
      if (!inline_query) {
        puts("no inline_query found.");
        close(new_socket);
        continue;
      }

      const char *query = json_getPropertyValue(inline_query, "query");
      if (!query) {
        puts("no query found.");
        close(new_socket);
        continue;
      }

      const char *inline_query_id = json_getPropertyValue(inline_query, "id");
      if (!inline_query_id) {
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
        jsonb_string(&b, buf, sizeof(buf), "answerInlineQuery",
                     strlen("answerInlineQuery"));
        jsonb_key(&b, buf, sizeof(buf), "inline_query_id",
                  strlen("inline_query_id"));
        jsonb_string(&b, buf, sizeof(buf), inline_query_id,
                     strlen(inline_query_id));
        jsonb_key(&b, buf, sizeof(buf), "results", strlen("results"));
        jsonb_array(&b, buf, sizeof(buf));
        {
          int big_score[3] = {0};
          int big_index[3] = {0};
          for (int i = 0; i < sizeof(g_kota) / sizeof(char *); i++) {
            const char *entry = g_kota[i];
            int32_t score = fuzzy_match(query, entry);
            if (score < 0)
              continue;
            if (score > big_score[0]) {
              big_score[2] = big_score[1];
              big_score[1] = big_score[0];
              big_score[0] = score;
              big_index[2] = big_index[1];
              big_index[1] = big_index[0];
              big_index[0] = i;
            } else if (score > big_score[1]) {
              big_score[2] = big_score[1];
              big_score[1] = score;
              big_index[2] = big_index[1];
              big_index[1] = i;
            } else if (score > big_score[2]) {
              big_score[2] = score;
              big_index[2] = i;
            }
          }

          int k;
          for (k = 0; k < 3; k++) {
            if (big_score[k] == 0)
              continue;
            char text[1024];
            int text_length = 0;
            int cur_idx = big_index[k];

            char *header = "*Jadwal Sholat Untuk Wilayah ";
            text_length += snprintf(text + text_length,
                                    sizeof(text) - text_length, "%s", header);
            text_length +=
                snprintf(text + text_length, sizeof(text) - text_length, "%s\n",
                         g_kota[cur_idx]);
            text_length += snprintf(text + text_length,
                                    sizeof(text) - text_length, "*\n\n```\n");

            int col_idx;
            const int baseMinutes[] = {0,   240, 300,  360, 420,
                                       720, 900, 1080, 1140};
            // Indices:       0, 1,   2,   3,   4,   5,   6,   7,    8
            // Prayer Names:  --, Imsak, Subuh, Terbit, Dhuha, Dzuhur, Ashar,
            // Maghrib, Isya

            for (col_idx = 0; col_idx < 9; col_idx++) {
              char valueStr[32];
              if (col_idx == 0) {
                snprintf(valueStr, sizeof(valueStr), "%s, %02d/%02d/%d",
                         get_weekday(2025, 3, day_idx + 1), day_idx + 1, 3,
                         2025);
              } else {
                minutesToHHMM(baseMinutes[col_idx] +
                                  (int)g_jadwal[cur_idx][day_idx][col_idx],
                              valueStr);
              }
              text_length +=
                  snprintf(text + text_length, sizeof(text) - text_length,
                           "%s%s\n", g_colname[col_idx], valueStr);
            }

            text_length += snprintf(text + text_length,
                                    sizeof(text) - text_length, "```\n");

            jsonb_object(&b, buf, sizeof(buf));
            {
              jsonb_key(&b, buf, sizeof(buf), "type", strlen("type"));
              jsonb_string(&b, buf, sizeof(buf), "article", strlen("article"));
              jsonb_key(&b, buf, sizeof(buf), "id", strlen("id"));
              jsonb_number(&b, buf, sizeof(buf), cur_idx);
              jsonb_key(&b, buf, sizeof(buf), "title", strlen("title"));
              jsonb_string(&b, buf, sizeof(buf), g_kota[cur_idx],
                           strlen(g_kota[cur_idx]));
              jsonb_key(&b, buf, sizeof(buf), "input_message_content",
                        strlen("input_message_content"));
              jsonb_object(&b, buf, sizeof(buf));
              {
                jsonb_key(&b, buf, sizeof(buf), "message_text",
                          strlen("message_text"));
                jsonb_string(&b, buf, sizeof(buf), text, strlen(text));
                jsonb_key(&b, buf, sizeof(buf), "parse_mode",
                          strlen("parse_mode"));
                jsonb_string(&b, buf, sizeof(buf), "Markdown",
                             strlen("Markdown"));
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
      send_response(new_socket, "application/json", buf);
    }

    // Close the connection
    close(new_socket);
  }

  return 0;
}
