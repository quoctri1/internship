#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>


int main()
{
    int client_fd;
    struct sockaddr_in svr_addr, cli_addr;
    socklen_t sin_len = sizeof(cli_addr);
    char request[1024];
    char response[] = "HTTP/1.1 200 OK\r\n\r\n<html><body><h1>Goodbye, world!</h1></body></html>\r\n";

    int num_write;
    int child;
    int one = 1;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        fprintf(stderr, "Error create socket\n");
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(8080);


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

        int num_read;
        if ((num_read = read(client_fd, request, sizeof(request))) < 0) {
            perror("Error read");
            close(client_fd);
            return 1;
        }

        if ((num_write = write(client_fd, response, sizeof(response)) -1) < 0) {
            perror("Error write\n");
            close(client_fd);
            return 1;
        }

        close(client_fd);
    }

    close(sock);
    return 0;
}
