#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

#define RECV_BUFF_SIZE 1024
// FUNCTION DECLARATIONS
int connectToServer(char* serverName, int serverPort);

// SAMPLE CALL TO INVOKE THIS PROGRAM
// ./httpclient dtunes.ecn.purdue.edu 80 /ece463/lab1/test_short.txt

int main(int argc, char *argv[])
{
    if(argc < 4)
    {
        printf("usage: ./httpclient <servername> <serverport> <pathname>");
        return EXIT_FAILURE;
    }

    // Storing the server name and the port number
    char *serverName = argv[1];
    int serverPort = atoi(argv[2]);

    // Storing the file path on the server
    char *filepath = argv[3];
    
    // Creating the get request string
    // SAMPLE GET Request
    // “GET /ece463/lab1/test_short.txt HTTP/1.0\r\n\r\n”.
    char *httpRequest = "GET ";
    char *httpVersion = " HTTP/1.0\r\n\r\n";
    char getRequest[strlen(httpRequest) + strlen(filepath) + strlen(httpVersion) + 1];
    strcpy(getRequest, httpRequest);
    strcat(getRequest, filepath);
    strcat(getRequest, httpVersion);
    
    // Set up a connection to the server
    int connectionNumber = connectToServer(serverName, serverPort);
    if(connectionNumber < 0) // Failed to open the connection
    {
        printf("Opening a connection failed with error no : %d", connectionNumber);
        return EXIT_FAILURE;
    }
    // Sending the GET Request to the server
    if(send(connectionNumber,getRequest,strlen(getRequest),0) != strlen(getRequest))
    {
        printf("Failed to send the GET request to the server");
        close(connectionNumber);
        return EXIT_FAILURE;
    }

    // Receiving data back from the server
    int bytesRecv = 0;
    char recvBuffer[RECV_BUFF_SIZE];
    while((bytesRecv = recv(connectionNumber,recvBuffer,RECV_BUFF_SIZE - 1,0)) > 0)
    {
        recvBuffer[bytesRecv] = '\0';
        printf("%s", recvBuffer);
    }
    // FREE ALL HEAP SPACE
    close(connectionNumber);
    return EXIT_SUCCESS;
}

// Function to open a connection to the server
// Adapted from lecture slides available on blackboard

int connectToServer(char* serverName, int serverPort)
{
    int clientfd;
    struct hostent *host;
    struct sockaddr_in serverAd;

    // Creating socket?
    if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return -1;
    }
    if((host = gethostbyname(serverName)) == NULL)
    {
        return -2;
    }
    
    // Fill up struct with server details
    bzero((char* ) &serverAd, sizeof(serverAd));
    serverAd.sin_family = AF_INET;
    bcopy((char *)host->h_addr_list[0], (char* )&serverAd.sin_addr.s_addr, host->h_length);
    serverAd.sin_port = htons(serverPort);

    // Trying to etablish a connection
    if(connect(clientfd, (struct sockaddr *) &serverAd, sizeof(serverAd)) < 0)
    {
        return -1;
    }
    return clientfd;
}