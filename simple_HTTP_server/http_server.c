#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>
 
char response[] = "HTTP/1.1 200 OK\r\n\r\n<html><body><h1>Goodbye, world!</h1></body></html>\r\n";
 
int main()
{
    int client_fd;
    struct sockaddr_in svr_addr, cli_addr;
    socklen_t sin_len = sizeof(cli_addr);
    char request[1024];

    int num_write;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        err(1, "can't open socket");
    }
 
    int port = 8080;
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(port);
 
    if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
        close(sock);
        err(1, "Can't bind");
    }
 
    listen(sock, 5);

    while (1) {
        client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);
        printf("got connection\n");
        
        if(!fork()){
            printf("pid: %d\n", getpid());
            close(sock);
            int num_read;
            if ((num_read = read(client_fd, request, sizeof(request))) < 0) {
                fprintf(stderr, "Error read\n");
            }

            printf("request: %s\n", request);

            if (client_fd == -1) {
                perror("Can't accept");
                continue;
            }
     
            if ((num_write = write(client_fd, response, sizeof(response)) - 1) < 0) {
                fprintf(stderr, "Error write\n");
            }

            close(client_fd);
            exit(0);
        }

        close(client_fd);
    }

    close(sock);

    return 0;
}
