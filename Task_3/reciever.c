#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>

#define SERVER_PORT 5060  // The port that the server listens
#define BUFFER_SIZE 65536 // how many bytes to recieve each time

int main()
{
    // Open the listening (server) socket
    int listeningSocket = -1;
    listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 0 means default protocol for stream sockets (Equivalently, IPPROTO_TCP)
    if (listeningSocket == -1)
    {
        printf("Could not create listening socket : %d\n", errno);
        return 1;
    }
    // Reuse the address if the server socket on was closed
    // and remains for 45 seconds in TIME-WAIT state till the final removal.
    int enableReuse = 1;
    int ret = setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0)
    {
        printf("setsockopt() failed with error code : %d\n", errno);
        return 1;
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    //
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;          // choosing ipv4
    serverAddress.sin_addr.s_addr = INADDR_ANY;  // any IP at this port (Address to accept any incoming messages)
    serverAddress.sin_port = htons(SERVER_PORT); // network order (makes byte order consistent)

    // Bind the socket to the port with any IP at this port
    int bindResult = bind(listeningSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (bindResult == -1)
    {
        printf("Bind failed with error code : %d\n", errno);
        // close the socket
        close(listeningSocket);
        return -1;
    }
    printf("Bind() success\n");

    // Make the socket listening;
    // 500 is a Maximum size of queue connection requests; number of concurrent connections.
    int listenResult = listen(listeningSocket, 5);
    if (listenResult == -1)
    {
        printf("listen() failed with error code : %d\n", errno);
        // close the socket
        close(listeningSocket);
        return -1;
    }

    // Accept and incoming connection
    printf("Waiting for incoming TCP-connections...\n");
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);
    char buffer[BUFFER_SIZE];
    while (1)
    {
        printf("waiting..\n");
        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddressLen = sizeof(clientAddress);
        int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (clientSocket == -1)
        {
            printf("listen failed with error code : %d", errno);
            // close the sockets
            close(listeningSocket);
            return -1;
        }
        printf("A new client connection accepted\n");
        int amountReceived = 0;
        int continueRecv;
        // char *extMSG = "";
        while (1)
        {
            //// Receive from client first half of file
            // change to cubic CC algorithm
            char ccBuffer[256];
            printf("Changed Congestion Control to Cubic\n");
            strcpy(ccBuffer, "cubic");
            socklen_t socklen = strlen(ccBuffer);
            if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, ccBuffer, socklen) != 0)
            {
                perror("ERROR! socket setting failed!");
                return -1;
            }
            socklen = sizeof(ccBuffer);
            if (getsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, ccBuffer, &socklen) != 0)
            {
                perror("ERROR! socket getting failed!");
                return -1;
            }
            // receiving the data

            memset(&ccBuffer, 0, sizeof(ccBuffer));
            while ((continueRecv = recv(clientSocket, &buffer, sizeof(buffer), 0) > 0))
            {
                if (continueRecv < 0)
                {
                    printf("recv failed with error code : %d", errno);
                    // close the sockets
                    close(listeningSocket);
                    close(clientSocket);
                    return -1;
                }
                amountReceived += continueRecv;
            }
            printf("Received: %d bytes", amountReceived);

            //// send authentication to client:
            // calculated before xor between IDs: 206391054 XOR 207083353 = 1234
            char *message = "1234";
            int messageLen = strlen(message) + 1;
            // sending the authentication msg to client for
            int bytesSent = send(clientSocket, message, messageLen, 0);
            if (bytesSent == -1)
            {
                printf("send() failed with error code : %d", errno);
                close(listeningSocket);
                close(clientSocket);
                return -1;
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
                printf("Authentication message was successfully sent.\n");
            }
            // Changing to reno algorithm
            printf("Changed Congestion Control to Reno\n");
            strcpy(ccBuffer, "reno");
            socklen = strlen(ccBuffer);
            if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, ccBuffer, socklen) != 0)
            {
                perror("ERROR! socket setting failed!");
                return -1;
            }
            socklen = sizeof(ccBuffer);
            if (getsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, ccBuffer, &socklen) != 0)
            {
                perror("ERROR! socket getting failed!");
                return -1;
            }
            //// Receive from client second half of file:
            amountReceived = 0;
            continueRecv = 0;
            memset(&ccBuffer, 0, sizeof(ccBuffer));
            while ((continueRecv = recv(clientSocket, &buffer, sizeof(buffer), 0) > 0))
            {

                if (continueRecv < 0)
                {
                    printf("recv failed with error code : %d\n", errno);
                    // close the sockets
                    close(listeningSocket);
                    close(clientSocket);
                    return -1;
                }
                amountReceived += continueRecv;
            }
            printf("Received: %d bytes\n", amountReceived);

            // receiving if i want to exit:
            amountReceived = 0;
            amountReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            printf("Received msg of %d bytes from client: %s\n", amountReceived, buffer);
            // check if got the "exit" command from client, if yes, then exit and close the client socket
            if (strncmp(buffer, "exit", 4) == 0)
            {
                printf("Client has quit the session\n");
            }
            memset(buffer, 0, BUFFER_SIZE);
        }
        close(listeningSocket);
    }
    return 0;
}