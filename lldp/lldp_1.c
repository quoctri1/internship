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

struct if_info {
    int if_index;
    char sa_data[6];
};

struct subtype {
    char subtype;
    char data[1024];
    int  len;
};

static int  build_tlv(unsigned int type, unsigned int length, void* value, void* frame);
static int  set_ethernet_header(unsigned char *frame, struct if_info ifr);
static int  get_ethernet_ifreq(struct if_info *ifr, const char* if_name);
static void dump_mac_addr(unsigned char *mac);
static void build_socket_address(struct sockaddr_ll *socket_address, struct if_info ifr);
static int  set_tlv_chassis_component(unsigned char *frame);
static int  set_tlv_port_id(unsigned char *frame, const char*if_name);
static int  set_tlv_time_to_live(unsigned char *frame, unsigned int ttl);

int main(int argc, char const *argv[])
{
    struct if_info      ifr;
    struct sockaddr_ll  socket_address;
    unsigned char       frame_buff[256];
    int                 offset;

    if (argc < 2) {
        perror("argv1");
        return 1;
    }

    get_ethernet_ifreq(&ifr, argv[1]);

    memset(&socket_address, 0, sizeof(socket_address));
    build_socket_address(&socket_address, ifr);

    offset = set_ethernet_header(frame_buff, ifr);

    offset += set_tlv_chassis_component(frame_buff + offset);

    offset += set_tlv_port_id(frame_buff + offset, argv[1]);

    // time_to_live
    offset += set_tlv_time_to_live(frame_buff + offset, 120);

    // END of TLV
    offset += build_tlv(0, 0, NULL, frame_buff + offset);

    int num_send;
    int sd;

    sd = socket(AF_PACKET, SOCK_RAW, 0);
    if (sd == -1) {
        perror("socket():");
        return 1;
    }

    if ((num_send = sendto(sd, frame_buff, offset, 0, (struct sockaddr*) &socket_address,
        sizeof(socket_address))) < 0) {
        perror("Error sendto");
        return 1;
    }

    return 0;
}

static int set_ethernet_header(unsigned char *frame, struct if_info ifr)
{
#define DEST0 0x01
#define DEST1 0x80
#define DEST2 0xc2
#define DEST3 0x00
#define DEST4 0x00
#define DEST5 0x0e

    struct ethhdr *send_req = (struct ethhdr *) frame;

    send_req->h_dest[0] = (unsigned char) DEST0;
    send_req->h_dest[1] = (unsigned char) DEST1;
    send_req->h_dest[2] = (unsigned char) DEST2;
    send_req->h_dest[3] = (unsigned char) DEST3;
    send_req->h_dest[4] = (unsigned char) DEST4;
    send_req->h_dest[5] = (unsigned char) DEST5;

    send_req->h_source[0] = (unsigned char) ifr.sa_data[0];
    send_req->h_source[1] = (unsigned char) ifr.sa_data[1];
    send_req->h_source[2] = (unsigned char) ifr.sa_data[2];
    send_req->h_source[3] = (unsigned char) ifr.sa_data[3];
    send_req->h_source[4] = (unsigned char) ifr.sa_data[4];
    send_req->h_source[5] = (unsigned char) ifr.sa_data[5];

    send_req->h_proto = htons(0x88cc);

    return sizeof(struct ethhdr);
}

static int get_ethernet_ifreq(struct if_info *if_info, const char* if_name)
{
    struct ifreq ifr;
    int socket_fd;

    socket_fd = socket(AF_PACKET, SOCK_RAW, 0);
    if (socket_fd == -1) {
        perror("socket():");
        close(socket_fd);
        return -1;
    }

    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", if_name);

    if (ioctl(socket_fd, SIOCGIFINDEX, &ifr) == -1) {
        perror("SIOCGIFINDEX");
        close(socket_fd);
        return -1;
    }

    if_info->if_index = ifr.ifr_ifindex;

    if (ioctl(socket_fd, SIOCGIFHWADDR, &ifr) == -1) {
        perror("SIOCGIFHWADDR");
        close(socket_fd);
        return -1;
    }

    memcpy(if_info->sa_data, ifr.ifr_hwaddr.sa_data, 6);

    printf("ifindex : %d \n", if_info->if_index);
    dump_mac_addr((unsigned char *)if_info->sa_data);

    close(socket_fd);

    return 1;
}

static void dump_mac_addr(unsigned char *mac)
{
    printf("Mac address: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", mac[0], mac[1], mac[2], mac[3],
                            mac[4], mac[5]);
}

static void build_socket_address(struct sockaddr_ll *socket_address,
                struct if_info if_info)
{
    socket_address->sll_family = AF_PACKET;
    socket_address->sll_protocol = 0x88cc;
    socket_address->sll_ifindex = if_info.if_index;

    socket_address->sll_hatype = htons(ARPHRD_ETHER);
    socket_address->sll_pkttype = (PACKET_BROADCAST);

    socket_address->sll_halen = 6;
    socket_address->sll_addr[0] = (unsigned char) if_info.sa_data[0];
    socket_address->sll_addr[1] = (unsigned char) if_info.sa_data[1];
    socket_address->sll_addr[2] = (unsigned char) if_info.sa_data[2];
    socket_address->sll_addr[3] = (unsigned char) if_info.sa_data[3];
    socket_address->sll_addr[4] = (unsigned char) if_info.sa_data[4];
    socket_address->sll_addr[5] = (unsigned char) if_info.sa_data[5];
}

static int build_tlv(unsigned int type, unsigned int length, void* value, void* frame)
{
    unsigned int type_lenght;

    type_lenght = htons((type << 9) | (length & 0x1FF));  // 7 bits + 9 bits = 16 bits

    memcpy(frame, &type_lenght, 2);
    memcpy(frame + 2, value, length);

    return 2 + length;
}

static int  set_tlv_chassis_component(unsigned char *frame) {
    struct subtype sub;

    char hostname[64];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("Error get hostname");
        return -1;
    }

    sub.subtype = 1;
    snprintf(sub.data, sizeof(sub.data), "%s", hostname);
    sub.len = strlen(hostname) + 1;

    printf("hostname: %s\n", hostname);

    return build_tlv(1, sub.len, &sub, frame);
}

static int  set_tlv_port_id(unsigned char *frame, const char*if_name) {
    struct subtype sub;

    sub.subtype = 1;
    snprintf(sub.data, sizeof(sub.data), "%s", if_name);
    sub.len = strlen(if_name) + 1;

    return build_tlv(2, sub.len, &sub, frame);
}

static int  set_tlv_time_to_live(unsigned char *frame, unsigned int ttl) {
    ttl = htons(ttl);
    return build_tlv(3, 2, &ttl, frame);
}
