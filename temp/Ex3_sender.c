#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define PORT 5093
#define SERVER_IP_ADDRESS "127.0.0.1"
#define SIZE 65536
int main(int argc, char const *argv[])
{
	int sock = 0;
	struct sockaddr_in serv_addr;

	for (int i = 0; i < 5; i++)
	{
		// printf("open socket\n");
		char buffer[SIZE] = {0};
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("\n Socket creation error \n");
			return -1;
		}

		bzero(&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(PORT);
		if (inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &serv_addr.sin_addr) <= 0)
		{
			printf("\ninet_pton() failed.\n");
			return -1;
		}

		// connect
		printf("connecting...\n");
		if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		{
			printf("connect() failed with error code : %d \n", errno);
			return -1;
		}
		printf("connected to server\n");

		printf("sending...\n");
		FILE *fp1;
		fp1 = fopen("1gb.txt", "r");
		if (fp1 == NULL)
		{
			perror("Error in reading file.");
			return 2; // exit(1);
		}
		printf("Sending file %d...\n", i + 1);
		int b = 0;
		int bytesSent = 0;
		int count = 0;
		int amountSeg = 0;
		while ((b = fread(buffer, 1, sizeof(buffer), fp1)) > 0)
		{ // while(fgets(buffer, sizeof(buffer), fp1) != NULL) {
			//	send(sock, buffer, b, 0);
			int bytesSent = send(sock, buffer, b, 0); // int bytesSent = send(sock, buffer, sizeof(buffer), 0);
			if (-1 == bytesSent)
			{
				printf("Error in sending file: %d", errno);
			}
			else if (0 == bytesSent)
			{
				printf("peer has closed the TCP connection prior to send().\n");
			}
			else if (b > bytesSent) //(sizeof(buffer) > bytesSent)
			{
				printf("sent only %d bytes from the required %d.\n", bytesSent, b); // printf("sent only %d bytes from the required %d.\n", bytesSent, sizeof(buffer));
			}
			else
			{
				// printf("Message was successfully sent. Send %d bytes: \n", bytesSent);
			}
			count += bytesSent;
			amountSeg += 1;
		}
		printf("Send %d bytes of file %d by %d segments.\n", count, i + 1, amountSeg);
		// bzero(buffer, SIZE);
		fclose(fp1);
		close(sock);
	}

	printf("Changing algorithm!\n");
	for (int i = 0; i < 5; i++)
	{
		// printf("open socket\n");
		sock = 0;
		char buffer[SIZE] = {0};

		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("\n Socket creation error \n");
			return -1;
		}
		bzero(&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(PORT);
		if (inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &serv_addr.sin_addr) <= 0)
		{
			printf("\ninet_pton() failed \n");
			return -1;
		}

		// change CC algorithm to reno
		strcpy(buffer, "reno");
		if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buffer, strlen(buffer)) != 0)
		{
			perror("setsockopt");
			return -1;
		}
		socklen_t len = sizeof(buffer);
		if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buffer, &(len)) != 0)
		{
			perror("getsockopt");
			return -1;
		}
		printf("***********Algorithm changed to %s ***********\n", buffer);

		printf("connecting...\n");
		if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		{
			printf("\nConnection Failed \n");
			return -1;
		}

		FILE *fp1;
		printf("Sending file... %d\n", i + 1);
		fp1 = fopen("1gb.txt", "rb");
		if (fp1 == NULL)
		{
			perror("Error in reading file.");
			return 2;
		}
		int b = 0;
		int bytesSent = 0;
		int count = 0;
		int amountSeg = 0;
		bzero(buffer, SIZE);
		while ((b = fread(buffer, 1, sizeof(buffer), fp1)) > 0)
		{
			bytesSent = send(sock, buffer, b, 0);

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
				// printf("Message was successfully sent. Send %d bytes: \n", bytesSent);
			}
			count += bytesSent;
			amountSeg += 1;
		}
		printf("Send %d bytes of file %d by %d segments.\n", count, i + 1, amountSeg);
		// bzero(buffer, SIZE);
		fclose(fp1);
		close(sock);
	}

	return 0;
}