#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <map>
#include <stack>
#include <set>

#define MAX_STRING_REPRESENTATION_PACKAGE 4
#define PORT 2908
#define MESSAGE_SIZE 6000
char substringBuffer[MESSAGE_SIZE];

extern int errno;

typedef struct thData
{
    int idThread;
    int cl;
} thData;

char *substring(char *buff, int position, int length)
{
    memcpy(substringBuffer, &buff[position], length);
    substringBuffer[length] = '\0';
    return substringBuffer;
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

void readFileServer(char sendMsg[], char file[]);
void checkAndLogout(int idThread);
int checkIfNotQuitting(char recMsg[], int idThread);
void unpackThePackage(char recMsg[]);
void makePackage(char sendMsg[]);
int sendBackToclient(char sendMsg[], thData tdL);
void logoutDispatcher(char recMsg[], char sendMsg[], int id);
void authenticateDispatcher(char recMsg[], char sendMsg[], int id);
void logoutTrain(char recMsg[], char sendMsg[], int id);
void authenticateTrain(char recMsg[], char sendMsg[], int id);
void authenticateAll(char recMsg[], char sendMsg[], int id);
void startReminder(char sendMsg[]);
void reset(char recMsg[], char sendMsg[], int id);
void undo(char recMsg[], char sendMsg[], int id);
void delay(char recMsg[], char sendMsg[], int id);
void analyzeTrainCommand(char recMsg[], char sendMsg[], int id);
void analyzeDispatcherCommand(char recMsg[], char sendMsg[], int id);
void analyzeCmd(char recMsg[], char sendMsg[], int id);
int interactWithClient(void *arg);
int serveClients();
int listenToNewConnections();
int bindStructures();
int createEndpoint();
int turnServerOn();
