#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>

#define LISTEN_BACKLOG 10
#define CLIENT_READ_SIZE 1024

// FUNCTION DECLARATIONS
int listenToConnection(int serverPort);
void handleClientConnection(int clientSocket);

// SAMPLE CALL TO INVOKE THIS PROGRAM
// ./httpserver 5000

int main(int argc, char** argv)
{
    int serverSocket, clientSocket, port;
    struct sockaddr_in httpClient;

    if(argc != 2)
    {
        printf("usage: ./httpclient <serverport>");
        return EXIT_FAILURE;
    }

    port = atoi(argv[1]);
    serverSocket = listenToConnection(port);
    if(serverSocket < 0)
    {
        printf("Failed to open port with error code : %d", serverSocket);
        return EXIT_FAILURE;
    }
    while(1)
    {
        unsigned int clientlen = sizeof(httpClient);
        // Wait for client connection
        if((clientSocket = accept(serverSocket, 
            (struct sockaddr *) &httpClient, &clientlen)) < 0)
        {
            return EXIT_FAILURE;
        }
        // Handle data from the incoming connection
        handleClientConnection(clientSocket);
    }
    return EXIT_SUCCESS;
}

// Function to create a socket descriptor to listen for connections
// Adopted from the lecture notes
int listenToConnection(int serverPort)
{
    int listenfd = -1, optval = 1;
    struct sockaddr_in serveraddr;

    // Creating a socket descriptor
    if((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        return -1;
    }

    // Creates a TCP socket
    if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,
        (const void *) &optval, sizeof(int)) < 0)
    {
        return -2;
    }

    // Creating an endpoint for all incoming connections
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)serverPort);
    
    // Binding the arguements to listenfd
    if(bind(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
    {
        return -3;
    }

    if(listen(listenfd, LISTEN_BACKLOG) < 0)
    {
        return -4;
    }
    
    return listenfd;
}

void handleClientConnection(int clientSocket)
{
    char buffer[CLIENT_READ_SIZE];
    int received = -1;
    
    // Receive data from the client
    if((received = recv(clientSocket, 
        buffer, CLIENT_READ_SIZE, 0)) < 0)
    {
        printf("Failed to receive data from the client");
    }

    char* command = strtok(buffer," ");
    if(strcmp(command, "GET") == 0)
    {
        char *filePath;
        strcpy(filePath,strtok(NULL," "));

        // Check if file can be opened
        ssize_t checkFile = open(filePath,O_RDONLY);

        // Return error is file doesnt exist
        if(errno == EACCES)
        {
            strcpy(buffer,"HTTP/1.0 403 Forbidden\r\n\r\n");
            send(clientSocket, buffer, strlen(buffer),0);
        }

        // File access error
        else if(errno == ENOENT)
        {
            strcpy(buffer,"HTTP/1.0 404 NOT FOUND\r\n\r\n");
            send(clientSocket, buffer, strlen(buffer),0);
        }
        
        // Send File if file exists
        else
        {
            strcpy(buffer,"HTTP/1.0 200 OK\r\n\r\n");
            send(clientSocket, buffer, strlen(buffer),0);
            FILE* readFile = fopen(filePath, "r");
            if(readFile == NULL)
            {
                printf("Failed to open file");
                return;
            }
            else
            {
                close(checkFile);
                ssize_t read;
                memset(buffer, '\0', strlen(buffer));
                while((fgets(buffer, sizeof(buffer), readFile)) != NULL)
                {
                    send(clientSocket, buffer, strlen(buffer), 0);
                    memset(buffer, '\0', strlen(buffer));
                }
                fclose(readFile);
            }
        }
    }
    close(clientSocket);
}