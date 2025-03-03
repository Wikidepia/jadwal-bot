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

enum { 
    MAX_FIELDS = 32
};
json_t g_pool[ MAX_FIELDS ];
char *g_kota[] = {"KOTA SOLOK","KAB. BANYUMAS","KAB. GARUT","KAB. SUMBAWA BARAT","KAB. PONOROGO","KAB. PROBOLINGGO","KAB. BADUNG","KAB. SIMEULUE","KAB. SEMARANG","PEKAJANG KAB. LINGGA","KOTA DENPASAR","KOTA CIREBON","KAB. TANGGAMUS","KAB. BLORA","KAB. ASAHAN","KAB. KEBUMEN","KOTA SERANG","KAB. SAMOSIR","KOTA PASURUAN","KAB. BELITUNG TIMUR","KAB. MESUJI","KOTA PROBOLINGGO","KAB. WONOGIRI","KAB. JOMBANG","KOTA CIMAHI","KAB. GROBOGAN","KAB. PEMALANG","KAB. OGAN ILIR","KAB. LOMBOK BARAT","KOTA SURABAYA","KAB. DAIRI","KAB. BOGOR","KAB. KUANTAN SINGINGI","KAB. BENGKULU UTARA","KAB. GAYO LUES","KAB. SAROLANGUN","KAB. PIDIE","KAB. BONDOWOSO","KAB. PENUKAL ABAB LEMATANG ILIR","KAB. TIMOR TENGAH SELATAN","KAB. PEKALONGAN","KAB. SUKOHARJO","KAB. LAMPUNG TIMUR","KAB. TANJUNG JABUNG TIMUR","KOTA BOGOR","KOTA TEGAL","KAB. OGAN KOMERING ILIR","KAB. GIANYAR","KOTA BINJAI","KAB. ACEH TAMIANG","KAB. TANAH DATAR","KAB. BENGKALIS","KAB. TOBA SAMOSIR","KAB. SUMBA BARAT","KOTA SURAKARTA","KAB. NIAS SELATAN","KAB. PURWOREJO","KAB. ALOR","KAB. INDRAGIRI HULU","KAB. LIMA PULUH KOTA","KAB. BANGKA BARAT","KOTA MEDAN","KOTA KUPANG","KAB. PURBALINGGA","KOTA TANGERANG SELATAN","KAB. BLITAR","KOTA LUBUKLINGGAU","KAB. FLORES TIMUR","KAB. ACEH TIMUR","KOTA PALEMBANG","KAB. PESAWARAN","KAB. LAMPUNG TENGAH","KAB. MANGGARAI TIMUR","KAB. BANGKA","KAB. PESISIR SELATAN","KOTA PEKALONGAN","KOTA MATARAM","KAB. CIANJUR","KOTA SAWAHLUNTO","KAB. HUMBANG HASUNDUTAN","KAB. LUMAJANG","KAB. LEBAK","KAB. ROKAN HULU","KAB. EMPAT LAWANG","KAB. PADANG PARIAMAN","KAB. CIREBON","KAB. WAY KANAN","KAB. MUSI RAWAS","KOTA PADANGSIDEMPUAN","KAB. SITUBONDO","KAB. JEMBER","KAB. TRENGGALEK","KAB. SELUMA","KAB. KARO","KAB. BATANGHARI","KAB. DEMAK","KOTA MADIUN","KAB. PAMEKASAN","KAB. INDRAGIRI HILIR","KAB. SIJUNJUNG","KAB. NGADA","KAB. PASAMAN","KOTA PEMATANGSIANTAR","KAB. LAHAT","KOTA JAKARTA","KOTA LANGSA","KOTA BANJAR","KAB. KEDIRI","KAB. ROTE NDAO","KAB. MAGELANG","KAB. TAPANULI TENGAH","KAB. NIAS UTARA","KOTA SUBULUSSALAM","KAB. PASURUAN","KAB. BELITUNG","KAB. SAMPANG","KAB. ACEH JAYA","KAB. OGAN KOMERING ULU","KAB. BINTAN","KAB. ACEH TENGGARA","KAB. CILACAP","KAB. BULELENG","KAB. PAKPAK BHARAT","KOTA TANGERANG","KAB. TAPANULI UTARA","KAB. LOMBOK TIMUR","KOTA BATU","KAB. KERINCI","KAB. PADANG LAWAS","KAB. ACEH SELATAN","KAB. TEMANGGUNG","KOTA TANJUNGBALAI","KOTA SALATIGA","KOTA METRO","KAB. LANGKAT","KOTA SABANG","KAB. TAPANULI SELATAN","KAB. MUSI BANYUASIN","KAB. LAMPUNG BARAT","KAB. BANYUASIN","KAB. SUMBA TIMUR","KAB. BOYOLALI","KAB. KEPULAUAN ANAMBAS","PULAU SERASAN KAB. NATUNA","KAB. PATI","KOTA BENGKULU","KOTA PADANGPANJANG","KAB. CIAMIS","KAB. BENER MERIAH","KAB. SUMENEP","KAB. NAGAN RAYA","KAB. TUBAN","KAB. SIMALUNGUN","KAB. MANGGARAI BARAT","PULAU TAMBELAN KAB. BINTAN","KAB. TULUNGAGUNG","KAB. KEPULAUAN MERANTI","KAB. BANGLI","KAB. TEGAL","KAB. KAMPAR","KAB. KUNINGAN","KAB. SIAK","KAB. LAMPUNG UTARA","KAB. NATUNA","KAB. BANGKALAN","KAB. KARANGASEM","KOTA KEDIRI","KOTA MAGELANG","KOTA DUMAI","KAB. SERDANG BEDAGAI","KAB. TULANG BAWANG BARAT","KAB. LEMBATA","KAB. MUKOMUKO","KAB. KULON PROGO","KAB. SIKKA","KAB. WONOSOBO","KAB. BANJARNEGARA","KAB. KEPAHIANG","KAB. PADANG LAWAS UTARA","KAB. OGAN KOMERING ULU TIMUR","KAB. KAUR","KOTA PAGAR ALAM","KAB. NGAWI","KAB. REJANG LEBONG","KAB. PESISIR BARAT","KOTA BUKITTINGGI","KAB. SOLOK","KOTA TASIKMALAYA","KAB. PACITAN","KOTA DEPOK","KAB. ACEH TENGAH","KAB. TANJUNG JABUNG BARAT","KAB. KARANGANYAR","KOTA SIBOLGA","KAB. SABU RAIJUA","KAB. BEKASI","KAB. MUARA ENIM","KAB. KUPANG","KAB. SUMBA BARAT DAYA","KAB. ACEH BARAT DAYA","KAB. SOLOK SELATAN","KAB. DHARMASRAYA","KOTA MALANG","KAB. KEPULAUAN MENTAWAI","KOTA BIMA","PULAU LAUT KAB. NATUNA","KAB. PELALAWAN","KAB. LINGGA","KAB. BANGKA SELATAN","KAB. LAMONGAN","KAB. LOMBOK TENGAH","KAB. PRINGSEWU","KAB. MOJOKERTO","KAB. KUDUS","KAB. PANDEGLANG","KAB. SLEMAN","KAB. BIMA","KAB. KLUNGKUNG","KAB. MAGETAN","KAB. MANGGARAI","KAB. MUARO JAMBI","KAB. BANDUNG","KAB. SUBANG","KAB. ACEH BARAT","KAB. LAMPUNG SELATAN","PULAU MIDAI KAB. NATUNA","KAB. MADIUN","KOTA TEBING TINGGI","KAB. GUNUNGKIDUL","KAB. ACEH SINGKIL","KAB. DELI SERDANG","KAB. KARIMUN","KOTA BANDUNG","KOTA PADANG","KAB. BANYUWANGI","KAB. LOMBOK UTARA","KAB. REMBANG","KAB. LABUHANBATU UTARA","KAB. NIAS","KOTA PEKANBARU","KAB. KENDAL","KAB. BIREUEN","KAB. PURWAKARTA","KOTA BLITAR","KAB. MALAKA","KAB. MUSI RAWAS UTARA","KOTA GUNUNGSITOLI","KAB. SERANG","KAB. LEBONG","KAB. PANGANDARAN","KAB. BOJONEGORO","KAB. TEBO","KAB. BREBES","KAB. LABUHANBATU SELATAN","KAB. ADMINISTRASI KEPULAUAN SERIBU","KAB. JEMBRANA","KAB. SIDOARJO","KAB. INDRAMAYU","KAB. MALANG","KAB. KARAWANG","KAB. TABANAN","KOTA CILEGON","KOTA PRABUMULIH","KAB. JEPARA","KAB. NIAS BARAT","KAB. ACEH BESAR","KAB. KLATEN","KOTA MOJOKERTO","KAB. NAGEKEO","KAB. PASAMAN BARAT","KAB. LABUHANBATU","KAB. AGAM","KAB. BANGKA TENGAH","KAB. MAJALENGKA","KAB. TANGERANG","KOTA PARIAMAN","KOTA PANGKAL PINANG","KAB. SUKABUMI","KAB. BATU BARA","KAB. BANTUL","KAB. TULANG BAWANG","KOTA SUNGAI PENUH","KAB. GRESIK","KAB. ACEH UTARA","KAB. SUMBA TENGAH","KAB. SUMEDANG","KOTA PAYAKUMBUH","KAB. ENDE","KAB. MERANGIN","KAB. BUNGO","KAB. TASIKMALAYA","KOTA TANJUNG PINANG","KAB. BENGKULU SELATAN","KAB. NGANJUK","KAB. BATANG","KAB. SUMBAWA","KOTA BATAM","KAB. OGAN KOMERING ULU SELATAN","KAB. BANDUNG BARAT","KOTA BANDAR LAMPUNG","KOTA SEMARANG","KOTA SUKABUMI","KAB. MANDAILING NATAL","KAB. BENGKULU TENGAH","KAB. DOMPU","KOTA YOGYAKARTA","KAB. TIMOR TENGAH UTARA","KOTA JAMBI","KOTA BEKASI","KOTA LHOKSEUMAWE","KOTA BANDA ACEH","KAB. ROKAN HILIR","KAB. PIDIE JAYA","KAB. SRAGEN",};
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

void send_response(int socket, const char* content_type, const char* body) {
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

    // HTTP response
    char *http_response = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/html\r\n"
                          "Content-Length: 22\r\n\r\n"
                          "<h1>Hello, World!</h1>";

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
            json_t const* parent = json_create( body, g_pool, MAX_FIELDS );
            if ( parent == NULL ) return 1;

            json_t const* inline_query = json_getProperty( parent, "inline_query" );
            if ( !inline_query ) {
                puts("Error, the last name property is not found.");
                return 1;
            }

            const char* query = json_getPropertyValue( inline_query, "query" );
            if ( !query ) {
                puts("Error, the last name property is not found.");
                return 1;
            }

            const char* inline_query_id = json_getPropertyValue( inline_query, "id" );
            if ( !inline_query_id ) {
                puts("Error, the last name property is not found.");
                return 1;
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
                    for (int i = 0; i < sizeof(g_kota)/sizeof(char*); i++) {
                        char *entry = g_kota[i];
                        int32_t score = fuzzy_match(query, entry);
                        if (score < 0) continue;
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
                    int cc;
                    for (cc = 0; cc < 3; cc++) {
                        if (big_score[cc] == 0) continue;
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
