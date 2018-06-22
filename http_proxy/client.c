#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <strings.h>
#include <stdbool.h>
#include "parse_url.h"


#define SIZE 1024


static void edit_request_proxy (char protocol[], char host[],
                int port, char path[], char *request) {

    char *method = "GET";
    char *version = "HTTP/1.1";

    if (port == 0) {
        snprintf(request, sizeof(char)*300,
            "%s %s://%s%s %s\r\n"
            "Host: %s\r\n"
            "Proxy-Connection: close\r\n\r\n",
            method, protocol, host, path, version,
            host);


    } else {
        snprintf(request, sizeof(char)*200,
            "%s %s://%s:%d%s %s\r\n"
            "Host: %s\r\n"
            "Proxy-Connection: close\r\n\r\n",
            method, protocol, host, port, path, version,
            host);
    }

}

static void edit_request_no_proxy (char protocol[], char host[],
                int port, char path[], char *request) {
    char *method = "GET";
    char *version = "HTTP/1.1";

    if (port == 0) {
        snprintf(request, sizeof(char*)*200,
            "%s %s %s\r\n"
            "Host: %s\r\n\r\n",
            method, path, version,
            host);
    } else {
        snprintf(request, sizeof(char*)*200,
            "%s %s %s\r\n"
            "Host: %s:%d\r\n\r\n",
            method, path, version,
            host, port);
    }

    printf("request: %s\n", request);

}

static int host_to_ip (const char *address, char *ip) {
    struct addrinfo hints, *servinfo, *rp;
    int s;
    struct sockaddr_in *ser_add;

    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((s = getaddrinfo(address, "http", &hints, &servinfo)) != 0) {
        perror("Error");
        return 0;
    }

    rp = servinfo;
    if (rp != NULL) {
        ser_add = (struct sockaddr_in *) rp->ai_addr;
        snprintf(ip, sizeof(char*)*100, "%s",inet_ntoa(ser_add->sin_addr));
    }

    freeaddrinfo(servinfo);
    return 1;
}

static void strtolower(char *str) {

    int i;

    for(i = 0; str[i]; i++){
        str[i] = tolower(str[i]);
    }

}

static int read_http_header_2_file(FILE *f, int *body_len) {
    char buf[1024];
    int get_ok = 0;
    unsigned int content_length = 0;

    int rc = 1;

    FILE *fheader_file;

    fheader_file = fopen("header.txt", "w");

    if (fheader_file == NULL) {
        fprintf(stderr, "cannot open file header.txt\n");
        return -1;
    }

    do {

        if (fgets(buf, sizeof(buf), f) == NULL) {
            fprintf(stderr, "cannot read from http response\n");
            rc = -1;
            break;
        }

        printf("%s", buf);

        // the first line
        if (get_ok == 0) {

            if (strstr(buf, "200") == NULL) {
                fprintf(stderr, "http response not OK(200)\n");
                rc = -1;
                goto out_read_http_header_2_file;
            }

            get_ok = 1;

        } else {

            strtolower(buf);

            if (strncmp(buf, "content-length", strlen("content-length")) == 0 ) {

                if (sscanf(buf, "content-length: %u", &content_length) == 1) {
                    *body_len = content_length;
                }

            } else {

                if (strncmp(buf, "\r\n", 2) == 0) {

                    if (fwrite(buf, strlen(buf), 1, fheader_file) < 0) {
                        fprintf(stderr, "Error write the end to header file\n");
                    }

                    goto out_read_http_header_2_file;
                }
            }
        }

        if (fwrite(buf, strlen(buf), 1, fheader_file) <= 0) {
            fprintf(stderr, "Error write to header file\n");
        }

    } while (1);

out_read_http_header_2_file:

    fclose(fheader_file);
    return rc;
}

static int read_http_body_2_file(FILE *f, int body_len) {

    char buf[1024];
    int rc = 1;
    int total_bytes = 0;
    int n;

    FILE *fbody_file;

    fbody_file = fopen("body.dat", "w");

    if (fbody_file == NULL) {
        fprintf(stderr, "cannot open file body.dat\n");
        return -1;
    }

    printf("body_len: %d\n", body_len);

    do {
        if ((n =fread(buf, 1, 1024 , f)) <= 0) {
            if (n < 0) {
                rc = -1;
            }
            goto out_read_http_body_2_file;
        }

        if (fwrite(buf, n, 1, fbody_file) <= 0) {
            fprintf(stderr, "Error write to body file\n");
        }

        total_bytes += n;

        printf("total_bytes: %d\n", total_bytes);

        if (body_len > 0 && total_bytes >= body_len) {
            goto out_read_http_body_2_file;
        }

    } while (1);

out_read_http_body_2_file:

    fclose(fbody_file);
    return rc;
}

static int receive_http_response(int socket) {

    int rc = 1;
    int body_len = 0;

    FILE *f = fdopen(socket, "r+b");

    if (f == NULL) {
        return -1;
    }

    if (read_http_header_2_file(f, &body_len) == -1) {
        fprintf(stderr, "read_http_header_2_file fail\n");
        rc = -1;
        goto out;
    }

    if (read_http_body_2_file(f, body_len) == -1) {
        fprintf(stderr, "read_http_body_2_file fail\n");
        rc = -1;
        goto out;
    }

out:
    fclose(f);
    return rc;
}

static int send_request(struct sockaddr_in client_addr, int sd, char *request, int port, char *ip) {

    int num_write;
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(ip);
    client_addr.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Error connect");
        return 0;
    }


    if ((num_write = write(sd, request, strlen(request)) ) < 0) {
        perror("Error write");
        return 0;
    }

    receive_http_response(sd);
    return 1;
}

int main(int argc, char* argv[])
{
    char request[100];
    char ip[100];
    char add_proxy[100];
    char *env;
    struct sockaddr_in client_addr;
    int port = 0, port_server;
    int sd;

    url_parser_url_t *parsed_url;
    const char *address="http://www.cplusplus.com";
    int check_par;

    parsed_url = (url_parser_url_t *) malloc(sizeof(url_parser_url_t));


    if ((check_par = parse_url(address , true , parsed_url)) != 0) {
        fprintf(stderr, "Error parse url\n");
    }

    char *protocol, *host, *path;
    protocol = parsed_url->protocol;
    host = parsed_url->host;
    path = parsed_url->path;
    port_server = parsed_url->port;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket not created\n");
    }

    memset(&client_addr, 0, sizeof(client_addr));

    env = getenv("http_proxy");

    if (env) {

        edit_request_proxy(protocol, host, port_server, path, request);

        sscanf(env, "http://%[^:]:%d/", add_proxy, &port);

        printf("In proxy session\n");

        if (send_request(client_addr, sd, request, port, add_proxy) == 0) {
            perror("Error send request to proxy");
        }

    } else {
        printf("In no proxy\n");

        edit_request_no_proxy(protocol, host, port_server, path, request);

        if (host_to_ip(address, ip) != 1) {
            perror("Error host to ip");
        }

        if (send_request(client_addr, sd, request, port_server, ip) == 0) {
            perror("Error send request");
        }
    }

    return 0;
}
