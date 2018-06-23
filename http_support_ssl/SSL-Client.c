#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "parse_url.h"


#define FAIL -1
 
int OpenConnection(const char *hostname, int port) {   
    int sd;
    struct hostent *host;
    struct sockaddr_in addr;
 
    if ((host = gethostbyname(hostname)) == NULL) {
        perror(hostname);
        abort();
    }
    sd = socket(PF_INET, SOCK_STREAM, 0);

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = *(long*)(host->h_addr);

    if (connect(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0){
        close(sd);
        perror(hostname);
        abort();
    }
    return sd;
}
 
SSL_CTX* InitCTX() {   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_2_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if (ctx == NULL){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}


static void edit_request (char protocol[], char host[],
                int port, char path[], char *request) {
    char *method = "GET";
    char *version = "HTTP/1.1";

    if (port == 0) {
        snprintf(request, sizeof(char*)*200,
            "%s %s %s\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n\r\n",
            method, path, version,
            host);
    } else {
        snprintf(request, sizeof(char*)*200,
            "%s %s %s\r\n"
            "Host: %s:%d\r\n"
            "Connection: close\r\n\r\n",
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

    if ((s = getaddrinfo(address, "https", &hints, &servinfo)) != 0) {
        perror("Error");
        return 0;
    }

    rp = servinfo;
    if (rp != NULL) {
        ser_add = (struct sockaddr_in *) rp->ai_addr;
        snprintf(ip, sizeof(char*)*20, "%s",inet_ntoa(ser_add->sin_addr));
    }

    freeaddrinfo(servinfo);
    return 1;
}

int send_request (SSL *ssl, char *request) {
    int num_write;
    if ((num_write = SSL_write(ssl, request, strlen(request))) <= 0) {
        fprintf(stderr, "Error write\n");
        return 0;
    }
    return 1;
}


int receive_response (SSL *ssl) {
    int num_read;
    char buf[1024];
    int rc = 1;

    while (1) {
        if ((num_read = SSL_read(ssl, buf, sizeof(buf))) < 0 ) {
            fprintf(stderr, "Error read\n");
            goto out;
        }
        buf[num_read] = '\0';

        if (num_read <= 0) {
            rc = -1;
            goto out;
        }
        printf("%s", buf);
    }

out:
    return rc;
}

void parse_url_func (url_parser_url_t *parsed_url,  char *protocol, 
                    char *host, char *path, int *port_server) {
    snprintf(protocol, sizeof(char*)*20, "%s",parsed_url->protocol);
    snprintf(host, sizeof(char*)*20, "%s", parsed_url->host);
    snprintf(path, sizeof(char*)*20, "%s", parsed_url->path);
    *port_server = parsed_url->port;
}
 
int main (int argc, char *argv[]) {   
    SSL_CTX *ctx;
    int server;
    url_parser_url_t *parsed_url;
    int check_par;
    SSL *ssl;
    char ip[100];
    const char *hostname = "https://www.24h.com.vn/";
    char request[100];

    SSL_library_init();

    parsed_url = (url_parser_url_t *) malloc(sizeof(url_parser_url_t));

    if (parsed_url == NULL) {
        return 1;
    }

    if ((check_par = parse_url(hostname , true , parsed_url)) != 0) {
        fprintf(stderr, "Error parse url\n");
        return 1;
    }

    int port_server = 0;
    char protocol[100], host[100], path[100];

    parse_url_func(parsed_url, protocol, host, path, &port_server);

    edit_request(protocol, host, port_server, path, request);

    if (port_server == 0) {
        port_server = 443;
    }

    ctx = InitCTX();

    if (host_to_ip(host, ip) == 0 ) {
        fprintf(stderr, "Error host to ip\n");
        return 1;
    }

    server = OpenConnection(ip, port_server);

    if (server < 0) {
        fprintf(stderr, "Error create socket\n");
    }

    ssl = SSL_new(ctx);

    if (ssl == NULL) {
        fprintf(stderr, "Error create new SSL connection state\n");
        goto out;
    }

    if (SSL_set_fd(ssl, server) == 0) {
        fprintf(stderr, "Error attach the socket descriptor\n");
        goto out;
    }   

    if (SSL_connect(ssl) == FAIL) {
        ERR_print_errors_fp(stderr);
        goto out;
    }
    
    if (send_request(ssl, request) == 0) {
        goto out;
    }

    if (receive_response(ssl) == 0) {
        goto out;
    }

    
out:
    
    SSL_free(ssl);        /* release connection state */
    close(server);         /* close socket */
    SSL_CTX_free(ctx);        /* release context */
    return 0;
}
