#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_arp.h>
#include <sys/time.h>
#include <signal.h>


#define DEST0 0x01
#define DEST1 0x80
#define DEST2 0xc2
#define DEST3 0x00
#define DEST4 0x00
#define DEST5 0x0e


struct lldp_subtype
{
    unsigned char  sub_type;
    char           data[1024];
    unsigned char  len;
};

static int add_tlv(unsigned int type, unsigned int length, void *value,
                    unsigned char *buffer);
static void set_ethernet (unsigned char *request, struct ifreq ifr);
static void get_MAC_address (struct ifreq *ifr, int *ifindex, const char * interface_name);
static void set_socket_address (struct sockaddr_ll *socket_address, struct ifreq ifr,
                            int ifindex);
static int set_chassis (unsigned char *buffer);
static int set_port (unsigned char *buffer);
static int set_ttl (unsigned char *buffer, unsigned int time);
static void send_req (struct sockaddr_ll socket_address, unsigned char *buffer,
                    unsigned int total_len);


static int add_tlv(unsigned int type, unsigned int length,
                    void *value, unsigned char *buffer) {
    unsigned int type_len;
    type_len = (type << 9) | (length & 0x1FF);
    type_len = htons(type_len);
    memcpy(buffer, &type_len, 2);
    memcpy(buffer + 2 , value, length);
    return 2 + length;
}

static void set_ethernet (unsigned char *request, struct ifreq ifr) {

    struct ethhdr *send_req = (struct ethhdr *)request;

    send_req->h_dest[0] = (unsigned char)DEST0;
    send_req->h_dest[1] = (unsigned char)DEST1;
    send_req->h_dest[2] = (unsigned char)DEST2;
    send_req->h_dest[3] = (unsigned char)DEST3;
    send_req->h_dest[4] = (unsigned char)DEST4;
    send_req->h_dest[5] = (unsigned char)DEST5;

    send_req->h_source[0] = (unsigned char)ifr.ifr_hwaddr.sa_data[0];
    send_req->h_source[1] = (unsigned char)ifr.ifr_hwaddr.sa_data[1];
    send_req->h_source[2] = (unsigned char)ifr.ifr_hwaddr.sa_data[2];
    send_req->h_source[3] = (unsigned char)ifr.ifr_hwaddr.sa_data[3];
    send_req->h_source[4] = (unsigned char)ifr.ifr_hwaddr.sa_data[4];
    send_req->h_source[5] = (unsigned char)ifr.ifr_hwaddr.sa_data[5];

    send_req->h_proto = htons(0x88cc);
}

static void get_MAC_address (struct ifreq *ifr, int *ifindex, const char * interface_name) {
    int sd;
    sd = socket(AF_PACKET, SOCK_RAW, 0);
    if (sd == -1) {
        perror("socket():");
        close(sd);
        return ;
    }

    snprintf(ifr->ifr_name, 10, "%s", interface_name);

    if (ioctl(sd, SIOCGIFINDEX, ifr) == -1) {
        perror("SIOCGIFINDEX");
        close(sd);
        return ;
    }

    *ifindex = ifr->ifr_ifindex;

    if (ioctl(sd, SIOCGIFHWADDR, ifr) == -1) {
        perror("SIOCGIFINDEX");
        close(sd);
        return ;
    }

    close(sd);
}

static void set_socket_address (struct sockaddr_ll *socket_address, struct ifreq ifr,
                            int ifindex) {
    socket_address->sll_family = AF_PACKET;
    socket_address->sll_protocol = 0x88cc;
    socket_address->sll_ifindex = ifindex;
    socket_address->sll_hatype = htons(ARPHRD_ETHER);
    socket_address->sll_pkttype = (PACKET_OTHERHOST);
    socket_address->sll_halen = 6;
    socket_address->sll_addr[0] = (unsigned char)ifr.ifr_hwaddr.sa_data[0];
    socket_address->sll_addr[1] = (unsigned char)ifr.ifr_hwaddr.sa_data[1];
    socket_address->sll_addr[2] = (unsigned char)ifr.ifr_hwaddr.sa_data[2];
    socket_address->sll_addr[3] = (unsigned char)ifr.ifr_hwaddr.sa_data[3];
    socket_address->sll_addr[4] = (unsigned char)ifr.ifr_hwaddr.sa_data[4];
    socket_address->sll_addr[5] = (unsigned char)ifr.ifr_hwaddr.sa_data[5];
}

static int set_chassis (unsigned char *buffer) {
    char hostname[20];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("Error get hostname");
        return 1;
    }
    struct lldp_subtype lldp_chassis;

    lldp_chassis.sub_type = 1;
    snprintf(lldp_chassis.data, sizeof(lldp_chassis.data), "%s", hostname);
    lldp_chassis.len = 1 + strlen(hostname);

    return add_tlv(1, lldp_chassis.len, &lldp_chassis, buffer);
}

static int set_port (unsigned char *buffer) {
    struct lldp_subtype lldp_port;
    lldp_port.sub_type = 7;
    snprintf(lldp_port.data, sizeof(lldp_port.data), "%s", "PORT-1");
    lldp_port.len = 1 + strlen("PORT-1");

    return add_tlv(2, lldp_port.len, &lldp_port, buffer);
}

static int set_ttl (unsigned char *buffer, unsigned int time) {
    unsigned int ttl = htons(time);
    return add_tlv(3, 2, &ttl, buffer);
}

static void send_req (struct sockaddr_ll socket_address, unsigned char *buffer,
                    unsigned int total_len) {
    int num_send;
    int sd = socket(AF_PACKET, SOCK_RAW, 0);
    if (sd == -1) {
        perror("socket():");
        close(sd);
        return ;
    }

    if ((num_send = sendto(sd, buffer, total_len, 0,
            (struct  sockaddr*)&socket_address, sizeof(socket_address))) < 0 ) {
        printf("socket_address: %d\n", socket_address.sll_ifindex);
        perror("Error sendto");
        close(sd);
        return ;
    }

    close(sd);
}

static void set_timer (struct itimerval *it_val) {
    if (signal(SIGALRM, (void)send_req) == SIG_ERR) {
        perror("Unable to catch SIGALRM");
        exit(1);
    }

    it_val->it_value.tv_sec =     2;
    it_val->it_value.tv_usec =    100;
    it_val->it_interval = it_val->it_value;

    if (setitimer(ITIMER_REAL, it_val, NULL) == -1) {
        perror("error calling setitimer()");
        exit(1);
    }
}

int main(int argc, char const *argv[])
{
    int chassis_len, port_len, ttl_len, end_len;
    struct sockaddr_ll           socket_address;
    int                      ifindex, total_len;
    unsigned char                   buffer[128];
    struct                            ifreq ifr;
    struct                     itimerval it_val;

    if (argc < 2) {
        perror("argv1");
        return 1;
    }

    get_MAC_address(&ifr, &ifindex, argv[1]);

    set_socket_address(&socket_address, ifr, ifindex);

    memset(buffer, 0, sizeof(buffer));

    set_ethernet(buffer, ifr);

    total_len = sizeof(struct ethhdr);

    chassis_len = set_chassis(buffer + total_len);
    if (chassis_len < 0 ) {
        fprintf(stderr, "Error set chassis\n");
        exit(1);
    }
    total_len += chassis_len;

    port_len = set_port(buffer + total_len);
    if (port_len < 0 ) {
        fprintf(stderr, "Error set port\n");
        exit(1);
    }
    total_len += port_len;

    ttl_len = set_ttl(buffer + total_len, 120);
    if (ttl_len < 0 ) {
        fprintf(stderr, "Error set ttl\n");
        exit(1);
    }
    total_len += ttl_len;

    end_len = add_tlv(0, 0, NULL, buffer + total_len);
    if (end_len < 0) {
        fprintf(stderr, "Error set ends\n");
        exit(1);
    }
    total_len += end_len;

    send_req(socket_address, buffer, total_len);

    set_timer(&it_val);

    while (1) {
        pause();
    }

    return 0;
}
