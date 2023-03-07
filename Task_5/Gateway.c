#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define P 8000
#define BUFFER_SIZE 1024
int create_dest_sock(int *Socket, struct sockaddr_in *Server, int Port)
{
    if ((*(Socket) = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Error in the socket for receiving\n");
        exit(1);
    }
    // set up server_for_sending information
    memset((char *)Server, 0, sizeof(Server));
    Server->sin_family = AF_INET;
    Server->sin_addr.s_addr = htonl(INADDR_ANY);
    Server->sin_port = htons(Port);
    return 0;
}

int main(int argc, char **argv)
{
    int send_sock, recv_sock;
    struct sockaddr_in sending_addr, recv_addr;
    struct hostent *hp;
    char buffer[BUFFER_SIZE];
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s hostname\n", argv[0]);
        exit(1);
    }
    create_dest_sock(&send_sock, &sending_addr, P + 1);
    if ((hp = gethostbyname(argv[1])) == NULL)
    {
        fprintf(stderr, "Error: host %s not found\n", argv[1]);
        exit(1);
    }
    memcpy((char *)&sending_addr.sin_addr, hp->h_addr, hp->h_length);
    create_dest_sock(&recv_sock, &recv_addr, P);
    if (bind(recv_sock, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0)
    {
        perror("bind\n");
        exit(1);
    }
    while (1)
    {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client); // receiving from
        printf("We are waiting for a message\n");
        // receive datagram from port P
        int n = recvfrom(recv_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client, &client_len);
        if (n < 0)
        {
            perror("recvfrom\n");
            exit(1);
        }
        printf("got msg\n");
        // sample random number to decide whether to forward the datagram
        float rnd = ((float)random()) / ((float)RAND_MAX);
        if (rnd > 0.5)
        {
            printf("got %f :: forwarding msg\n", rnd);
            if (sendto(send_sock, buffer, n, 0, (struct sockaddr *)&sending_addr, sizeof(sending_addr)) < 0)
            {
                perror("sendto\n");
                exit(1);
            }
        }
        else
            printf("got %f :: throwing msg\n", rnd);
    }
    return 0;
}