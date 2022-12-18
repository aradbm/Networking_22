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

#define SERVER_PORT 5065
#define SERVER_IP_ADDRESS "127.0.0.1"
#define FILE_SIZE 1048575
#define BUFFER_SIZE 8192

int main()
{
    // Creating new socket
    int SendingSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 1
    if (SendingSocket == -1)
    {
        printf("Could not create socket : %d", errno);
        goto end;
    }
    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    // convert IPv4 and IPv6 addresses from text to binary form
    int rval = inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &serverAddress.sin_addr);
    if (rval <= 0)
    {
        printf("inet_pton() failed");
        goto end;
    }
    // Make a connection to the server with socket SendingSocket.
    int connectResult = connect(SendingSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (connectResult == -1)
    {
        printf("connect() failed with error code : %d", errno);
        // cleanup the socket;
        close(SendingSocket);
        goto end;
    }
    printf("connected to server..\n");
restart:
    // Changing to cubic CC algorithm
    char ccBuffer[256];
    strcpy(ccBuffer, "cubic");
    socklen_t socklen = strlen(ccBuffer);
    if (setsockopt(SendingSocket, IPPROTO_TCP, TCP_CONGESTION, ccBuffer, socklen) != 0)
    {
        perror("ERROR! socket setting failed!");
        goto end;
    }
    socklen = sizeof(ccBuffer);
    if (getsockopt(SendingSocket, IPPROTO_TCP, TCP_CONGESTION, ccBuffer, &socklen) != 0)
    {
        perror("ERROR! socket getting failed!");
        goto end;
    }
    printf("Changed Congestion Control to Cubic\n");

    FILE *file;
    file = fopen("0.txt", "r");
    int amountSent = 0, bytesRead;
    char data1[FILE_SIZE / 2] = {0};
    char data2[FILE_SIZE / 2] = {0};

    // Reads first half of file
    bytesRead = fread(data1, 1, sizeof(data1), file);
    if (bytesRead < 0)
    {
        printf("fread failed loading.\n");
        goto end;
    }
    else if (bytesRead == 0)
    {
        printf("fread 0 bytes!!\n");
    }
    else
    {
        printf("fread %d bytes.\n", bytesRead);
    }
    // Sending first half of file
    if ((amountSent = send(SendingSocket, data1, sizeof(data1), 0)) == -1)
    {
        printf("ERROR! Sending has failed!\n");
        goto end;
    }
    else if (amountSent == 0)
    {
        printf("ERROR! Sending only 0.\n");
        goto end;
    }
    else
    {
        printf("sent %d bytes\n", bytesRead);
    }

    // Getting authentication message
    char bufferReply[BUFFER_SIZE] = {'\0'};
    int bytesReceived = recv(SendingSocket, bufferReply, BUFFER_SIZE, 0);
    if (bytesReceived == -1)
    {
        printf("recv() failed with error code.");
        goto end;
    }
    else if (bytesReceived == 0)
    {
        printf("peer has closed the TCP connection prior to recv().\n");
        goto end;
    }
    else
    {
        printf("received %d bytes from server. reply: %s\n", bytesReceived, bufferReply);
    }
    // Checking authentication
    char * xor = "1740887";
    if (strcmp(xor, bufferReply) != 0)
    {
        printf("inccorect autheritication\n");
        goto end;
    }
    // Changing to reno CC algorithm
    strcpy(ccBuffer, "reno");
    socklen = strlen(ccBuffer);
    if (setsockopt(SendingSocket, IPPROTO_TCP, TCP_CONGESTION, ccBuffer, socklen) != 0)
    {
        printf("ERROR! socket setting failed!\n");
        return -1;
    }
    socklen = sizeof(ccBuffer);
    if (getsockopt(SendingSocket, IPPROTO_TCP, TCP_CONGESTION, ccBuffer, &socklen) != 0)
    {
        printf("ERROR! socket getting failed!\n");
        goto end;
    }
    printf("Changed Congestion Control to Reno\n");
    // reads second half of file
    bytesRead = fread(data2, 1, sizeof(data2), file);
    if (bytesRead < 0)
    {
        printf("fread failed loading.\n");
        close(SendingSocket);
        return -1;
    }
    else if (bytesRead == 0)
    {
        printf("fread 0 bytes!!\n");
    }
    else
    {
        printf("fread %d bytes.\n", bytesRead);
    }
    fclose(file);
    amountSent = 0;
    // Sending 2nd half of file
    if ((amountSent = send(SendingSocket, data2, sizeof(data2), 0)) == -1)
    {
        printf("ERROR! Sending has failed!\n");
        return -1;
    }
    else if (amountSent == 0)
    {
        printf("ERROR! Sending only 0.\n");
        return -1;
    }
    else
    {
        printf("sent %d bytes\n", bytesRead);
    }
    printf("Sent in half 2  %d bytes \n", amountSent);

    // USER DECISION
    char buffer[BUFFER_SIZE] = {0};
    printf("Enter \"stop\" to end session:\n");
    fgets(buffer, BUFFER_SIZE, stdin);
    int bytesSent = send(SendingSocket, buffer, BUFFER_SIZE, 0); // 4
    if (bytesSent == -1)
    {
        printf("send() failed with error code : %d", errno);
    }
    else if (bytesSent == 0)
    {
        printf("peer has closed the TCP connection prior to send().\n");
    }
    else if (bytesSent < strlen(buffer))
    {
        printf("sent only %d bytes\n", bytesSent);
    }
    else
    {
        printf("sent msg succesfuly!: %s \n", buffer);
    }
    if (strncmp(buffer, "stop", 4) != 0)
    {
        bzero(buffer, BUFFER_SIZE);
        printf("****************************************************");
        goto restart;
    }
end:;
    close(SendingSocket);
    return 0;
}