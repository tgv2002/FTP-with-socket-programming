// Client side
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#define PORT 8000
#define YELLOW  "\x1B[33m"
#define RES "\x1B[0m"

char* fileName[128];
char iters[32];
char msg[32];
char buffer[16384];
char compl[128];
char connec[2];
char download[2];

void handleCrash(int signum)
{
	fprintf(stderr, "\nServer got shut down\n");
	exit(EXIT_FAILURE);
}

void ctrlc(int signum)
{
	fprintf(stderr, "\nClient can't exit this way, try 'exit' command\n");
	fprintf(stderr, YELLOW "Client> " RES);
}

int getNum(char buffer[16384])
{
	int leng = strlen(buffer);
	int val = 0, dig;
	int neg = 0;

	for(int i=0;i<leng;i++)
	{
		if(buffer[i] >= '0' && buffer[i] <= '9')
		{
			dig = (int)(buffer[i] - '0');
			val = (val * 10) + dig;
		}

		else
		{
			   neg = 1;
		}
	}

	if(neg == 1)
		val *= (-1);
	return val;
}

void printPrompt()
{
	printf(YELLOW "Client> " RES);
}

int main(int argc, char const *argv[])
{
	signal(SIGPIPE, handleCrash);
	signal(SIGINT, ctrlc);
	struct sockaddr_in address;
	int sock = 0, readContent;
	struct sockaddr_in server_addr;
	int ctr = 0;
	char* cmd;
	cmd = (char *) malloc(1024 * sizeof(char));
	int j;
	size_t lengy;
	char* tokenized = (char *) malloc(1024 * sizeof(char));
	int c = 0;

	for(int i=0;i<128;i++)
	{
		fileName[i] = (char *) malloc(1024 * sizeof(char));
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}

	memset(&server_addr, '0', sizeof(server_addr)); // Ensure the struct is empty
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	// Converts an IP address in numbers-and-dots notation

	if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr)<=0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)  // connect to the server address
	{
		printf("\nConnection Failed \n");
		return -1;
	}

	int f_sz, fd_req, prevv = 0, done = 0, store = 0;
	long double num, den, percent;
	int iterCount = 0;
	int haveRead = 0;
	int readThis = 0;

	while(1)
	{
		printPrompt();
		lengy = 0;
		getline(&cmd, &lengy, stdin);

		tokenized = strtok(cmd, " \t\n");
		ctr = 0;
		c = 0;

		while(tokenized != NULL) 
		{ 
			ctr++;

			if(ctr >= 128)
			{
				printf("TOO MANY ARGUMENTS !!!!!!!!!!!!!!!!!!\n");
				printf("Only the first 127 files are considered\n");
				break;
			}

			if(ctr == 1)
			{
				if(strcmp("exit", tokenized) == 0)
				{
					// exit connections, tell server to exit too
					sprintf(connec, "%d", 0);
					send(sock, connec, strlen(connec), 0);
					memset(connec, '\0', sizeof(connec));
					read(sock, connec, 2);
					close(sock);
					return 0;
				}

				else if(strcmp("get", tokenized) != 0)
				{
					// Invalid command, so needn't tell server to exit
					sprintf(connec, "%d", 1);
					send(sock, connec, strlen(connec), 0);
					memset(connec, '\0', sizeof(connec));
					read(sock, connec, 2); 
					printf("ENTER A VALID COMMAND !!!!!!!!!!!!!!!!!!!!\n");
					break;
				}

				// Valid command, so needn't tell server to exit
				sprintf(connec, "%d", 1);
				send(sock, connec, strlen(connec), 0);
				memset(connec, '\0', sizeof(connec));
				read(sock, connec, 2);     
			}

			else
			{
				// Copy the argument (filename) into separate array
				strcpy(fileName[c], tokenized);
				c++; 
			}

			tokenized = strtok(NULL, " \t\n"); 
		}

		if(ctr == 0)
		{
			// If nothing was entered as command, server needn't exit, and we needn't worry as c=0 and further steps won't execute anyway
			sprintf(connec, "%d", 1);
			send(sock, connec, strlen(connec), 0);
			memset(connec, '\0', sizeof(connec));
			read(sock, connec, 2); 	
		}

		// Let the server know the number of files to be downloaded

		sprintf(iters, "%d", c);
		send(sock, iters, strlen(iters), 0);
		memset(iters, '\0', sizeof(iters));
		done = read(sock, iters, 32);
		iters[done] = '\0';

		for(j=0;j<c;j++)
		{
			// Sends server name of file

			send(sock, fileName[j], strlen(fileName[j]), 0);
			memset(buffer, '\0', sizeof(buffer));
			done = read(sock, buffer, 16384);
			buffer[done] = '\0';

			// Server sends file size if file can be downloaded, otherwise, an error message

			if(strcmp(buffer, "Cannot") == 0)
			{
				printf("File %s cannot be provided by the server\n\n", fileName[j]);   
				continue;
			}

			f_sz = getNum(buffer);

			// create the file on client side or overwrite if exists 

			fd_req = open(fileName[j], O_WRONLY | O_APPEND |
					O_CREAT | O_TRUNC, (mode_t)0600);

			// Let the server know if there is a problem on the client side and it can't download

			if(fd_req < 0)
			{
				sprintf(download, "%d", 0);
				send(sock, download, strlen(download), 0);
				memset(download, '\0', sizeof(download));
				read(sock, download, 2); 
				printf("File %s cannot be downloaded\n\n", fileName[j]);
				continue;
			}
			// Download is possible on the client side

			sprintf(download, "%d", 1);
			send(sock, download, strlen(download), 0);
			memset(download, '\0', sizeof(download));
			read(sock, download, 2); 

			// Read all contents of file now, sent by server. Downloading starts here

			prevv = 0;
			done = 0;
			iterCount = 0;
			store = 0;

			// Tell server to start sending contents
			sprintf(msg, "Send next chunk...");
			send(sock, msg, strlen(msg), 0);

			memset(&buffer, '\0', sizeof(buffer));
			prevv = 0;
			printf("\n");

			while ((haveRead = recv(sock, buffer, 16384, 0)) > 0)
			{
				store = write(fd_req, buffer, haveRead);		// write(append) those contents to file we just created in our directory
				prevv += haveRead;
				
				//printing percentage
				num = (long double)prevv;
				den = (long double)f_sz;
				percent = 100.0 * (((long double)1.0 * num) / den);
				if((prevv) >= f_sz)
					percent = (long double)100;
				sprintf(compl, "\rDownloading %s: %Lf%% completed.....", fileName[j] , percent);
				write(1, compl, (size_t)strlen(compl));

				if (store < haveRead)
				{
					fprintf(stderr, "\nFile write failed\n");
					break;
				}

				memset(&buffer, '\0', sizeof(buffer));

				if(prevv == f_sz)
				{
					break;
				}
			}

			if(haveRead < 0)
			{
				printf("\n");
				perror("read");
				fprintf(stderr, "\nFile %s might be corrupted..\n", fileName[j]);
			}
			
			sprintf(msg, "Thanks myann!...");	// Thank server for helping you download
			send(sock, msg, strlen(msg), 0);
			memset(msg, '\0', sizeof(msg));
			done = read(sock, msg, 32);			// Wait for server to reply
			msg[done] = '\0';

			printf("\n");
			printf("%s downloaded successfully\n\n", fileName[j]);
			close(fd_req);						// close the downloaded file
		}
	}

	return 0;
}
