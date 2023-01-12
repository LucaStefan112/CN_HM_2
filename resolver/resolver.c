#include "../includes.h"

extern int errno;

struct ThreadInfo
{
    int idThread;
    int acceptDescriptor;
};

struct ThreadsList{
    pthread_t thread;
    struct ThreadInfo* threadInfo;
    struct ThreadsList* next;
} *start, *last;

pthread_mutex_t lock;

struct sockaddr_in server;
struct sockaddr_in client;

int socketDescriptor;
int idThreadCounter;

static void *treat(void *);

void createAndOpenServer(){
    pthread_mutex_init(&lock, NULL);

    if ((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        exit(errno);
    }

    int on = 1;
    setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server, sizeof(server));
    bzero(&client, sizeof(client));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(RESOLVER_PORT);

    if (bind(socketDescriptor, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("Bind error!\n");
        exit(errno); errno;
    }

    if (listen(socketDescriptor, 5) == -1)
    {
        perror("Listen error.\n");
        exit(errno); errno;
    }
}

void serveClients(){
    while(true){
        int clientDescriptor;
        int length = sizeof(client);

        printf("Listening on port %d...\n", RESOLVER_PORT);
        fflush(stdout);

        if ((clientDescriptor = accept(socketDescriptor, (struct sockaddr *)&client, (socklen_t*)&length)) < 0)
        {
            perror("Accept error!\n");
            continue;
        }

        if(idThreadCounter == 0){
            start = (struct ThreadsList*)malloc(sizeof(struct ThreadsList));
            start->threadInfo = (struct ThreadInfo*)malloc(sizeof(struct ThreadInfo));
            start->threadInfo->idThread = idThreadCounter++;
            start->threadInfo->acceptDescriptor = clientDescriptor;
            start->next = NULL;
            last = start;
        } else {
            last->next = (struct ThreadsList*)malloc(sizeof(struct ThreadsList));
            last->next->threadInfo = (struct ThreadInfo*)malloc(sizeof(struct ThreadInfo));
            last->next->threadInfo->idThread = idThreadCounter++;
            last->next->threadInfo->acceptDescriptor = clientDescriptor;
            last->next->next = NULL;
            last = last->next;
        }

        pthread_create(&last->thread, NULL, &treat, last->threadInfo);
    }
}

bool isDomainInCache(char domain[], char ip[])
{
    char data[100];

    FILE *f = fopen(RESOLVER_CACHE_FILE, "r");
    while (fgets(data, sizeof(data), f) != NULL)
    {
        char *token = strtok(data, " ");
        if (strcmp(token, domain) == 0)
        {
            token = strtok(NULL, " ");
            strcpy(ip, token);
            return true;
        }
    }

    fclose(f);

    return false;
}

void saveIpInCache(char domain[], char ip[])
{
    pthread_mutex_lock(&lock);

    FILE *f1 = fopen(RESOLVER_CACHE_FILE, "r");
    FILE *f2 = fopen(RESOLVER_CACHE_FILE_TMP, "w");

    char data[100];

    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    while (fgets(data, sizeof(data), f1) != NULL)
    {
        char thisDomain[100], thisIp[100];
        char *token = strtok(data, " ");
        strcpy(thisDomain, token);
        token = strtok(NULL, " ");
        strcpy(thisIp, token);
        token = strtok(NULL, " ");
        long int timestamp = atol(token);

        if (ms - timestamp < 600000)
        {
            fprintf(f2, "%s %s %ld\n", thisDomain, thisIp, timestamp);
        }
    }
    fprintf(f2, "%s %s %ld\n", domain, ip, ms);

    fclose(f1);
    fclose(f2);

    remove(RESOLVER_CACHE_FILE);
    rename(RESOLVER_CACHE_FILE_TMP, RESOLVER_CACHE_FILE);

    pthread_mutex_unlock(&lock);
}

int getTLDAddress(char request[])
{
    int rootDescriptor;
    struct sockaddr_in root;

    if ((rootDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        exit(errno);
    }

    bzero(&root, sizeof(root));

    root.sin_family = AF_INET;
    root.sin_addr.s_addr = inet_addr(IP);
    root.sin_port = htons(ROOT_PORT);

    if (connect(rootDescriptor, (struct sockaddr *)&root, sizeof(struct sockaddr)) == -1)
    {
        perror("Connect error!.\n");
        exit (errno);
    }

    if (write(rootDescriptor, request, strlen(request)) <= 0)
    {
        perror("Write to root error!\n");
        exit(errno);
    }

    int tldAddress;

    if (read(rootDescriptor, &tldAddress, sizeof(tldAddress)) < 0)
    {
        perror("Read error!\n");
        exit(errno);
    }

    close(rootDescriptor);

    return tldAddress;
}

int getAuthAddress(char request[], int tldAddress)
{
    int topLevelDescriptor;
    struct sockaddr_in topLevel;

    if(tldAddress == -1){
        return -1;
    }

    if ((topLevelDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        exit(errno);
    }

    bzero(&topLevel, sizeof(topLevel));

    topLevel.sin_family = AF_INET;
    topLevel.sin_addr.s_addr = inet_addr(IP);
    topLevel.sin_port = htons(tldAddress);

    if (connect(topLevelDescriptor, (struct sockaddr *)&topLevel, sizeof(struct sockaddr)) == -1)
    {
        perror("Connect error!.\n");
        exit (errno);
    }

    if (write(topLevelDescriptor, request, strlen(request)) <= 0)
    {
        perror("Write to tld error!\n");
        exit(errno);
    }

    int authAddress;

    if (read(topLevelDescriptor, &authAddress, sizeof(authAddress)) < 0)
    {
        perror("Read error!\n");
        exit(errno);
    }

    close(topLevelDescriptor);

    return authAddress;
}

void getIpFromAuth(char request[], int authAddress, char ip[])
{
    int authDescriptor;
    struct sockaddr_in auth;

    if(authAddress == -1){
        strcpy(ip, NOT_FOUND);
        return;
    }

    if ((authDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        exit(errno);
    }

    bzero(&auth, sizeof(auth));

    auth.sin_family = AF_INET;
    auth.sin_addr.s_addr = inet_addr(IP);
    auth.sin_port = htons(authAddress);

    if (connect(authDescriptor, (struct sockaddr *)&auth, sizeof(struct sockaddr)) == -1)
    {
        perror("Connect error!.\n");
        exit (errno);
    }

    if (write(authDescriptor, request, strlen(request)) <= 0)
    {
        perror("Write to auth error!\n");
        exit(errno);
    }

    if (read(authDescriptor, ip, 100) < 0)
    {
        perror("Read error!\n");
        exit(errno);
    }

    printf("IP: %s", ip);

    close(authDescriptor);
}

void getIPFromServers(char request[], char ip[])
{
    int rootDescriptor;
    struct sockaddr_in root;

    if ((rootDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        exit(errno);
    }

    bzero(&root, sizeof(root));

    root.sin_family = AF_INET;
    root.sin_addr.s_addr = inet_addr(IP);
    root.sin_port = htons(ROOT_PORT);

    if (connect(rootDescriptor, (struct sockaddr *)&root, sizeof(struct sockaddr)) == -1)
    {
        perror("Connect error!.\n");
        exit (errno);
    }

    if (write(rootDescriptor, request, strlen(request)) <= 0)
    {
        perror("Write to server error!\n");
        exit(errno);
    }

    if (read(rootDescriptor, ip, 100) < 0)
    {
        perror("Read error!\n");
        exit(errno);
    }

    close(rootDescriptor);
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
    strcpy(domain, request + 1);

    if(!isDomainInCache(domain, ip)){
        if(reqType == 'i'){
            getIpFromAuth(request, getAuthAddress(request, getTLDAddress(request)), ip);
        } else {
            char authAddress[100];
            getIPFromServers(request, ip);
        }

        if(strcmp(ip, NOT_FOUND) != 0) {
            saveIpInCache(domain, ip);
        }
    }

    printf("[Thread %d] IP: %s\n", thisThread.idThread, ip);

    if (write(thisThread.acceptDescriptor, ip, strlen(ip)) <= 0)
    {
        printf("[Thread %d] ", thisThread.idThread);
        perror("Write to client error!\n");
    }

    close(thisThread.acceptDescriptor);
}

void *treat(void *arg)
{
    struct ThreadInfo thisThread;
    thisThread = *((struct ThreadInfo *)arg);
    fflush(stdout);
    pthread_detach(pthread_self());

    communicateWithClient(arg);
    
    close((intptr_t)arg);
    
    return (NULL);
};

int main()
{
    createAndOpenServer();

    serveClients();
};