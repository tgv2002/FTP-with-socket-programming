// Server side
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#define PORT 8000

char buffer[16384];
char fileName[1024];
int name_leng;
char msg[32];
char connec[2];
char download[2];

void handleCrash(int signum)
{
	fprintf(stderr, "\nClient had exited, Server proceeding forward\n");
}

void ctrlc(int signum)
{
	fprintf(stdout, "\nServer shutting down\n");
	exit(EXIT_SUCCESS);
}

int getNum(char buffer[16384])
{
	int leng = strlen(buffer);
	int val = 0, dig;

	for(int i=0;i<leng;i++)
	{
		if(buffer[i] >= '0' && buffer[i] <= '9')
		{
			dig = (int)(buffer[i] - '0');
			val = (val * 10) + dig;
		}

		else
		{
			return -1;   
		}
	}

	return val;
}

int main(int argc, char const *argv[])
{
	signal(SIGPIPE, handleCrash);
	signal(SIGINT, ctrlc);
	int serv_fd, new_sock, readContent;
	struct sockaddr_in address;  
	int option = 1, count, i;
	int addrleng = sizeof(address);

	// Creating socket file descriptor
	if ((serv_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)  // creates socket, for TCP
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Lose the pesky "Address already in use" error message
	if (setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
				&option, sizeof(option))) // SOL_SOCKET is the socket layer
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;  // Address family
	address.sin_addr.s_addr = INADDR_ANY;  // Accept connections from any IP address, listen to all interfaces.
	address.sin_port = htons( PORT );    // Server port to open and convert to Big Endian

	// Forcefully attaching socket to the port 8000
	if (bind(serv_fd, (struct sockaddr *)&address,
				sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	// Port bind is done, wait for incoming connections and handle them.
	// First you listen(), then you accept()

	if (listen(serv_fd, 3) < 0) // 3 is the maximum size of queue
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	printf("Server up and running. Shoot your queries clienties!\n");

	while(1)
	{
		// returns a brand new socket file descriptor to use for this single accepted connection.
		// Connection established to send and receive messages.

		if ((new_sock = accept(serv_fd, (struct sockaddr *)&address,
						(socklen_t*)&addrleng))<0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}

		int f_sz, fd_req, prevv = 0, done = 0, store = 0;
		int iterCount = 0;
		int haveRead = 0;

		while(1)
		{
			memset(connec, '\0', sizeof(connec));
			// Wait till client tells you either to exit or continue

			if((done = recv(new_sock, connec, 2, 0)) < 0)
			{
				perror("Client connection");
				break;
			}

			if(connec[0] == '\0')
			{
				printf("Disconnecting with this client\n\n");
				break;
			}

			connec[done] = '\0';

			if(strcmp(connec, "0") == 0)
			{
				// client asked to exit, so server should also exit
				send(new_sock, connec, strlen(connec), 0);
				close(new_sock);
				break;
			}

			send(new_sock, connec, strlen(connec), 0);		// acknowledgement that it is running

			// read the number of files to be downloaded by server

			memset(buffer, '\0', sizeof(buffer));
			done = read(new_sock, buffer, 16384);
			buffer[done] = '\0';
			send(new_sock, buffer, strlen(buffer), 0);
			count = getNum(buffer);

			for(i=0;i<count;i++)
			{
				// read the file name by obtaining it from client
				memset(fileName, '\0', sizeof(fileName));
				name_leng = read(new_sock, fileName, 1024);
				fileName[name_leng] = '\0';

				// Check existence of file
				if(access(fileName, F_OK) != -1)
				{
					fd_req = open(fileName, O_RDONLY);

					if(fd_req < 0)
					{
						// Download not possible, file cannot be read

						strcpy(buffer, "Cannot");
						send(new_sock, buffer, strlen(buffer), 0);
						continue;
					}

					else
					{
						// Download possible, send file size to the client as acknowledgement

						f_sz = lseek(fd_req, (off_t) 0, SEEK_END);
						sprintf(buffer, "%d", f_sz);
						send(new_sock, buffer, strlen(buffer), 0);
					}
				}

				else
				{
					// Download not possible, file not present here
					strcpy(buffer, "Cannot");
					send(new_sock, buffer, strlen(buffer), 0);
					continue;
				}

				// Confirm if the download is possible on the client side

				memset(download, '\0', sizeof(download));
				done = read(new_sock, download, 2);
				download[done] = '\0';
				send(new_sock, download, strlen(download), 0);		// acknowledgement that it is running

				if(strcmp(download, "0") == 0)
					continue;					// If there's a problem with download of this file (on client side), proceed to next file

				// sending contents of file chunk by chunk

				prevv = 0;
				done = 0;
				iterCount = 0;
				store = 0;

				memset(msg, '\0', sizeof(msg));
				done = read(new_sock, msg, 32);		// wait for client to tell you to start sending data
				msg[done] = '\0';

				memset(&buffer, '\0', sizeof(buffer));

				prevv = 0;
				lseek(fd_req, (off_t)prevv, SEEK_SET);

				while ((haveRead = read(fd_req, buffer, 16384)) > 0)
				{
					if(send(new_sock, buffer, haveRead, 0) < 0)
					{
						fprintf(stderr, "Server failed to send file %s\n", fileName);
						iterCount = 1;
						break;
					}

					prevv += haveRead;
					memset(&buffer, '\0', sizeof(buffer));
					lseek(fd_req, (off_t)prevv, SEEK_SET);
				}

				close(fd_req);
				memset(msg, '\0', sizeof(msg));

				if(iterCount == 1)
				{
					perror("Client acknowledgement");
					break;
				}
		
				done = read(new_sock, msg, 32); // Receive acknowledgement

				if(done <= 0)
				{
					perror("Client acknowledgement");
					break;
				}

				msg[done] = '\0';
				send(new_sock, "cool bruh", strlen("cool bruh"), 0);	// Reply to client's thanks
			}
		}
	}

	close(serv_fd);
	return 0;
}

