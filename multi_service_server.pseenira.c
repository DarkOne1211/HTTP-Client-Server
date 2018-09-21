#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define LISTEN_BACKLOG 10
#define CLIENT_READ_SIZE 1024
#define UDP_PING_SIZE 68

// FUNCTION DECLARATIONS
int handleUDPConnection(struct sockaddr_in UDPClient, int udpSocket);
int handleTCPRequest(struct sockaddr_in httpClient, int serverSocket);
int listenToConnection(int serverPort, bool isUDP);
void handleClientConnection(int clientSocket);

// SAMPLE CALL TO INVOKE THIS PROGRAM
// ./multiservice_server 5000 5001

int main(int argc, char** argv)
{
    int udpSocket, serverSocket, port, udpPort;
    struct sockaddr_in httpClient, UDPClient;

    if(argc != 3)
    {
        printf("usage: ./httpclient <serverport> <udp serverport>\n");
        return EXIT_FAILURE;
    }

    // Building the socket and binding the connection

    port = atoi(argv[1]);
    udpPort = atoi(argv[2]);
    serverSocket = listenToConnection(port, false);
    udpSocket = listenToConnection(udpPort, true);

    if(serverSocket < 0 || udpSocket < 0)
    {
        printf("Failed to open ports with error code : %d\n", serverSocket);
        return EXIT_FAILURE;
    }
    
    // Selecting between handling TCP and UDP
    fd_set rdfs;
    int selectRet, max;
    while(1)
    {
        FD_ZERO(&rdfs);
        FD_SET(serverSocket,&rdfs);
        FD_SET(udpSocket,&rdfs);
        max = (serverSocket >= udpSocket) ? serverSocket + 1 : udpSocket + 1;
        if((selectRet = select(max, &rdfs, NULL, NULL, NULL)) == -1)
        {
            printf("select command failed with error code %d",errno);
            return EXIT_FAILURE;
        }

        // if TCP connection
        if(FD_ISSET(serverSocket,&rdfs))
        {
            handleTCPRequest(httpClient, serverSocket);
        }
        // if UDP connection
        else if(FD_ISSET(udpSocket,&rdfs))
        {
            handleUDPConnection(UDPClient, udpSocket);
        }
        // error if neither
        else
        {
            printf("Error while setting the socket");
            return EXIT_FAILURE;
        }
        
    }
    return EXIT_SUCCESS;
}

// Handles the UDP connection
int handleUDPConnection(struct sockaddr_in UDPClient, int udpSocket)
{
    char buffer[UDP_PING_SIZE];
    bzero((char *) &buffer, sizeof(buffer));
    int recvLen, sendLen;
    int clientlen = sizeof(UDPClient);
    recvLen = recvfrom(udpSocket, buffer, UDP_PING_SIZE, 0, 
        (struct sockaddr *) &UDPClient, &clientlen);
	buffer[3]++;
	sendLen = sendto(udpSocket, buffer, recvLen, 0,
		(struct sockaddr *) &UDPClient, clientlen);

}

// Handles the TCP connection
int handleTCPRequest(struct sockaddr_in httpClient, int serverSocket)
{
    int clientSocket = -1;
    unsigned int clientlen = sizeof(httpClient);
    // Wait for client connection
    if((clientSocket = accept(serverSocket, 
        (struct sockaddr *) &httpClient, &clientlen)) < 0)
    {
        printf("Failed to receive data from the client\n");
        return EXIT_FAILURE;
    }
    // Handle data from the incoming connection
    if(fork() == 0)
    {
        close(serverSocket);
        handleClientConnection(clientSocket);
        exit(0);
    }
    close(clientSocket);
    return EXIT_SUCCESS;
}

// Function to create a socket descriptor to listen for connections
// Adopted from the lecture notes
int listenToConnection(int serverPort, bool isUDP)
{
    int listenfd = -1, optval = 1;
    struct sockaddr_in serveraddr;

    // Creating a socket descriptor
    if(isUDP)
    {
        if((listenfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
        {
            return -1;
        }
    }
    else
    {
        if((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
        {
            return -1;
        }
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

    if(isUDP)
    {
        return listenfd;
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
        printf("Failed to receive data from the client\n");
    }

    char* command = strtok(buffer," ");
    if(strcmp(command, "GET") == 0)
    {
        char *filePath = strtok(NULL," ");
        char openFilePath[1024];
        openFilePath[0] = '.';
        openFilePath[1] = '\0';
        if(filePath[0] == '/')
        {
            strcat(openFilePath,filePath);
        }
        else
        {
            strcpy(openFilePath,filePath);  
        }
        // Check if file can be opened
        ssize_t checkFile = open(openFilePath,O_RDONLY);

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
            close(checkFile);
            strcpy(buffer,"HTTP/1.0 200 OK\r\n\r\n");
            send(clientSocket, buffer, strlen(buffer),0);
            FILE* readFile = fopen(openFilePath, "r");
            if(readFile == NULL)
            {
                printf("Failed to open file\n");
                return;
            }
            else
            {
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
