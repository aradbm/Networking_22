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

#define SERVER_PORT 5060
#define SERVER_IP_ADDRESS "127.0.0.1"
#define BUFFER_SIZE 65536

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 1
    if (sock == -1)
    {
        printf("Could not create socket : %d", errno);
        return -1;
    }
    int messageLen, bytesSent;
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
    int connectResult = connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (connectResult == -1)
    {
        printf("connect() failed with error code : %d", errno);
        // cleanup the socket;
        close(sock);
        return -1;
    } // 3
    printf("connected to server\n");

    FILE *file;
    char *filename = "mobydick2times.txt";

    ////////////////////////////////////
    while (1)
    {
        // Sends first half to server
        // reads text until newline is encountered
        file = fopen(filename, "r");
        if(!file){
            printf("ERROR! file opening has failed!\n");
        }
        char data[BUFFER_SIZE];
        while ((fread(data, 1, sizeof(data), file)) > 0)
        {
            if (send(sock, data, sizeof(data), 0) == -1)
            {
                perror("ERROR! Sending has failed!\n");
                exit(1);
            }
        }
        if (ferror(file))
        {
            perror("ERROR! file opening has failed!");
            close(sock);
            return -1;
        }
        fclose(file);
        printf("first half was successfully sent.\n");

        // Receive authentication from server
        char bufferReply[BUFFER_SIZE] = {'\0'};
        int bytesReceived = recv(sock, bufferReply, BUFFER_SIZE, 0);
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
                printf("Correct authentication!\n");
            }
            else
            {
                printf("Incorrect authentication!\n");
                goto exit_loop;
            }
        }
        //  Sends second half to server:
        file = fopen(filename, "r");
        if(!file){
            printf("ERROR! file opening has failed!\n");
        }
        char data[BUFFER_SIZE];
        while ((fread(data, 1, sizeof(data), file)) > 0)
        {
            if (send(sock, data, sizeof(data), 0) == -1)
            {
                perror("ERROR! Sending has failed!\n");
                exit(1);
            }
        }
        if (ferror(file))
        {
            perror("ERROR! file opening has failed!");
            close(sock);
            return -1;
        }
        fclose(file);
        printf("first half was successfully sent.\n");

        // writing to server if to end or not:
        printf("Enter message to send to server (write exit to exit, anything else to continue): \n");
        char buffer[BUFFER_SIZE] = {'\0'};
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strlen(buffer) - 1] = '\0'; // remove the trailing newline
        if (strcmp(buffer, "exit") == 0)
        {
            goto exit_loop;
        }
    }
exit_loop:;
    close(sock);
    return 0;
}