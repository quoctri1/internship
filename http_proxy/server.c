#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <string.h>
#include <pthread.h>
#include<unistd.h>


#define SIZE 1024


void runSocket(int  vargp)
{
    char buffer[SIZE];
    int bytes = 0;
    while (1) {
        //receive data from client
        memset(&buffer,'\0',sizeof(buffer));
        bytes = read(vargp, buffer, sizeof(buffer));
        buffer[bytes] = '\0';
        if(bytes <0) {
            perror("Error read from client");
        } else if (bytes == 0) {
        } else {
            // similar to echo server
            write(vargp, buffer, sizeof(buffer));
            //printf("From client:\n");
            fputs(buffer,stdout);
        }
            fflush(stdout);
    }
    // return NULL;
}

int main()
{
    int client_fd;
    char buffer[SIZE];
    int fd;
    struct sockaddr_in server_sd;

    // add this line only if server exits when client exits
    signal(SIGPIPE,SIG_IGN);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("Server started\n");

    memset(&server_sd, 0, sizeof(server_sd));
    server_sd.sin_family = AF_INET;
    server_sd.sin_port = htons(5010);
    server_sd.sin_addr.s_addr = INADDR_ANY;

    // bind socket to the port
    bind(fd, (struct sockaddr*)&server_sd,sizeof(server_sd));
    // start listening at the given port for new connection requests
    listen(fd, 5);
    // continuously accept connections in while(1) loop
    while(1)
    {
        // accept any incoming connection
        client_fd = accept(fd, (struct sockaddr*)NULL ,NULL);
        //printf("accepted client with id: %d",client_fd);
        // if true then client request is accpted
        if(client_fd > 0)
        {
            //multithreading variables
            printf("proxy connected\n");
            runSocket(client_fd);
        }
    }
    close(client_fd);
    return 0;
}
