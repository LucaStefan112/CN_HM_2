#include "../includes.h"

extern int errno;

struct ThreadInfo
{
    int idThread;
    int acceptDescriptor;
};

struct sockaddr_in server;
struct sockaddr_in resolver;

int socketDescriptor;
int idThreadCounter;

static void *treat(void *);

struct ThreadsList{
    pthread_t thread;
    struct ThreadInfo* threadInfo;
    struct ThreadsList* next;
} *start, *last;

void createAndOpenServer(){
    if ((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        exit(errno);
    }

    int on = 1;
    setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server, sizeof(server));
    bzero(&resolver, sizeof(resolver));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(ROOT_PORT);

    if (bind(socketDescriptor, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("Bind error!\n");
        exit(errno);
    }

    if (listen(socketDescriptor, 5) == -1)
    {
        perror("Listen error.\n");
        exit(errno);
    }
}

int getTLDAddress(char domain[]){
    char data[100];

    FILE *f = fopen(ROOT_INFO_FILE, "r");
    while (fgets(data, sizeof(data), f) != NULL)
    {
        char *token = strtok(data, " ");
        if (strcmp(token, domain) == 0)
        {
            token = strtok(NULL, " ");
            return atoi(token);
        }
    }

    fclose(f);

    return -1;
}

void getIpFromServers(char request[], int tldAddress, char ip[]){
    int topLevelDescriptor;
    struct sockaddr_in topLevel;

    if ((topLevelDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        exit(errno);
    }

    bzero(&topLevel, sizeof(topLevel));

    topLevel.sin_family = AF_INET;
    topLevel.sin_addr.s_addr = inet_addr(IP);
    topLevel.sin_port = htons(tldAddress);

    if (connect(topLevelDescriptor, (struct sockaddr *)&topLevel, sizeof(struct sockaddr)) < 0)
    {
        perror("Connect error!\n");
        exit(errno);
    }

    if (write(topLevelDescriptor, request, strlen(request)) <= 0)
    {
        perror("Write to top level error!\n");
        exit(errno);
    }

    if (read(topLevelDescriptor, ip, 100) <= 0)
    {
        perror("Read from top level error!\n");
        exit(errno);
    }

    close(topLevelDescriptor);
}

void communicateWithClient(void *arg)
{
    char request[100], domain[100], ip[100];
    struct ThreadInfo thisThread;
    thisThread = *((struct ThreadInfo *)arg);

    if (read(thisThread.acceptDescriptor, request, sizeof(request)) <= 0)
    {
        printf("[Thread %d]\n", thisThread.idThread);
        perror("Read from client error!\n");
    }

    char reqType = request[0];
    int position = strlen(request) - 1;
    
    while(request[position] != '.'){
        position--;
    }

    strcpy(domain, request + position + 1);

    int tldAddress = getTLDAddress(domain);

    if(tldAddress == -1){
        if(reqType == 'r'){
            if (write(thisThread.acceptDescriptor, NOT_FOUND, sizeof(NOT_FOUND)) <= 0)
            {
                printf("[Thread %d] ", thisThread.idThread);
                perror("Write to resolver error!\n");
            }
        } else {
            if (write(thisThread.acceptDescriptor, &tldAddress, sizeof(tldAddress)) <= 0)
            {
                printf("[Thread %d] ", thisThread.idThread);
                perror("Write to resolver error!\n");
            }
        }
    } else {
        if(reqType == 'i'){
            if (write(thisThread.acceptDescriptor, &tldAddress, sizeof(tldAddress)) <= 0)
            {
                printf("[Thread %d] ", thisThread.idThread);
                perror("Write to resolver error!\n");
            }
        } else {
            char ip[100];
            getIpFromServers(request, tldAddress, ip);

            if (write(thisThread.acceptDescriptor, ip, strlen(ip)) <= 0)
            {
                printf("[Thread %d] ", thisThread.idThread);
                perror("Write to resolver error!\n");
            }
        }
    }

    close(thisThread.acceptDescriptor);
}

void *treat(void *arg) 
{
    struct ThreadInfo thisThread;
    thisThread = *((struct ThreadInfo *)arg);
    fflush(stdout);
    pthread_detach(pthread_self());

    communicateWithClient((struct thData *)arg);
    
    close((intptr_t)arg);
    
    return (NULL);
};

void serveClients(){
    while(true){
        int resolverDescriptor;
        int length = sizeof(resolver);

        printf("Listening on port %d...\n", ROOT_PORT);
        fflush(stdout);

        if ((resolverDescriptor = accept(socketDescriptor, (struct sockaddr *)&resolver, (socklen_t *)&length)) < 0)
        {
            perror("Accept error!\n");
            continue;
        }

        if(idThreadCounter == 0){
            start = (struct ThreadsList*)malloc(sizeof(struct ThreadsList));
            start->threadInfo = (struct ThreadInfo*)malloc(sizeof(struct ThreadInfo));
            start->threadInfo->idThread = idThreadCounter++;
            start->threadInfo->acceptDescriptor = resolverDescriptor;
            start->next = NULL;
            last = start;
        } else {
            last->next = (struct ThreadsList*)malloc(sizeof(struct ThreadsList));
            last->next->threadInfo = (struct ThreadInfo*)malloc(sizeof(struct ThreadInfo));
            last->next->threadInfo->idThread = idThreadCounter++;
            last->next->threadInfo->acceptDescriptor = resolverDescriptor;
            last->next->next = NULL;
            last = last->next;
        }

        pthread_create(&last->thread, NULL, &treat, last->threadInfo);
    }
}

int main()
{
    createAndOpenServer();

    serveClients();
};