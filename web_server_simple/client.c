#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SIZE 1024

static int send_to(int sock,const  char request[], int size_request) {
    int num_send;

    if ((num_send = write(sock, request, size_request)) < 0) {
        perror("Error send");
        return -1;
    }
    return num_send;
}

static int receiv_from (int sock, char buffer[], int size_buffer) {
    int num_receiv;

    if ((num_receiv = read(sock, buffer, size_buffer)) < 0) {
        perror("Error receive");
        return -1;
    }
    return num_receiv;
}

int main(int argc, char const *argv[])
{
    struct sockaddr_in svr_addr;

    char request[] = "GET /Makefile HTTP/1.1\r\nHost: 127.0.0.1:8080\r\n\r\n";
    char buffer[1024];

    int num_receiv;

    int one = 1;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        fprintf(stderr, "Error create socket\n");
        return 1;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    svr_addr.sin_port = htons(8080);

    if (connect(sock, (struct sockaddr *)&svr_addr, sizeof(svr_addr)) < 0) {
        perror("Error connect");
        return 1;
    }

    printf("Connected\n");

    while (1) {

        send_to(sock, request, SIZE);

        num_receiv = receiv_from(sock, buffer,  SIZE);
        buffer[num_receiv] = '\0';

        printf("receive:\n");
        printf("%s\n", buffer);

    }

    return 0;
}