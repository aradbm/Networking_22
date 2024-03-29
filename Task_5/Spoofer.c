#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pcap.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/filter.h>
#include <linux/types.h>
#include <arpa/inet.h>

unsigned short in_cksum(unsigned short *buf, int length)
{
  unsigned short *w = buf;
  int nleft = length;
  int sum = 0;
  unsigned short temp = 0;
  while (nleft > 1)
  {
    sum += *w++;
    nleft -= 2;
  }

  /* treat the odd byte at the end, if any */
  if (nleft == 1)
  {
    *(u_char *)(&temp) = *(u_char *)w;
    sum += temp;
  }

  /* add back carry outs from top 16 bits to low 16 bits */
  sum = (sum >> 16) + (sum & 0xffff); // add hi 16 to low 16
  sum += (sum >> 16);                 // add carry
  return (unsigned short)(~sum);
}

/* Ethernet header */
struct ethheader
{
  u_char ether_dhost[ETHER_ADDR_LEN]; /* destination host address */
  u_char ether_shost[ETHER_ADDR_LEN]; /* source host address */
  u_short ether_type;                 /* IP? ARP? RARP? etc */
};

// /* IP Header */
struct ipheader
{
  unsigned char iph_ihl : 4,       // IP header length
      iph_ver : 4;                 // IP version
  unsigned char iph_tos;           // Type of service
  unsigned short int iph_len;      // IP Packet length (data + header)
  unsigned short int iph_ident;    // Identification
  unsigned short int iph_flag : 3, // Fragmentation flags
      iph_offset : 13;             // Flags offset
  unsigned char iph_ttl;           // Time to Live
  unsigned char iph_protocol;      // Protocol type
  unsigned short int iph_chksum;   // IP datagram checksum
  struct in_addr iph_sourceip;     // Source IP address
  struct in_addr iph_destip;       // Destination IP address
};

void send_raw_ip_packet(struct ipheader *ip)
{
  struct sockaddr_in dest_info;
  int enable = 1;

  // Step 1: Create a raw network socket.
  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

  // Step 2: Set socket option.
  setsockopt(sock, IPPROTO_IP, IP_HDRINCL,
             &enable, sizeof(enable));

  // Step 3: Provide needed information about destination.
  dest_info.sin_family = AF_INET;
  dest_info.sin_addr = ip->iph_destip;
  // ip->iph_len = 0; // <--------------------for question 1------------------
  // Step 4: Send the packet out.
  sendto(sock, ip, ntohs(ip->iph_len), 0,
         (struct sockaddr *)&dest_info, sizeof(dest_info));
  close(sock);
}

// // /**********************************************
// //  * Listing 12.6: Constructing raw ICMP echo request packet
// //  **********************************************/

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>

// /* ICMP Header  */
struct icmpheader
{
  unsigned char icmp_type;        // ICMP message type
  unsigned char icmp_code;        // Error code
  unsigned short int icmp_chksum; // Checksum for ICMP Header and data
  unsigned short int icmp_id;     // Used for identifying request
  unsigned short int icmp_seq;    // Sequence number
};

// /******************************************************************
//   Spoof an ICMP echo request
// *******************************************************************/
int main()
{
  char buffer[1500];

  memset(buffer, 0, 1500);

  /*********************************************************
     Step 1: Fill in the ICMP header.
   ********************************************************/
  struct icmpheader *icmp = (struct icmpheader *)(buffer + sizeof(struct ipheader));
  icmp->icmp_type = 8; // ICMP Type: 8 is request, 0 is reply.

  // Calculate the checksum for integrity
  icmp->icmp_chksum = 0;
  icmp->icmp_chksum = (unsigned short)in_cksum((unsigned short *)icmp, sizeof(struct icmpheader));

  /*********************************************************
     Step 2: Fill in the IP header.
   ********************************************************/
  struct ipheader *ip = (struct ipheader *)buffer;
  ip->iph_ver = 4;
  ip->iph_ihl = 5;
  ip->iph_ttl = 20;
  ip->iph_sourceip.s_addr = inet_addr("1.2.3.4");
  ip->iph_destip.s_addr = inet_addr("172.17.0.1");
  ip->iph_protocol = IPPROTO_ICMP;
  ip->iph_len = htons(sizeof(struct ipheader) +
                      sizeof(struct icmpheader));

  /*********************************************************
     Step 3: Finally, send the spoofed packet
   ********************************************************/
  send_raw_ip_packet(ip);

  return 0;
}