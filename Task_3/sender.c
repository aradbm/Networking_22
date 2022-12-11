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

    // reads text until newline is encountered
    FILE *file;
    file = fopen("mobydick2times.txt", "r");
    char ch;
    int i = 0, j = 0, len = 0;
    while (!feof(file))
    {
        ch = fgetc(file);
        len++;
    }
    int parts = len / 1024;
    fclose(file);
    // 65536
    FILE *f;
    f = fopen("mobydick2times.txt", "r");
    printf("parts is: %d\n", parts);
    char half[parts / 2][1024];
    while (!feof(f) && i < parts / 2)
    {
        while (j < 1023)
        {
            half[i][j] = fgetc(f);
            // putchar(half[i][j]);
            j++;
        }
        half[i][j] = 0;

        j = 0;
        i++;
    }
    printf("1count is: %d\n", i);
    while (!feof(f) && i < parts / 2)
    {
        while (j < 1023)
        {
            half[i][j] = fgetc(f);

            j++;
        }
        half[i][j] = 0;
        j = 0;
        i++;
    }
    fclose(f);
    ////////////////////////////////////
    while (1)
    {
        // We got:  half[0] = first half, half[1] = second half
        // Sends first half to server
        messageLen = strlen(half[0]) + 1;
        bytesSent = send(sock, half[0], messageLen, 0); // 4
        if (bytesSent == -1)
        {
            printf("send() failed with error code : %d", errno);
        }
        else if (bytesSent == 0)
        {
            printf("peer has closed the TCP connection prior to send().\n");
        }
        else if (bytesSent < messageLen)
        {
            printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
        }
        else
        {
            printf("first half was successfully sent.\n");
        }

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
        messageLen = strlen(half[1]) + 1;
        bytesSent = send(sock, half[1], messageLen, 0); // 4
        if (bytesSent == -1)
        {
            printf("send() failed with error code : %d\n", errno);
        }
        else if (bytesSent == 0)
        {
            printf("peer has closed the TCP connection prior to send().\n");
        }
        else if (bytesSent < messageLen)
        {
            printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
        }
        else
        {
            printf("second half was successfully sent.\n");
        }
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