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

#define SIZE 1024

char request[] = "GET / HTTP/1.1\r\nHost: 192.168.81.249\r\n\r\n";
char request_proxy[] = "GET http://www.cplusplus.com HTTP/1.1\r\n"
                        "Host: www.cplusplus.com\r\n\r\n";

void edit_request_proxy (char address[], char *request) {

    char *method = "GET";
    char *version = "HTTP/1.1";
    char *host = "Host: www.";

    snprintf(request, sizeof(char)*100, "%s http://www.%s %s\r\n"
        "%s%s\r\n\r\n", method, address, version, host, address);
}

void edit_request_no_proxy (char address[], char *request) {
    char *method = "GET";
    char *version = "HTTP/1.1";
    char *host = "Host:";

    snprintf(request, sizeof(char*)*100, "%s / %s\r\n%s %s\r\n\r\n",method, version, host, address);

    printf("request: %s\n", request);
}

char * get_value_env (char *name_env) {
    char *value_env = malloc(1024);
    char *port;

    value_env = getenv(name_env);

    value_env = strstr(value_env,"//") + 2;

    port = strrchr(value_env, ':');
    port[strlen(port) - 1] = '\0';

    value_env[port-value_env] = '\0';

    return value_env;
}

char * get_port_proxy (char *name_env) {
    char * value_env;
    char *port = malloc(10);

    value_env = getenv(name_env);

    port = strrchr(value_env, ':') + 1;

    port[strlen(port)] = '\0';

    return port;
}

int host_to_ip (char *address, char *ip) {
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

int write_all_to_file (char buffer[], const char * filename) {
    int num_write;
    int fd = open("full.txt", O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);

    if (fd == -1 ) {
        perror("Error open all file");
        return 0;
    }

    if ((num_write = write(fd, buffer, sizeof((char *)buffer))) < 0) {
        perror("Error write from server");
        return 0;
    }
    return 1;
}

int write_header (char buffer[], const char * filename, FILE *f) {
    char header[100];

    FILE * f_head = fopen("header.txt", "w");

    if (f_head == NULL) {
        perror("Error fopen header");
        return 0;
    }

    while (fgets(header, sizeof(header), f) != NULL && strcmp(header, "\r\n") != 0) {
        if (fwrite(header, sizeof(char), strlen(header), f_head) == 0) {
            perror("Error line write to header");
            return 0;
        }
    }

    fclose(f_head);
    return 1;
}

int write_body(char buffer[], const char * filename, FILE * f) {
    char body[1024];
    FILE *f_content = fopen("content.cat", "wb");

    if (f_content == NULL) {
        perror("Error open f_content");
        return 0;
    }

    while (fgets(body, sizeof(body), f) != NULL) {
        printf("%s", body);
        if (fwrite(body, sizeof(char), strlen(body), f_content) == 0) {
            perror("Error line write to body");
            return 0;
        }
    }

    return 1;
}

int send_request(struct sockaddr_in client_addr, int sd, char *request, int port, char *ip) {
    char header[SIZE];
    char buffer[SIZE*SIZE];
    int num_read, num_write;

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(ip);
    client_addr.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Error connect");
        return 0;
    }

    if ((num_write = write(sd, request, sizeof(buffer)) ) < 0) {
        perror("Error write");
        return 0;
    }

    if ((num_read = read(sd, buffer, sizeof(header))) < 0) {
        perror("Error read");
        return 0;
    }
    header[num_read] = '\0';

    FILE * f = fopen("full.txt","rb");

    if (f == NULL) {
        perror("Error fopen");
        return 0;
    }

    if (write_all_to_file(buffer, "full.txt") == 0) {
        perror("Error write all to file");
        return 0;
    }

    if (write_header(buffer, "header.txt", f) == 0) {
        perror("Error write to file");
        return 0;
    }

    if (write_body(buffer, "content.cat", f) == 0){
        perror("Error write to body");
        return 0;
    }



    return 1;
}

int main(int argc, char* argv[])
{
    char request[100];
    char ip[100];
    char *add_proxy;
    char *port_proxy;
    int sd;
    struct sockaddr_in client_addr;
    int port;

    char address[100];

    printf("Enter address: ");
    scanf("%s",address);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket not created\n");
    }

    memset(&client_addr, 0, sizeof(client_addr));


    if (getenv("http_proxy")) {

        edit_request_proxy(address, request);

        port_proxy = get_port_proxy("http_proxy");

        add_proxy = get_value_env("http_proxy");

        sscanf(port_proxy, "%d", &port);

        printf("In proxy session\n");

        if (send_request(client_addr, sd, request, port, add_proxy) == 0) {
            perror("Error send request");
        }

    } else {
        printf("In no proxy\n");

        edit_request_no_proxy(address, request);
        port = 80;

        if (host_to_ip(address, ip) != 1) {
            perror("Error host to ip");
        }

        if (send_request(client_addr, sd, request, port, ip) == 0) {
            perror("Error send request");
        }
    }

    return 0;
}
