
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#define MAX_STRING_REPRESENTATION_PACKAGE 4
#define MESSAGE_SIZE 6000
extern int errno;
int port;
int sd;
struct sockaddr_in server;
int nr = 0;
char buf[MESSAGE_SIZE];
char substringBuffer[MESSAGE_SIZE];

void unpackThePackage(char recMsg[]);
void makePackage(char sendMsg[]);
char *trim(char *content);
char *substring(char *buff, int position, int length);
int checkArguments(int argc, char argValue[]);
int createEndPoint();
int bindAndConnect(char ipAddr[], char sentPort[]);
int interactWithServer();

int main(int argc, char *argv[])
{

    if (checkArguments(argc, argv[0]) != 0)
        return -1;

    if (createEndPoint() != 0)
        return -1;

    if (bindAndConnect(argv[1], argv[2]) != 0)
        return -1;

    interactWithServer();
}

int checkArguments(int argc, char argValue[])
{
    if (argc != 3)
    {
        printf("Sintax: %s <ip_address> <port>\n", argValue);
        return -1;
    }
    return 0;
}

int createEndPoint()
{
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error at socket().\n");
        return errno;
    }
    return 0;
}

int bindAndConnect(char ipAddr[], char sentPort[])
{

    port = atoi(sentPort);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ipAddr);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client]Error at connect().\n");
        return errno;
    }
    return 0;
}

int interactWithServer()
{
    while (1)
    {
        printf("[client]$ ");
        fflush(stdout);
        bzero(&buf, sizeof(buf));
        read(0, buf, sizeof(buf));
        printf("[client]What we read from cmd: %s\n", buf);
        char temp[MESSAGE_SIZE];
        strcpy(temp, buf);
        makePackage(buf);
        if (write(sd, buf, MESSAGE_SIZE) <= 0)
        {
            perror("[client]Error at write() to the server.\n");
            return errno;
        }
        // check for quitting
        if ((strcmp(trim(temp), "quit") == 0) || (strcmp(trim(temp), "q") == 0))
        {
            printf("Quitting server... Bye!\n");
            fflush(stdout);
            exit(0);
        }

        char recMessg[MESSAGE_SIZE];
        if (read(sd, recMessg, MESSAGE_SIZE) < 0)
        {
            perror("[client]Error at read() from server.\n");
            return errno;
        }
        unpackThePackage(recMessg);
        printf("[client]The received message is: %s\n", recMessg);
        fflush(stdout);
    }

    close(sd);
}

char *trim(char *content)
{
    while (isspace((unsigned char)*content))
        content++;
    char *finish;
    if (*content == 0)
        return content;

    finish = content + strlen(content) - 1;
    while (finish > content && isspace((unsigned char)*finish))
        finish--;
    finish[1] = '\0';
    return content;
}

void unpackThePackage(char recMsg[])
{
    int lengthPackage = atoi(substring(recMsg, 0, MAX_STRING_REPRESENTATION_PACKAGE));
    strcpy(recMsg, substring(recMsg, MAX_STRING_REPRESENTATION_PACKAGE, lengthPackage));
}

void makePackage(char sendMsg[])
{
    char stringLengthRepr[MESSAGE_SIZE];
    bzero(stringLengthRepr, MESSAGE_SIZE);
    int len3 = strlen(sendMsg);
    char buf_a[len3];
    sprintf(buf_a, "%d", len3);
    int delim = MAX_STRING_REPRESENTATION_PACKAGE - strlen(buf_a);
    for (int i = 0; i < delim; i++)
    {
        stringLengthRepr[i] = '0';
    }
    stringLengthRepr[delim] = '\0';
    strcat(stringLengthRepr, buf_a);
    strcat(stringLengthRepr, sendMsg);
    strcpy(sendMsg, stringLengthRepr);
    len3 = strlen(sendMsg);
    bzero(buf_a, len3);
    bzero(stringLengthRepr, MESSAGE_SIZE);
}

char *substring(char *buff, int position, int length)
{
    memcpy(substringBuffer, &buff[position], length);
    substringBuffer[length] = '\0';
    return substringBuffer;
}
