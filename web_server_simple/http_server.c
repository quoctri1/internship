#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>

#define SIZE 1024
#define PORT 8080

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

int main()
{
    int client_fd;
    struct sockaddr_in svr_addr, cli_addr;
    socklen_t sin_len = sizeof(cli_addr);
    char request[SIZE];
    char response[] = "HTTP/1.1 200 OK\r\n\r\n<html><body><h1>Goodbye, world!</h1></body></html>\r\n";

    int num_receiv;
    int one = 1;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        fprintf(stderr, "Error create socket\n");
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(PORT);


    if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
        perror("Error bind");
        close(sock);
        return 1;
    }


    if (listen(sock, 5)  < 0) {
        perror("Error listen");
        close(sock);
        return 1;
    }

    while (1) {
        if ((client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len)) == -1) {
            perror("Error accept");
            return 1;
        }
        printf("connection\n");

        num_receiv = receiv_from(client_fd, request, SIZE);
        request[num_receiv] = '\0';

        printf("request: %s\n", request);

        send_to(client_fd, response, SIZE);

        close(client_fd);
    }

    close(sock);
    return 0;
}
