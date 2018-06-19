#include<stdio.h> //printf
#include<string.h>    //strlen
#include<stdlib.h>    //malloc
#include<sys/socket.h>    //you know what this is for
#include<arpa/inet.h> //inet_addr , inet_ntoa , ntohs etc
#include<netinet/in.h>
#include<unistd.h>    //getpid

#define MAXLINE 1024


//This application helps to send a message to DNS server of google,
//get a response and show IP adress. You need to provide argv[1] as the address

struct DNS_HEADER
{
    unsigned short id; // identification number

    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritive answer
    unsigned char opcode :4; // purpose of message
    unsigned char qr :1; // query/response flag

    unsigned char rcode :4; // response code
    unsigned char cd :1; // checking disabled
    unsigned char ad :1; // authenticated data
    unsigned char z :1; // its z! reserved
    unsigned char ra :1; // recursion available

    unsigned short q_count; // number of question entries
    unsigned short ans_count; // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count; // number of resource entries
};

struct QUESTION {
    // unsigned char *name;
    unsigned short qtype;
    unsigned short qclass;
};

#pragma pack(push, 1)
struct R_DATA
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)

//Pointers to resource record contents
struct RES_RECORD
{
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};

void set_dns (struct DNS_HEADER *dns) {
    dns->id = (unsigned short) htons(getpid());
    dns->qr = 0; //This is a query
    dns->opcode = 0; //This is a standard query
    dns->aa = 0; //Not Authoritative
    dns->tc = 0; //This message is not truncated
    dns->rd = 1; //Recursion Desired
    dns->ra = 0; //Recursion not available! hey we dont have it (lol)
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1); //we have only 1 question
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;
}

static void ChangetoDnsNameFormat(unsigned char* dns, const char* host)
{
    int lock = 0 , i;
    strncat((char*)host,".",1);

    for(i = 0 ; i < strlen((char*)host) ; i++)
    {
        if(host[i]=='.')
        {
            *dns++ = i-lock;
            for(;lock<i;lock++)
            {
                *dns++=host[lock];
            }
            lock++;
        }
    }
    *dns++='\0';
}

static void send_to (int sockfd, unsigned char* buf, struct sockaddr_in servaddr, socklen_t size)
{
    printf("Sending Packet...");
    if( sendto(sockfd,(unsigned char*)buf, size, 0,(struct sockaddr*)&servaddr,sizeof(servaddr)) < 0)
    {
        perror("sendto failed");
        return;
    }
    printf("Done\n");
}

static void receive_from(int sockfd, unsigned char* buf, struct sockaddr_in servaddr)
{
    int size = sizeof(servaddr);
    printf("Receiving answer...");
    if(recvfrom (sockfd,(char*)buf , 65536 , 0 , (struct sockaddr*)&servaddr , (socklen_t*)&size ) < 0)
    {
        perror("recvfrom failed");
        return;
    }
    printf("Done\n");
}

static void show_address (struct DNS_HEADER *dns, struct RES_RECORD answers[], unsigned char * reader, struct sockaddr_in servaddr  ) {
    int i;
    for (i=0;i<ntohs(dns->ans_count); i++) {
        reader = reader + 2;

        answers[i].resource = (struct R_DATA*)(reader);

        reader = reader + sizeof(struct R_DATA);

        if(ntohs(answers[i].resource->type) == 1) //if its an ipv4 address
        {
            answers[i].rdata = (unsigned char*)malloc(ntohs(answers[i].resource->data_len));

            for(int j=0 ; j<ntohs(answers[i].resource->data_len) ; j++)
            {
                answers[i].rdata[j]=reader[j];
            }

            answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';

            reader = reader + ntohs(answers[i].resource->data_len);
        }
    }

    for(i=0 ; i < ntohs(dns->ans_count) ; i++)
    {
        if( ntohs(answers[i].resource->type) == 1) //IPv4 address
        {
            memcpy(&servaddr.sin_addr.s_addr, answers[i].rdata, 4); //working without ntohl
            printf("IPv4 address : %s",inet_ntoa(servaddr.sin_addr));
        }
        printf("\n");
    }
}

int main(int argc, char const *argv[]) {
    int sockfd;
    unsigned char buf[1024*1024];
    unsigned char *qname, *reader;
    // unsigned char address[100];

    struct sockaddr_in servaddr;

    struct DNS_HEADER *dns;

    struct QUESTION *q_ques;

    struct RES_RECORD answers[10];

    if (argc != 2) {
        fprintf(stderr, "Error argument\n");
        return 1;
    }

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("8.8.8.8");
    servaddr.sin_port = htons(53);

    dns = (struct DNS_HEADER *)&buf;
    set_dns(dns);

    qname = (unsigned char *)&buf[sizeof(struct DNS_HEADER)];
    ChangetoDnsNameFormat(qname, argv[1]);

    q_ques = (struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + strlen((const char*)qname) + 1];
    q_ques->qtype = htons(1); //IP4
    q_ques->qclass = htons(1); //use internet

    socklen_t size = sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION);

    send_to(sockfd, buf, servaddr,size);

    receive_from(sockfd, buf, servaddr);

    printf("Questions: %d\n",ntohs(dns->q_count));
    printf("Answer: %d\n", ntohs(dns->ans_count));

    reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];
    show_address(dns, answers, reader, servaddr);
    return 0;
}
