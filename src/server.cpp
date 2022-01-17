#include "server.h"
#include "Train.h"
struct sockaddr_in server;
struct sockaddr_in from;
int sd;
int thNumber = 0;
int dispatcherFlag = -1;
int loginNumber[100];
Train t[100];
pthread_t th[100];
std::map<int, int> mapThToNr;
std::stack<TrainRemoteControl *> queueCmd[100];

int main()
{
    if (turnServerOn() == -1)
        return -1;
};

void checkAndLogout(int idThread)
{
    if (dispatcherFlag == idThread)
    {
        dispatcherFlag = -1;
    }
    if ((mapThToNr[idThread] >= 0) && (t[mapThToNr[idThread]].getThreadId() == idThread))
    {
        t[mapThToNr[idThread]].setLoginFlag(0);
        t[mapThToNr[idThread]].setThreadId(-1);
        t[mapThToNr[idThread]].resetVariables();
        int trainNumber = mapThToNr[idThread];
        Train::removeConnectedTrain(trainNumber);
        mapThToNr[idThread] = -1;
    }
}

int checkIfNotQuitting(char recMsg[], int idThread)
{
    if ((strcmp(trim(recMsg), "quit") == 0) || (strcmp(trim(recMsg), "q") == 0))
    {
        checkAndLogout(idThread);
        printf("[Thread %d] Quitting server...\n", idThread);
        return -1;
    }
    return 0;
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

void readFileServer(char sendMsg[], char file[])
{
    char buffer[MESSAGE_SIZE];
    bzero(buffer, MESSAGE_SIZE);
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t trav;

    fp = fopen(file, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((trav = getline(&line, &len, fp)) != -1)
    {
        sprintf(buffer, "%s", line);
        strcat(sendMsg, buffer);
    }
    fclose(fp);
    if (line)
        free(line);

    bzero(buffer, MESSAGE_SIZE);
}

int sendBackToclient(char sendMsg[], thData tdL)
{
    printf("[Thread %d]Sending the message back...%s\n", tdL.idThread, sendMsg);
    fflush(stdout);
    makePackage(sendMsg);
    if (write(tdL.cl, sendMsg, MESSAGE_SIZE) <= 0)
    {
        checkAndLogout(tdL.idThread);
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Error at write() to client.\n");
        fflush(stdout);
        return -1;
    }
    else
        printf("[Thread %d]The message was sent successfully.\n", tdL.idThread);
    fflush(stdout);
    return 0;
}

void logoutDispatcher(char recMsg[], char sendMsg[], int id)
{
    strcpy(sendMsg, "Dispatcher disconnected");
    dispatcherFlag = -1;
}

void authenticateDispatcher(char recMsg[], char sendMsg[], int id)
{
    if ((mapThToNr[id] < 0) || (Train::checkIfSetOfConTrainsEmpty()))
    {
        if (dispatcherFlag == -1)
        {
            dispatcherFlag = id;
            strcpy(sendMsg, "Dispatcher connected");
        }
        else if (dispatcherFlag == id)
        {
            strcpy(sendMsg, "You're already connected on the current thread");
        }
        else
        {
            sprintf(sendMsg, "Dispatcher already in use on thread %d. Please wait", dispatcherFlag);
        }
    }
    else
    {
        strcpy(sendMsg, "You may be already authenticated.");
    }
}

void logoutTrain(char recMsg[], char sendMsg[], int id)
{
    sprintf(sendMsg, "You've logged out, train %d", t[mapThToNr[id]].getTrainIdInList());
    checkAndLogout(id);
}

void authenticateTrain(char recMsg[], char sendMsg[], int id)
{
    unsigned int trainNumber = atoi(substring(recMsg, 13, 6));
    if (dispatcherFlag != id)
    {
        switch (t[mapThToNr[id]].getLoginFlag())
        {
        case 0:
            if (trainNumber < Train::getNumberOfTrains())
            {
                if (Train::findIfElmInSet(trainNumber))
                {
                    sprintf(sendMsg, "Train%d already connected. Please wait", trainNumber);
                }
                else
                {
                    t[trainNumber].setLoginFlag(1);
                    t[trainNumber].setTrainIdInList(trainNumber);
                    t[trainNumber].setThreadId(id);
                    mapThToNr[id] = trainNumber;
                    Train::addConnectedTrain(trainNumber);
                    sprintf(sendMsg, "Train%d authenticated", trainNumber);
                }
            }
            else
            {
                sprintf(sendMsg, "Couldn't find train %d", trainNumber);
            }
            break;
        case 1:
            sprintf(sendMsg, "You're already logged in as train%d", t[mapThToNr[id]].getTrainIdInList());
            break;
        default:
            strcpy(sendMsg, "Train uthentication impossible. Failure.");
        }
    }
    else
    {
        strcpy(sendMsg, "Probably you're already connected. Try logout.");
    }
}

void authenticateAll(char recMsg[], char sendMsg[], int id)
{
    bzero(sendMsg, MESSAGE_SIZE);
    if (strcmp(substring(recMsg, 0, 13), "login : train") == 0)
    {
        authenticateTrain(recMsg, sendMsg, id);
    }
    else if (strcmp("login : dispatcher", trim(recMsg)) == 0)
    {
        authenticateDispatcher(recMsg, sendMsg, id);
    }
    else
    {
        strcpy(sendMsg, "login attempt failed, you entered an inexistent name");
    }
}

void startReminder(char sendMsg[])
{
    strcpy(sendMsg, "Wait, this train has not started yet");
}

void reset(char recMsg[], char sendMsg[], int id)
{
    if (t[mapThToNr[id]].getStartFlag() == 1)
    {
        while (!queueCmd[id].empty())
        {
            queueCmd[id].pop();
        }
        Train *ptrTrain;
        ptrTrain = &t[mapThToNr[id]];
        TrainDelayCommand *delaySet = new TrainDelayCommand(ptrTrain, -t[mapThToNr[id]].getDelay());
        TrainRemoteControl *control = new TrainRemoteControl;
        control->setCommand(delaySet);
        control->pressButton();
        std::string s = t[mapThToNr[id]].outputInRealTimeStationOfTrain(sendMsg);
    }
    else
    {
        startReminder(sendMsg);
    }
}

void undo(char recMsg[], char sendMsg[], int id)
{
    if (t[mapThToNr[id]].getStartFlag() == 1)
    {
        if (!queueCmd[id].empty())
        {
            queueCmd[id].top()->undoButton();
            queueCmd[id].pop();
        }
        std::string s = t[mapThToNr[id]].outputInRealTimeStationOfTrain(sendMsg);
    }
    else
    {
        startReminder(sendMsg);
    }
}

void delay(char recMsg[], char sendMsg[], int id)
{
    if (t[mapThToNr[id]].getStartFlag() == 1)
    {
        int delay = atoi(substring(recMsg, 5, 11));
        Train *ptrTrain;
        ptrTrain = &t[mapThToNr[id]];
        TrainDelayCommand *delaySet = new TrainDelayCommand(ptrTrain, delay);
        // invoker objects
        TrainRemoteControl *control = new TrainRemoteControl;
        // execute
        control->setCommand(delaySet);
        control->pressButton();
        queueCmd[id].push(control);
        std::string s = t[mapThToNr[id]].outputInRealTimeStationOfTrain(sendMsg);
    }
    else
    {
        startReminder(sendMsg);
    }
}

void analyzeTrainCommand(char recMsg[], char sendMsg[], int id)
{
    bzero(sendMsg, MESSAGE_SIZE);
    if (strcmp(trim(recMsg), "logout") == 0)
    {
        logoutTrain(recMsg, sendMsg, id);
    }
    else if ((strcmp(trim(recMsg), "start") == 0) && (t[mapThToNr[id]].getStartFlag() != 1))
    {
        std::string s = t[mapThToNr[id]].startTrain();
        strncpy(sendMsg, s.c_str(), MESSAGE_SIZE);
    }
    else if (strcmp(substring(recMsg, 0, 5), "delay") == 0)
    {
        delay(recMsg, sendMsg, id);
    }
    else if (strcmp(substring(recMsg, 0, 4), "undo") == 0)
    {
        undo(recMsg, sendMsg, id);
    }
    else if (strcmp(substring(recMsg, 0, 5), "reset") == 0)
    {
        reset(recMsg, sendMsg, id);
    }
    else
    {
        strcpy(sendMsg, "Inexistent command for trains or unavaiable for you now. Try \"m\" or \"man\" for help.");
    }
}

void analyzeDispatcherCommand(char recMsg[], char sendMsg[], int id)
{
    if (strcmp("logout", trim(recMsg)) == 0)
    {
        logoutDispatcher(recMsg, sendMsg, id);
    }
    else if (strcmp(substring(recMsg, 0, 14), "get line train") == 0)
    {
        int trainNr = atoi(substring(recMsg, 14, 7));
        if (trainNr < Train::getNumberOfTrains())
        {
            std::string s = t[trainNr].getBasicRoute();
            strncpy(sendMsg, s.c_str(), MESSAGE_SIZE);
        }
        else
        {
            sprintf(sendMsg, "Couldn't find train %d, check out \"get info trains\" for help.", trainNr);
        }
    }
    else if (strcmp(substring(recMsg, 0, 16), "get agenda train") == 0)
    {
        int trainNr = atoi(substring(recMsg, 16, 7));
        if (trainNr < Train::getNumberOfTrains())
        {
            t[trainNr].fetchSituationOnTrains(trainNr);
            std::string s = t[trainNr].getRoute();
            strncpy(sendMsg, s.c_str(), MESSAGE_SIZE);
        }
        else
        {
            sprintf(sendMsg, "Couldn't find train %d, check out \"get info trains\" for help.", trainNr);
        }
    }
    else if (strcmp("get -today -all", trim(recMsg)) == 0)
    {
        for (int i = 0; i < Train::getNumberOfTrains(); i++)
        {
            strcat(sendMsg, "\n");
            strcat(sendMsg, t[i].getCurrentStationAndTimeLeavingsToday().c_str());
            if (strlen(trim(sendMsg)) == 0)
                strcpy(sendMsg, "If no trains running this hour, check schedule with \"get -all\"");
        }
    }
    else if (strcmp("get -hour -leavings", trim(recMsg)) == 0)
    {
        for (int i = 0; i < Train::getNumberOfTrains(); i++)
        {
            strcat(sendMsg, "\n");
            strcat(sendMsg, t[i].getCurrentStationAndTimeLeavingsHour().c_str());
            if (strlen(trim(sendMsg)) == 0)
                strcpy(sendMsg, "If no trains running this hour, check schedule with \"get -all\"");
        }
    }
    else if (strcmp("get -hour -arrivals", trim(recMsg)) == 0)
    {
        for (int i = 0; i < Train::getNumberOfTrains(); i++)
        {
            strcat(sendMsg, "\n");
            strcat(sendMsg, t[i].getCurrentStationAndTimeArrivalsHour().c_str());
            if (strlen(trim(sendMsg)) == 0)
                strcpy(sendMsg, "If no trains running this hour, check schedule with \"get -all\"");
        }
    }
    else if (strcmp("get -all", trim(recMsg)) == 0)
    {
        for (int i = 0; i < Train::getNumberOfTrains(); i++)
        {
            strcat(sendMsg, "\n");
            strcat(sendMsg, t[i].getCurrentStationAndTimeLeavingsGeneral().c_str());
        }
    }

    else if (strcmp("get info trains", trim(recMsg)) == 0)
    {
        for (int i = 0; i < Train::getNumberOfTrains(); i++)
        {
            strcat(sendMsg, "\n");
            strcat(sendMsg, t[i].getTrainSettings().c_str());
        }
    }
    else if (strcmp(substring(recMsg, 0, 14), "get info train") == 0)
    {
        int trainNr = atoi(substring(recMsg, 14, 7));
        if (trainNr < Train::getNumberOfTrains())
        {
            std::string s = t[trainNr].getTrainSettings();
            strncpy(sendMsg, s.c_str(), MESSAGE_SIZE);
        }
        else
        {
            sprintf(sendMsg, "Couldn't find train %d", trainNr);
        }
    }
    else
    {
        strcpy(sendMsg, "Inexistent command on dispatcher. Try \"m\" or \"man\" for help.");
    }
}

void analyzeCmd(char recMsg[], char sendMsg[], int id)
{
    bzero(sendMsg, MESSAGE_SIZE);
    if (strcmp(trim(recMsg), "m") == 0)
    {
        readFileServer(sendMsg, (char *)"files/man.txt");
    }
    else if ((strcmp(trim(recMsg), "man") == 0) || (strcmp(trim(recMsg), "manual") == 0))
    {
        readFileServer(sendMsg, (char *)"files/manual.txt");
    }
    else if (strcmp(substring(recMsg, 0, 5), "login") == 0)
    {
        authenticateAll(recMsg, sendMsg, id);
    }
    else if ((mapThToNr[id] >= 0) && (t[mapThToNr[id]].getThreadId() == id) && (dispatcherFlag != id))
    {
        analyzeTrainCommand(recMsg, sendMsg, id);
    }
    else if ((dispatcherFlag == id))
    {
        analyzeDispatcherCommand(recMsg, sendMsg, id);
    }
    else
    {
        strcpy(sendMsg, "\nInexistent command or access denied. Try \"m\" or \"man\" for help.");
    }
}

int interactWithClient(void *arg)
{

    char recMsg[MESSAGE_SIZE], sendMsg[MESSAGE_SIZE];
    struct thData tdL;
    tdL = *((struct thData *)arg);

    // read from client
    int readFlag = read(tdL.cl, recMsg, MESSAGE_SIZE);
    if (readFlag <= 0)
    {
        checkAndLogout(tdL.idThread);
        perror("Quitting now\n");
        fflush(stdout);
        return -1;
    }
    unpackThePackage(recMsg);
    printf("[Thread %d]Message was recieved...%s\n", tdL.idThread, recMsg);

    if (checkIfNotQuitting(recMsg, tdL.idThread) == -1)
    {
        if (t[tdL.idThread].getLoginFlag() == 1)
        {
            logoutTrain(recMsg, sendMsg, tdL.idThread);
        }
        return -1;
    }

    analyzeCmd(recMsg, sendMsg, tdL.idThread);

    if (sendBackToclient(sendMsg, tdL) == -1)
        return -1;

    return 0;
}

static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[thread]- %d - Waiting for the message...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());
    while (1)
    {
        if (interactWithClient((struct thData *)arg) == -1)
        {
            close((intptr_t)arg);
            printf("Client with Thread %d *Quitted* the server...\n\n", tdL.idThread);
            return (NULL);
        }
    }
};

void initializeDataForStart()
{
    dispatcherFlag = -1;
    Train::checkXmlFile("files/scheduleTrains.xml");
    Train::traverseDocument();
    for (int i = 0; i < Train::getNumberOfTrains(); i++)
    {
        t[i].setThreadId(-1);
        t[i].initializeVariables(i);
    }
    for (int i = 0; i < 100; i++)
    {
        mapThToNr[i] = -1;
    }
    mapThToNr[-1] = -10;
}

int serveClients()
{
    initializeDataForStart();
    while (1)
    {
        int client;
        thData *td;
        socklen_t length = sizeof(from);

        printf("[server]We're waiting at the port %d...\n", PORT);
        fflush(stdout);

        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[server]Error at accept().\n");
            continue;
        }
        // idThread - the id of the thread
        // cl - descriptor returned by accept
        td = (struct thData *)malloc(sizeof(struct thData));
        td->idThread = thNumber++;
        td->cl = client;

        pthread_create(&th[thNumber], NULL, &treat, td);
    }
}

int listenToNewConnections()
{
    if (listen(sd, 5) == -1)
    {
        perror("[server]Error at listen().\n");
        return errno;
    }
    return 0;
}

int bindStructures()
{
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Error at bind().\n");
        return errno;
    }
    return 0;
}

int createEndpoint()
{
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Error at socket().\n");
        return errno;
    }
    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    return 0;
}

int turnServerOn()
{

    if (createEndpoint() != 0)
        return -1;

    if (bindStructures() != 0)
        return -1;

    if (listenToNewConnections() != 0)
        return -1;
    serveClients();
    return 0;
}
