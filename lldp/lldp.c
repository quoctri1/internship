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

const unsigned char LLDP_DST_MAC[] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e};
const unsigned int  LLDP_ETH_TYPE  = 0x88cc;

enum lldp_type {
    LLDP_END_LLDPDU         = 0,
    LLDP_CHASSIS_ID         = 1,
    LLDP_PORT_ID            = 2,
    LLDP_TIME_TO_LIVE       = 3,
    LLDP_PORT_DESCRIPTION   = 4,
    LLDP_SYSTEM_NAME        = 5,
    LLDP_SYSTEM_DESCRIPTION = 6,
    LLDP_SYSTEM_CAPABILITIES= 7,
    LLDP_MANAGEMENT_ADDRESS = 8,
};


#define TRUE    1
#define FALSE   0

struct lldp_subtype
{
    unsigned char  sub_type;
    char           data[1024];
    unsigned char  len;
};

struct lldp_mgnt_addr {
    unsigned char addr_string_len;
    unsigned char addr_subtype;
    struct in_addr mgnt_addr;
    unsigned char interf_sub;
    unsigned int interf_num;
    unsigned char OID_string_len;
    unsigned char len;
};

static int lldp_add_tlv(unsigned int type, unsigned int length, void *value,
                    unsigned char *buffer);
static void set_ethernet_header (unsigned char *frame, unsigned char hw_addr[]);
static int  get_network_interface_info (const char * interface_name, unsigned char hw_addr[],
                    int *interface_index);
static void set_socket_address (struct sockaddr_ll *socket_address,
            const unsigned char dst_hw_addr[], int interface_index);

static int  lldp_set_chassis_tlv (unsigned char *buffer);
static int  lldp_set_port_tlv (unsigned char *buffer);
static int  lldp_set_time_to_live_tlv (unsigned char *buffer, unsigned int seconds);
static int  lldp_set_system_desc_tlv (unsigned char *buffer);
static int  lldp_set_system_name_tlv (unsigned char *buffer);
static int  lldp_set_mngt_addr_tlv (unsigned char *buffer);
static void lldp_send_frame (struct sockaddr_ll socket_address, unsigned char *buffer,
                    unsigned int total_len);

/********************************************************************************/

static int lldp_add_tlv(unsigned int type, unsigned int length,
                    void *value, unsigned char *buffer)
{
    unsigned int type_and_length;

    type_and_length = (type << 9) | (length & 0x1FF);
    type_and_length = htons(type_and_length);

    memcpy(buffer, &type_and_length, 2);
    memcpy(buffer + 2 , value, length);

    return 2 + length;
}

static int lldp_add_tlv_mgnt_addr(unsigned int type, unsigned int length,
                    void *value, unsigned char *buffer)
{
    unsigned int type_and_length;

    type_and_length = (type << 9) | (length & 0x1FF);
    type_and_length = htons(type_and_length);

    memcpy(buffer, &type_and_length, 2);
    memcpy(buffer + 2 , value, 2);
    memcpy(buffer + 4, value + 4, 4);
    memcpy(buffer + 8, value + 8, length - 8);

    return 2 + length;
}


static void set_ethernet_header (unsigned char *frame, unsigned char hw_addr[]) {

    struct ethhdr *ethernet_header = (struct ethhdr *)frame;

    memcpy(ethernet_header->h_dest, LLDP_DST_MAC, 6);
    memcpy(ethernet_header->h_source, hw_addr, 6);

    ethernet_header->h_proto = htons(LLDP_ETH_TYPE);
}

static int get_network_interface_info(const char *interface_name, unsigned char hw_addr[],
                        int *interface_index)
{
    struct ifreq    ifr;
    int             socket_fd;

    socket_fd = socket(AF_PACKET, SOCK_RAW, 0);
    if (socket_fd == -1) {
        perror("socket():");
        close(socket_fd);
        return FALSE;
    }

    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", interface_name);

    if (ioctl(socket_fd, SIOCGIFINDEX, &ifr) == -1) {
        perror("SIOCGIFINDEX");
        close(socket_fd);
        return FALSE;
    }

    *interface_index = ifr.ifr_ifindex;

    if (ioctl(socket_fd, SIOCGIFHWADDR, &ifr) == -1) {
        perror("SIOCGIFHWADDR");
        close(socket_fd);
        return FALSE;
    }

    memcpy(hw_addr, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);

    close(socket_fd);

    return TRUE;
}

static void set_socket_address(struct sockaddr_ll *socket_address,
            const unsigned char dst_hw_addr[], int interface_index)
{
    socket_address->sll_family  = AF_PACKET;
    socket_address->sll_protocol= LLDP_ETH_TYPE;
    socket_address->sll_ifindex = interface_index;
    socket_address->sll_hatype  = htons(ARPHRD_ETHER);
    socket_address->sll_pkttype = (PACKET_OTHERHOST);
    socket_address->sll_halen   = 6;
    memcpy(socket_address->sll_addr, dst_hw_addr, 6);
}

static int  lldp_set_chassis_tlv (unsigned char *buffer) {
    char hostname[20];

    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("Error get hostname");
        return 0;
    }
    struct lldp_subtype lldp_chassis;

    lldp_chassis.sub_type = 1;
    snprintf(lldp_chassis.data, sizeof(lldp_chassis.data), "%s", hostname);
    lldp_chassis.len = 1 + strlen(hostname);

    return lldp_add_tlv(LLDP_CHASSIS_ID, lldp_chassis.len, &lldp_chassis, buffer);
}

static int lldp_set_port_tlv (unsigned char *buffer) {
    struct lldp_subtype lldp_port;
    lldp_port.sub_type = 7;
    snprintf(lldp_port.data, sizeof(lldp_port.data), "%s", "PORT-1");
    lldp_port.len = 1 + strlen("PORT-1");

    return lldp_add_tlv(LLDP_PORT_ID, lldp_port.len, &lldp_port, buffer);
}

static int lldp_set_time_to_live_tlv (unsigned char *buffer, unsigned int seconds) {
    unsigned int ttl = htons(seconds);
    return lldp_add_tlv(LLDP_TIME_TO_LIVE, 2, &ttl, buffer);
}

static int get_ubuntu_version (char *ubuntu_version)
{
    char temp[100];

    FILE *f = fopen("/etc/lsb-release", "r");

    if (f == NULL) {
        return FALSE;
    }

    while (fgets(temp, sizeof(temp), f) > 0) {
        sscanf(temp, "DISTRIB_DESCRIPTION=%[^\n]", ubuntu_version);
    }

    //modify ubuntu version after get it
    int element;
    for (element = 0; element < strlen(ubuntu_version); element++) {
        ubuntu_version[element] = ubuntu_version[element + 1];
    }
    ubuntu_version[strlen(ubuntu_version) - 1] = '\0';

    fclose(f);

    return TRUE;
}

static int get_CPU_name (char *CPU_name)
{
    char temp[100];
    FILE *f = fopen("/proc/cpuinfo", "r");

    if (f == NULL) {
        return FALSE;
    }

    while (fgets(temp, sizeof(temp), f) > 0) {
        sscanf(temp, "model name\t: %[^\n]", CPU_name);
    }

    return TRUE;
}

static int lldp_set_system_desc_tlv (unsigned char *buffer)
{
    char ubuntu_version[100];
    char CPU_name[100];

    get_ubuntu_version(ubuntu_version);

    get_CPU_name(CPU_name);

    struct lldp_subtype lldp_sys_desc;

    snprintf(lldp_sys_desc.data, sizeof(lldp_sys_desc.data), "%s ", ubuntu_version);

    strncat(lldp_sys_desc.data, CPU_name, sizeof(CPU_name));

    lldp_sys_desc.len = strlen(lldp_sys_desc.data);

    return lldp_add_tlv(LLDP_SYSTEM_DESCRIPTION, lldp_sys_desc.len, &lldp_sys_desc, buffer);
}

static int lldp_set_system_name_tlv (unsigned char *buffer) {

    char hostname[64];

    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("Error get hostname");
        return 0;
    }

    return lldp_add_tlv(LLDP_SYSTEM_NAME, strlen(hostname), hostname, buffer);
}

static int get_ip_address (char *ip4_address)
{
    struct ifreq    ifr;
    int fd;
    fd = socket(PF_INET, SOCK_DGRAM, 0);

    if (fd < 0 ) {
        return FALSE;
    }

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "enp3s0", sizeof("enp3s0"));

    if (ioctl(fd, SIOCGIFADDR, &ifr) == -1 ) {
        perror("SIOCGIFADDR");
        close(fd);
        return FALSE;
    }

    snprintf(ip4_address, strlen(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr)) + 1,
            "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

    close(fd);

    return 1;
}

static int lldp_set_mngt_addr_tlv (unsigned char *buffer)
{
    char ip4_address[20];

    get_ip_address(ip4_address);

    struct lldp_mgnt_addr lldp_mgnt;

    lldp_mgnt.addr_string_len = 1 + sizeof(lldp_mgnt.mgnt_addr);

    lldp_mgnt.addr_subtype = 1;

    inet_aton(ip4_address, &lldp_mgnt.mgnt_addr);

    lldp_mgnt.interf_sub = 2;

    lldp_mgnt.interf_num = htonl(1);

    lldp_mgnt.OID_string_len = 0;

    lldp_mgnt.len = lldp_mgnt.addr_string_len + sizeof(lldp_mgnt.interf_num) +
            sizeof(lldp_mgnt.addr_subtype) + sizeof(lldp_mgnt.interf_sub) +
                sizeof(lldp_mgnt.OID_string_len) + 1 + 1;

    // return lldp_add_tlv_mgnt_addr(LLDP_MANAGEMENT_ADDRESS, lldp_mgnt.len, &lldp_mgnt, buffer);
    return lldp_add_tlv(LLDP_MANAGEMENT_ADDRESS, lldp_mgnt.len, &lldp_mgnt, buffer);
}

static void lldp_send_frame(struct sockaddr_ll socket_address, unsigned char *buffer,
                    unsigned int total_len)
{
    int socket_fd;

    socket_fd = socket(AF_PACKET, SOCK_RAW, 0);
    if (socket_fd == -1) {
        perror("socket():");
        close(socket_fd);
        return;
    }

    if (sendto(socket_fd, buffer, total_len, 0, (struct sockaddr*) &socket_address,
                 sizeof(socket_address)) < 0) {
        perror("Error sendto");
        close(socket_fd);
        return;
    }

    close(socket_fd);
}

/********************************************************************************/

int main(int argc, char const *argv[])
{
    int                 interface_index;
    unsigned char       hw_addr[6];
    struct sockaddr_ll  socket_address;
    int                 total_len;
    unsigned char       buffer[128];

    if (argc < 2) {
        perror("argv1");
        return 1;
    }

    get_network_interface_info(argv[1], hw_addr, &interface_index);

    set_socket_address(&socket_address, LLDP_DST_MAC, interface_index);

    memset(buffer, 0, sizeof(buffer));

    set_ethernet_header(buffer, hw_addr);

    total_len = sizeof(struct ethhdr);

    total_len += lldp_set_chassis_tlv(buffer + total_len);
    total_len += lldp_set_port_tlv(buffer + total_len);
    total_len += lldp_set_time_to_live_tlv(buffer + total_len, 120);
    total_len += lldp_set_system_desc_tlv(buffer + total_len);
    total_len += lldp_set_system_name_tlv(buffer + total_len);
    total_len += lldp_set_mngt_addr_tlv(buffer + total_len);
    total_len += lldp_add_tlv(LLDP_END_LLDPDU, 0, NULL, buffer + total_len);

    lldp_send_frame(socket_address, buffer, total_len);

    return 0;
}
