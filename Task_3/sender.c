#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <time.h>

#define SERVER_PORT 9988
#define SERVER_IP_ADDRESS "127.0.0.1"
#define BUFFER_SIZE 1024

int main()
{
    int senderSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 1
    if (senderSocket == -1)
    {
        printf("Could not create socket : %d", errno);
        return -1;
    }
    int amountSent, bytesSent;
    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    //
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);                                             // (5001 = 0x89 0x13) little endian => (0x13 0x89) network endian (big endian)
    int rval = inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &serverAddress.sin_addr); // convert IPv4 and IPv6 addresses from text to binary form
    // e.g. 127.0.0.1 => 0x7f000001 => 01111111.00000000.00000000.00000001 => 2130706433
    if (rval <= 0)
    {
        printf("inet_pton() failed");
        return -1;
    } // 2

    // Make a connection to the server with socket SendingSocket.
    char CC[256];
    int socklen = 0;
    strcpy(CC, "reno");
    socklen = strlen(CC);
    if (setsockopt(senderSocket, IPPROTO_TCP, TCP_CONGESTION, CC, socklen) != 0)
    {
        perror("ERROR! socket setting failed!");
        return -1;
    }
    if (connect(senderSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("ERROR! connection has failed!");
    }
    else
    {
        printf("You have successfully connected to the server\n");
    }

    ////////////////////////////////////
    while (1)
    {
        // Sends first half to server
        FILE *file1;
        file1 = fopen("bigt.txt", "r");
        if (!file1)
        {
            printf("ERROR! file opening has failed!\n");
        }
        char data[BUFFER_SIZE] = {0};
        int b = 0;
        while ((b = fread(data, sizeof(char), sizeof data, file1)) > 0)
        {
            bytesSent = send(senderSocket, data, b, 0);

            if (-1 == bytesSent)
            {
                printf("ERROR:  in sending file: %d", errno);
            }
            else if (0 == bytesSent)
            {
                printf("ERROR: peer has closed the TCP connection prior to send().\n");
            }
            else if (b > bytesSent)
            {
                printf("ERROR:  sent only %d bytes from the required %d.\n", bytesSent, b);
            }
            else
            {
                printf("Message was sent: %d bytes. \n", bytesSent);
            }
            amountSent += bytesSent;
        }
        fclose(file1);
        printf("first half was successfully sent total of: %d.\n", amountSent);
        // Receive authentication from server
        char bufferReply[1024] = {0};
        int bytesReceived = recv(senderSocket, bufferReply, 1024, 0);
        printf("Got here!!\n");
        if (bytesReceived == -1)
        {
            printf("recv() failed with error code : %d", errno);
        }
        else if (bytesReceived == 0)
        {
            printf("peer has closed the TCP connection prior to recv().\n");
        }
        else
        {
            printf("checking authentication from server: %d bytes.\n", bytesReceived);
            if (strncmp(bufferReply, "1234", 4) == 0)
            {
                printf("Correct authentication! %s\n", bufferReply);
            }
            else
            {
                printf("Incorrect authentication! %s\n", bufferReply);
                goto exit_loop;
            }
        }
        //  Sends second half to server:
        bzero(data, BUFFER_SIZE);
        b = 0;
        while ((b = fread(data, sizeof(char), sizeof data, file1)) > 0)
        {
            bytesSent = send(senderSocket, data, b, 0);

            if (-1 == bytesSent)
            {
                printf("Error in sending file: %d", errno);
            }
            else if (0 == bytesSent)
            {
                printf("peer has closed the TCP connection prior to send().\n");
            }
            else if (b > bytesSent)
            {
                printf("sent only %d bytes from the required %d.\n", bytesSent, b);
            }
            else
            {
                printf("Message was successfully sent. Sent %d bytes: \n", bytesSent);
            }
            amountSent += bytesSent;
        }
        // int sendResult = send(senderSocket, data, strlen(data) + 1, 0);
        // if (sendResult == -1)
        // {
        //     printf("send() failed with error code : %d\n", errno);
        //     // cleanup the socket;
        //     close(senderSocket);
        //     return -1;
        // }
        printf("Sent %d bytes to server\n", amountSent);
        bzero(data, BUFFER_SIZE);
        // writing to server if to end or not:
        printf("Enter message to send to server (write ex to exit, anything else to continue): \n");
        fgets(data, BUFFER_SIZE, stdin);
        data[strlen(data) - 1] = '\0'; // remove the trailing newline
        if (strcmp(data, "ex") == 0)
        {
            goto exit_loop;
        }
        bzero(data, BUFFER_SIZE);
    }
exit_loop:;
    close(senderSocket);
    return 0;
}