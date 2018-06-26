#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <net/if.h>
#include <netinet/ether.h>



int main(int argc, char const *argv[])
{

    int sock_raw;
    sock_raw = socket(AF_PACKET,SOCK_RAW,IPPROTO_RAW);
    if (sock_raw == -1)
        printf("error in socket\n");


    struct ifreq ifreq_i;
    memset(&ifreq_i, 0, sizeof(ifreq_i));
    strncpy(ifreq_i.ifr_name, "enp3s0", IFNAMSIZ-1); //giving name of Interface

    if ((ioctl(sock_raw, SIOCGIFINDEX, &ifreq_i))<0)
        printf("error in index ioctl reading\n");//getting Index Name

    printf("index=%d\n", ifreq_i.ifr_ifindex);

    struct ifreq ifreq_c;
    memset(&ifreq_c , 0, sizeof(ifreq_c));
    strncpy(ifreq_c.ifr_name, "enp3s0", IFNAMSIZ-1);//giving name of Interface

    if ((ioctl(sock_raw, SIOCGIFHWADDR, &ifreq_c))<0) //getting MAC Address
        printf("error in SIOCGIFHWADDR ioctl reading\n");

    struct ifreq ifreq_ip;
    memset(&ifreq_ip, 0, sizeof(ifreq_ip));
    strncpy(ifreq_ip.ifr_name, "enp3s0", IFNAMSIZ-1);//giving name of Interface
    if (ioctl(sock_raw, SIOCGIFADDR, &ifreq_ip)<0) //getting IP Address
    {
        printf("error in SIOCGIFADDR \n");
    }

    char sendbuff[64];

    memset(sendbuff,0,64);

    struct ethhdr *eth = (struct ethhdr *)(sendbuff);

    eth->h_source[0] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]);
    eth->h_source[1] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]);
    eth->h_source[2] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]);
    eth->h_source[3] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]);
    eth->h_source[4] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]);
    eth->h_source[5] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]);

    /* filling destination mac. DESTMAC0 to DESTMAC5 are macro having octets of mac address. */
    eth->h_dest[0] = DESTMAC0;
    eth->h_dest[1] = DESTMAC1;
    eth->h_dest[2] = DESTMAC2;
    eth->h_dest[3] = DESTMAC3;
    eth->h_dest[4] = DESTMAC4;
    eth->h_dest[5] = DESTMAC5;

    eth->h_proto = htons(ETH_P_IP); //means next header will be IP header

    /* end of ethernet header */
    total_len+=sizeof(struct ethhdr);


    return 0;
}
