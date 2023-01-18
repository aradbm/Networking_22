/**********************************************
 * Listing 12.2: Packet Capturing using raw socket
 **********************************************/
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <linux/filter.h>
#include <linux/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>

/**********************************************
 * Listing 12.1: A compiled BPF code
 **********************************************/

typedef struct clchdr
{
  uint32_t unixtime;
  uint16_t length;
  uint16_t reserved : 3, c_flag : 1, s_flag : 1, t_flag : 1, status : 10;
  uint16_t cache;
  uint16_t padding;
} cpack, *pcpack;

int main()
{
  int PACKET_LEN = 512;
  char buffer[PACKET_LEN];
  struct sockaddr saddr;
  struct packet_mreq mr;

  // Create the raw socket
  int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));

  // Turn on the promiscuous mode.
  mr.mr_type = PACKET_MR_PROMISC;
  setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr,
             sizeof(mr));

  // Getting captured packets
  while (1)
  {
    int data_size = recvfrom(sock, buffer, PACKET_LEN, 0,
                             &saddr, (socklen_t *)sizeof(saddr));

    struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));
    unsigned short iphdrlen = iph->ihl * 4;
    struct tcphdr *tcph = (struct tcphdr *)(buffer + iphdrlen + sizeof(struct ethhdr));
    // struct clchdr *clch = (struct clchdr *)(buffer + iphdrlen + sizeof(struct ethhdr) + sizeof(struct tcphdr));
    unsigned short tcphdrlen = tcph->th_off;
    struct clchdr *clch = (struct clchdr *)(buffer + iphdrlen + sizeof(struct tcphdr) + sizeof(struct ethhdr));

    unsigned int source_port = ntohs(tcph->source);
    unsigned int dest_port = ntohs(tcph->dest);

    if (data_size)
    {
      if (dest_port == 9999 || source_port == 9999)
      {
        struct in_addr addr;
        addr.s_addr = htonl(iph->saddr);
        char *str = inet_ntoa(addr);
        addr.s_addr = htonl(iph->daddr);
        char *str2 = inet_ntoa(addr);
        printf("{ source_ip: %s, dest_ip: %s, source_port: %d, dest_port: %d, timestamp: %d, \ntotal_length: %d, cache_flag:%hu, steps_flag:%hu, type_flag:%hu, status_code:%d, cache_control:%hu, data:%d}\n",
               str, str2, dest_port, source_port, 0, ntohs(iph->tot_len), clch->c_flag, clch->s_flag, clch->t_flag, ntohs(clch->status), ntohs(clch->cache), 0);
      }
    }
  }

  close(sock);
  return 0;
}

// /**********************************************
//  * Listing 12.3: Packet Capturing using raw libpcap
//  **********************************************/

// #include <pcap.h>
// #include <stdio.h>

// void got_packet(u_char *args, const struct pcap_pkthdr *header,
//         const u_char *packet)
// {
//    printf("Got a packet\n");
// }

// int main()
// {
//   pcap_t *handle;
//   char errbuf[PCAP_ERRBUF_SIZE];
//   struct bpf_program fp;
//   char filter_exp[] = "ip proto icmp";
//   bpf_u_int32 net;

//   // Step 1: Open live pcap session on NIC with name eth3
//   handle = pcap_open_live("eth3", BUFSIZ, 1, 1000, errbuf);
//   puts("finised step 1");
//   // Step 2: Compile filter_exp into BPF psuedo-code
//   pcap_compile(handle, &fp, filter_exp, 0, net);
//   pcap_setfilter(handle, &fp);
//   puts("finised step 2");

//   // Step 3: Capture packets
//   pcap_loop(handle, -1, got_packet, NULL);
//   puts("finised step 3");

//   pcap_close(handle);   //Close the handle
//   return 0;
// }

// /**********************************************
//  * Code on Page 213 (Section 12.2.4)
//  **********************************************/

// /* Ethernet header */
// struct ethheader {
//   u_char  ether_dhost[ETHER_ADDR_LEN]; /* destination host address */
//   u_char  ether_shost[ETHER_ADDR_LEN]; /* source host address */
//   u_short ether_type;                  /* IP? ARP? RARP? etc */
// };

// void got_packet(u_char *args, const struct pcap_pkthdr *header,
//                               const u_char *packet)
// {
//   struct ethheader *eth = (struct ethheader *)packet;
//   if (ntohs(eth->ether_type) == 0x0800) { ... } // IP packet
//   ...
// }

// /**********************************************
//  * Listing 12.4: Get captured packet
//  **********************************************/

// #include <pcap.h>
// #include <stdio.h>
// #include <arpa/inet.h>

// /* IP Header */
// struct ipheader {
//   unsigned char      iph_ihl:4, //IP header length
//                      iph_ver:4; //IP version
//   unsigned char      iph_tos; //Type of service
//   unsigned short int iph_len; //IP Packet length (data + header)
//   unsigned short int iph_ident; //Identification
//   unsigned short int iph_flag:3, //Fragmentation flags
//                      iph_offset:13; //Flags offset
//   unsigned char      iph_ttl; //Time to Live
//   unsigned char      iph_protocol; //Protocol type
//   unsigned short int iph_chksum; //IP datagram checksum
//   struct  in_addr    iph_sourceip; //Source IP address
//   struct  in_addr    iph_destip;   //Destination IP address
// };

// void got_packet(u_char *args, const struct pcap_pkthdr *header,
//                               const u_char *packet)
// {
//   struct ethheader *eth = (struct ethheader *)packet;

//   if (ntohs(eth->ether_type) == 0x0800) { // 0x0800 is IP type
//     struct ipheader * ip = (struct ipheader *)
//                            (packet + sizeof(struct ethheader));

//     printf("       From: %s\n", inet_ntoa(ip->iph_sourceip));
//     printf("         To: %s\n", inet_ntoa(ip->iph_destip));

//     /* determine protocol */
//     switch(ip->iph_protocol) {
//         case IPPROTO_TCP:
//             printf("   Protocol: TCP\n");
//             return;
//         case IPPROTO_UDP:
//             printf("   Protocol: UDP\n");
//             return;
//         case IPPROTO_ICMP:
//             printf("   Protocol: ICMP\n");
//             return;
//         default:
//             printf("   Protocol: others\n");
//             return;
//     }
//   }
// }
