#include "../includes.h"

extern int errno;

typedef struct ThreadInfo
{
    int idThread;
    int acceptDescriptor;
};

struct ThreadsList{
    pthread_t thread;
    struct ThreadInfo* threadInfo;
    struct ThreadsList* next;
} *start, *last;

struct sockaddr_in server;
struct sockaddr_in client;

int socketDescriptor;
int idThreadCounter;

static void *treat(void *);
void raspunde(void *);

void createAndOpenServer(){
    if ((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        return errno;
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
        return errno;
    }

    if (listen(socketDescriptor, 5) == -1)
    {
        perror("Listen error.\n");
        return errno;
    }
}

void serveClients(){
    while(true){
        int clientDescriptor;
        int length = sizeof(client);

        printf("Listening on port %d...\n", RESOLVER_PORT);
        fflush(stdout);

        if ((clientDescriptor = accept(socketDescriptor, (struct sockaddr *)&client, &length)) < 0)
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

        pthread_create(last->thread, NULL, &treat, last->threadInfo);
    }
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

    return false;
}

void saveIpInCache(char domain[], char ip[])
{
    FILE *f = fopen(RESOLVER_CACHE_FILE, "a");
    fprintf(f, "%s %s", domain, ip);
    fclose(f);
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

    if ((topLevelDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        exit(errno);
    }

    bzero(&topLevel, sizeof(topLevel));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(IP);
    server.sin_port = htons(tldAddress);

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

void getIpFromAuth(char domain[], int authAddress, char ip[])
{
    int authDescriptor;
    struct sockaddr_in auth;

    if ((authDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        exit(errno);
    }

    bzero(&auth, sizeof(auth));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(IP);
    server.sin_port = htons(authAddress);

    if (connect(authDescriptor, (struct sockaddr *)&auth, sizeof(struct sockaddr)) == -1)
    {
        perror("Connect error!.\n");
        exit (errno);
    }

    if (write(authDescriptor, domain, strlen(domain)) <= 0)
    {
        perror("Write to auth error!\n");
        exit(errno);
    }

    if (read(authDescriptor, ip, sizeof(ip)) < 0)
    {
        perror("Read error!\n");
        exit(errno);
    }

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

    if (read(rootDescriptor, ip, sizeof(ip)) < 0)
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
            getIPFromAuth(domain, getAuthAddress(request, getTLDAddress(request)), ip);
        } else {
            char authAddress[100];
            getIPFromServers(request, ip);
        }

        saveIpInCache(request, ip);
    }

    if (write(thisThread.acceptDescriptor, ip, strlen(ip)) <= 0)
    {
        printf("[Thread %d] ", thisThread.idThread);
        perror("Write to client error!\n");
    }

    close(thisThread.acceptDescriptor);
}

int main()
{
    createAndOpenServer();

    serveClients();
};