#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#define PORT_SERVER 80
#define PORT_PROXY 3128
#define SIZE 1024*1024

char request[] = "GET / HTTP/1.1\r\nHost: 192.168.81.249\r\n\r\n";
char request_proxy[] = "GET http://www.cplusplus.com/reference/cstring/memset/ HTTP/1.1\r\n"
                        "Host: www.cplusplus.com\r\n\r\n";

void parse_request (char *request) {

}

int send_request(struct sockaddr_in client_sd, int sd, char ip_des[]) {
    char buffer[SIZE];
    int num_read, num_write;

    client_sd.sin_family = AF_INET;
    client_sd.sin_addr.s_addr = inet_addr(ip_des);
    client_sd.sin_port = htons(PORT_PROXY);

    if (connect(sd, (struct sockaddr *)&client_sd, sizeof(client_sd)) < 0) {
        perror("Error connect");
        return 0;
    }

    if ((num_write = write(sd, request_proxy, sizeof(buffer)) ) < 0) {
        perror("Error write");
        return 0;
    }

    printf("Server proxy response:\n");
    while (1) {
        if ((num_read = read(sd, buffer, sizeof(buffer))) < 0) {
            perror("Error read");
            return 0;
        }
        buffer[num_read] = '\0';
        printf("%s", buffer);
    }
    return 1;
}

int main(int argc, char* argv[])
{
    char buffer[SIZE];
    int sd;
    struct sockaddr_in client_sd;
    int num_read, num_write;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket not created\n");
    }

    memset(&client_sd, 0, sizeof(client_sd));

    if (getenv("http_proxy")) {
        printf("In proxy session\n");
        if (send_request(client_sd, sd, "192.168.81.31") == 0) {
            perror("Error send request");
        }

    } else {
        printf("In no proxy\n");

        if( send_request(client_sd, sd, "192.168.81.249") == 0) {
            perror("Error send request");
        }
    }

    return 0;
}
