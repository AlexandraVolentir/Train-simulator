#include "files/pugixml.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <random>
#include <ctime>
#include <cerrno>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <cstddef>
#define LOWER_LIMIT 2
#define UPPER_LIMIT 10
#define MESSAGE_SIZE 6000
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

class Command
{
public:
    virtual void execute() = 0;
    virtual void unexecute() = 0;
};

class Train
{
private:
    std::string route;
    std::string basicRoute;
    std::string routeOrigDest;
    std::string currentStationAndTime;
    std::string nameStations[100];
    std::string destinationStations[100];
    tm realTimeimeArrivals[100];
    tm realTimeLeavings[100];
    std::string timeArrivals[100];
    std::string timeLeavings[100];
    std::string dateAndTime[100];
    std::string kmTrain, lengthTrain, operatorTrain;
    std::string tonnage, ownerTrain, rankTrain, trainCode;
    std::string trainSettings;
    int loginFlag = 0;
    unsigned trainIdInList;
    int threadId = -1;
    int estimatedFinalArr = -1;
    int delay = 0;
    int vectorDelays[100];
    int traverseTimeSt[100];
    int baseTraverseSt[100];
    int curStation;
    int startFlag = 0;
    int stopFlag = 0;
    int numberOfStations;
    char checkTraversal[100];
    static int xmlCheck;
    static pugi::xml_document doc;
    static std::map<int, pugi::xml_node> trainMapIdToNodes;
    static const int numberOfTrains = 44;

public:
    static std::set<int> setOfConnectedTrains;
    Train() {}
    Train(int number)
    {
        this->trainIdInList = number;
    }
    int getDelay()
    {
        return delay;
    }
    void setCurStation(int st)
    {
        curStation = st;
    }
    int getCurStation()
    {
        return curStation;
    }
    void setThreadId(int id)
    {
        threadId = id;
    }
    int getThreadId()
    {
        return this->threadId;
    }
    void setLoginFlag(int login)
    {
        if (login == 0 || login == 1)
            loginFlag = login;
    }
    int getLoginFlag()
    {
        return this->loginFlag;
    }
    int setTrainIdInList(unsigned id)
    {
        if (id <= numberOfTrains)
        {
            this->trainIdInList = id;
            return id;
        }
        return -1;
    }
    int getTrainIdInList()
    {
        return this->trainIdInList;
    }
    void setStartFlag(int i)
    {
        this->startFlag = i;
    }
    int getStartFlag()
    {
        return this->startFlag;
    }
    int getNumberOfStations()
    {
        return numberOfStations;
    }
    std::string getTrainSettings()
    {
        return trainSettings;
    }
    static void addConnectedTrain(int elm)
    {
        pthread_mutex_lock(&lock);
        setOfConnectedTrains.insert(elm);
        pthread_mutex_unlock(&lock);
    }
    static void removeConnectedTrain(int elm)
    {
        pthread_mutex_lock(&lock);
        setOfConnectedTrains.erase(elm);
        pthread_mutex_unlock(&lock);
    }
    static int getNumberOfConnectedTrains()
    {
        return setOfConnectedTrains.size();
    }
    static int checkIfSetOfConTrainsEmpty()
    {
        return setOfConnectedTrains.empty();
    }
    static int findIfElmInSet(int elm)
    {
        if (setOfConnectedTrains.find(elm) != setOfConnectedTrains.end())
        {
            return 1; // present in set
        }
        return 0;
    }
    int convertCharToTime(const char *seconds)
    {
        return 0;
    }
    std::string getRoute()
    {
        return this->route;
    }
    std::string getBasicRoute()
    {
        return this->basicRoute;
    }
    int calcFinalArrivalTime()
    {
        return 1;
    }
    std::string getRouteOrgDest()
    {
        return this->routeOrigDest;
    }
    std::string trim(const std::string &s)
    {
        // https://www.techiedelight.com/trim-string-cpp-remove-leading-trailing-spaces/
        auto start = s.begin();
        while (start != s.end() && std::isspace(*start))
        {
            start++;
        }
        auto end = s.end();
        do
        {
            end--;
        } while (std::distance(start, end) > 0 && std::isspace(*end));
        return std::string(start, end + 1);
    }
    time_t currentTime()
    {
        return time(NULL);
    }
    std::string timeToString(time_t time)
    {
        return trim(std::to_string(time));
    }
    std::string convertTmToString(tm time)
    {
        char actualTime[150];
        sprintf(actualTime, "%s\n", asctime(&time));
        std::string timeThen(actualTime);
        timeThen = trim(timeThen);
        return timeThen;
    }
    tm addSecondsToTime(time_t now, int seconds)
    {
        struct tm nowTime = *localtime(&now);
        struct tm delayedTime = nowTime;
        delayedTime.tm_min += seconds;
        mktime(&delayedTime);
        return delayedTime;
    }
    void delayValue(int value)
    {
        delay += value;
        traverseTimeSt[curStation + 1] += delay;
        time_t temp = currentTime();
        dateAndTime[curStation + 1] = timeToString(temp);
        int sum = 0;
        for (int i = curStation + 1; i < numberOfStations; i++)
        {
            sum += traverseTimeSt[i];
            vectorDelays[i] = sum;

            realTimeLeavings[i] = addSecondsToTime(temp, vectorDelays[i]);
            realTimeimeArrivals[i] = addSecondsToTime(temp, vectorDelays[i] + traverseTimeSt[i]);
            timeLeavings[i] = convertTmToString(realTimeLeavings[i]);
            timeArrivals[i] = convertTmToString(realTimeimeArrivals[i]);
            // std::cout << timeArrivals[i] << " " << timeLeavings[i] << std::endl;
            dateAndTime[i] = timeLeavings[i];
            if ((i == numberOfStations - 1) || (i == numberOfStations - 2) 
                || (i == numberOfStations))
            {
                const char *str = "terminus";
                dateAndTime[i] = timeLeavings[i] = timeArrivals[i] = str;
            }
        }
    }
    void initializeRouteOrgDest()
    {
        routeOrigDest = nameStations[0] + "  --->  " + nameStations[numberOfStations - 1];
    }
    void updateCurrentStationAndTime()
    {
        std::string state;
        if (startFlag == 1)
            state = "Running";
        else
            state = "Sleep";
        currentStationAndTime = routeOrigDest + " " + dateAndTime[curStation] +
                                " leaving " + nameStations[curStation] + " st. nr." + std::to_string(curStation) +
                                " delay " + std::to_string(delay) + "min (state: " + state + ")";
    }
    std::string getCurrentStationAndTimeArrivalsHour()
    {
        std::string state;
        if (startFlag == 1)
            state = "Running";
        else
            state = "Sleep";
        dateAndTime[0] = timeLeavings[0];
        time_t now = currentTime();
        struct tm nowTime = *localtime(&now);
        if ((nowTime.tm_hour == realTimeLeavings[curStation].tm_hour) && (nowTime.tm_wday == realTimeLeavings[curStation].tm_wday))
        {
            currentStationAndTime = routeOrigDest + " " + timeArrivals[curStation] +
                                    " arriving " + destinationStations[curStation] + " st. nr." + std::to_string(curStation) +
                                    " delay " + std::to_string(delay) + "min (state: " + state + ")";
            return currentStationAndTime;
        }
        return "";
    }
    std::string getCurrentStationAndTimeLeavingsGeneral()
    {
        dateAndTime[0] = timeLeavings[0];
        updateCurrentStationAndTime();
        return currentStationAndTime;
    }

    std::string getCurrentStationAndTimeLeavingsToday()
    {
        dateAndTime[0] = timeLeavings[0];
        time_t now = currentTime();
        struct tm nowTime = *localtime(&now);
        if ((nowTime.tm_hour <= realTimeLeavings[curStation].tm_hour))
        {
            updateCurrentStationAndTime();
            return currentStationAndTime;
        }
        return "";
    }
    std::string getCurrentStationAndTimeLeavingsHour()
    {
        dateAndTime[0] = timeLeavings[0];
        time_t now = currentTime();
        struct tm nowTime = *localtime(&now);
        if ((nowTime.tm_hour == realTimeLeavings[curStation].tm_hour) && (nowTime.tm_wday == realTimeLeavings[curStation].tm_wday))
        {
            updateCurrentStationAndTime();
            return currentStationAndTime;
        }
        return "";
    }
    std::string getCurrentStationAndTimeArrivals()
    {
        dateAndTime[0] = timeArrivals[0];
        currentStationAndTime = routeOrigDest + " " + std::to_string(curStation) + " " + dateAndTime[curStation];
        return currentStationAndTime;
    }
    void fetchSituationOnTrains(int index)
    {
        route.clear();
        route += "\nNr|Station|MinToNextStation|ActualDelay|UntilTerminus|Time";
        for (int i = 0; i < curStation; i++)
        {
            route += "\n";
            route += std::to_string(i);
            route += " ";
            route += nameStations[i];
            route += "  ###passed###";
        }
        int timeNow = 0, temp;

        for (int i = curStation; i < numberOfStations; i++)
        {
            route += "\n";
            route += std::to_string(i);
            route += " ";
            route += nameStations[i];
            route += " ";
            route += std::to_string(traverseTimeSt[i]);
            route += "min ";
            route += std::to_string(delay);
            route += " ";
            route += std::to_string(vectorDelays[i]);
            route += " | ";
            route += dateAndTime[i];
        }
    }
    void initializeVectorOfDelays()
    {
        vectorDelays[0] = traverseTimeSt[0];
        for (int i = 1; i < numberOfStations; i++)
        {
            vectorDelays[i] = vectorDelays[i - 1] + traverseTimeSt[i];
        }
    }
    void initializeCheckTraversals()
    {
        for (int i = 0; i < numberOfStations; i++)
        {
            checkTraversal[i] = '-';
            dateAndTime[i] = "-";
        }
    }
    void resetVariables()
    {
        initializeCheckTraversals();
        setLoginFlag(0);
        setStartFlag(0);
        setTraverseTimeSt(trainIdInList);
        initializeVectorOfDelays();
        initializeCheckTraversals();
        initializeRouteOrgDest();
        // setThreadId(-1);
        startFlag = 0;
        stopFlag = 1;
        curStation = 0;
        delay = 0;
    }
    std::string outputInRealTimeStationOfTrain(char result[])
    {
        if (curStation < numberOfStations)
        {
            char buffer[MESSAGE_SIZE];
            sprintf(result, "\ntraversed %d - %s => %d %s, \nexpected %d min(%d delay, %d usual agenda)\nWrite delay if needed (\"delay+/-value\")",
                    curStation, nameStations[curStation].c_str(), curStation + 1,
                    destinationStations[curStation].c_str(), delay + baseTraverseSt[curStation], delay, baseTraverseSt[curStation]);
            curStation++;
            // sleep(traverseTimeSt[curStation]);
            sleep(0);
        }
        else
        {
            sprintf(result, "\nThat was the end of the route nr. %d. Thank you for your work!\n", trainIdInList);
            resetVariables();
        }
        std::string s(result);
        return s;
    }
    std::string startTrain()
    {
        char result[MESSAGE_SIZE];
        startFlag = 1;
        char traversalSituation[MESSAGE_SIZE];
        delayValue(0);
        outputInRealTimeStationOfTrain(result);
        std::string s(result);
        return s;
    }
    tm convertSecondsToTime(int sec)
    {
        int seconds, hours, minutes;
        minutes = int(sec / 60);
        hours = int(minutes / 60);
        seconds = sec % 60;
        time_t now = currentTime();
        struct tm nowTime = *localtime(&now);
        struct tm delayedTime = nowTime;
        delayedTime.tm_sec = seconds;
        delayedTime.tm_min = minutes;
        delayedTime.tm_hour = hours;
        mktime(&delayedTime);
        return delayedTime;
    }
    void setTraverseTimeSt(int index)
    {
        auto train = trainMapIdToNodes[index];
        auto stations = train.child("Trase").child("Trasa").children();
        int i = 0;
        int sumOfTraversedTime = 0;
        estimatedFinalArr = 0;
        for (auto obj : stations)
        {
            const char *oraP = obj.attribute("OraP").as_string();
            const char *oraS = obj.attribute("OraS").as_string();
            int leavInteg = atoi(oraP), comingInteg = atoi(oraS);
            realTimeLeavings[i] = convertSecondsToTime(leavInteg);
            realTimeimeArrivals[i] = convertSecondsToTime(comingInteg);
            timeLeavings[i] = convertTmToString(realTimeLeavings[i]);
            timeArrivals[i] = convertTmToString(realTimeimeArrivals[i]);

            int interval = atoi(oraS) - atoi(oraP);
            traverseTimeSt[i] = interval;
            estimatedFinalArr += interval;
            traverseTimeSt[i] /= 60;
            // note: further for the sake of a faster demo we will take the
            // nr of minutes as seconds
            // in the xml we've got the time in seconds so first we need to
            // convert to minutes
            i++;
        }
        for (int i = 0; i < numberOfStations; i++)
        {
            baseTraverseSt[i] = traverseTimeSt[i];
        }
        i = 0;
    }
    void mapTrainsToRoutesAndIndexes()
    {
        trainSettings = "Nr" + std::to_string(trainIdInList) + " " + getRouteOrgDest() + " || Code: " + trainCode + "; Km: " + kmTrain +
                        "; Length: " + lengthTrain + "; Owner: " + ownerTrain + "; Rank: " + rankTrain + "; Tonnage: " + tonnage + ".";
    }
    void setTrainSettings(int index)
    {
        auto train = trainMapIdToNodes[index];
        trainCode = train.attribute("Numar").as_string();
        kmTrain = train.attribute("KmCum").as_string();
        lengthTrain = train.attribute("Lungime").as_string();
        operatorTrain = train.attribute("Operator").as_string();
        ownerTrain = train.attribute("Proprietar").as_string();
        rankTrain = train.attribute("Rang").as_string();
        tonnage = train.attribute("Tonaj").as_string();
        mapTrainsToRoutesAndIndexes();
    }
    void setStations(int index)
    {
        auto train = trainMapIdToNodes[index];

        auto stations = train.child("Trase").child("Trasa").children();
        int i = 0;
        route.clear();
        numberOfStations = 0;
        for (auto obj : stations)
        {
            route += "\n";
            nameStations[i] = obj.attribute("DenStaOrigine").as_string();
            destinationStations[i] = obj.attribute("DenStaDestinatie").as_string();
            route += std::to_string(i);
            route += " ";
            route += nameStations[i];
            numberOfStations++;
            i++;
        }
        basicRoute = route;
        i = 0;
    }
    void initializeVariables(int id)
    {

        setTrainIdInList(id);
        setStations(id);
        setTraverseTimeSt(id);
        initializeVectorOfDelays();
        initializeCheckTraversals();
        initializeRouteOrgDest();
        setTrainSettings(id);
    }
    static int getNumberOfTrains()
    {
        return numberOfTrains;
    }
    static void traverseDocument()
    {
        int i = 0;
        auto train = doc.child("XmlMts").child("Mt").child("Trenuri").children();
        for (auto &obj : train)
        {
            trainMapIdToNodes[i++] = obj;
        }
        i = 0;
    }
    static void checkXmlFile(const char *source)
    {
        pugi::xml_parse_result result = doc.load_file(source);
        if (result)
        {
            std::cout << "XML [" << source << "] parsed successfully!\nThe file is ready for use.\n\n";
            xmlCheck = 1;
        }
        else
        {
            std::cout << result.description();
            std::cout << "XML [" << source << "] parsed with errors, attr value: [" << doc.child("node").attribute("attr").value() << "]\n";
            std::cout << "Error description: " << result.description() << "\n";
            std::cout << "Error offset: " << result.offset << " (error at [..." << (source + result.offset) << "]\n\n";
        }

        auto trenuri = doc.child("XmlMts").child("Mt").child("Trenuri").children();
    }
    void on()
    {
        std::cout << "The train is on\n";
    }
    void off()
    {
        std::cout << "The train is off\n";
    }
    void delayOn(int delayTr)
    {
        printf("Now your train will be delayed by %d minutes.\n", delayTr);
        delayValue(delayTr);
    }
    void delayOff(int delayTr)
    {
        printf("Now your train will be undelayed by %d minutes.\n", delayTr);
        delayValue(-delayTr);
    }
};

std::map<int, pugi::xml_node> Train::trainMapIdToNodes; 
std::set<int> Train::setOfConnectedTrains;
int Train::xmlCheck = 1;
pugi::xml_document Train::doc;

class TrainDelayCommand : public Command
{
    int delay;

public:
    TrainDelayCommand(Train *train, int delayNumber) : mTrain(train)
    {
        delay = delayNumber;
    }
    void execute()
    {
        mTrain->delayOn(delay);
    }
    void unexecute()
    {
        mTrain->delayOff(delay);
    }

private:
    Train *mTrain;
};

// Invoker stores the concrete command obj
class TrainRemoteControl
{
public:
    void setCommand(Command *cmd)
    {
        mCmd = cmd;
    }

    void pressButton()
    {
        mCmd->execute();
    }

    void undoButton()
    {
        mCmd->unexecute();
    }

private:
    Command *mCmd;
};
