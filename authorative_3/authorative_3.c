#include "../includes.h"

extern int errno;

typedef struct ThreadInfo
{
    int idThread;
    int acceptDescriptor;
};

struct sockaddr_in server;
struct sockaddr_in topLevel;

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
        return errno;
    }

    int on = 1;
    setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server, sizeof(server));
    bzero(&topLevel, sizeof(topLevel));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(AUTHORATIVE_PORT_3);

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

bool getIPAddress(char domain[], char ip[]){
    char data[100];

    FILE *f = fopen(AUTHORATIVE_INFO_FILE_3, "r");
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

    strcpy(domain, request + 1);

    int authAddress = getIPAddress(domain, ip);

    if(authAddress == -1){
        if (write(thisThread.acceptDescriptor, NOT_FOUND, sizeof(NOT_FOUND)) <= 0)
        {
            printf("[Thread %d] ", thisThread.idThread);
            perror("Write to root error!\n");
        }
    } else {
        if (write(thisThread.acceptDescriptor, authAddress, sizeof(authAddress)) <= 0)
        {
            printf("[Thread %d] ", thisThread.idThread);
            perror("Write to root error!\n");
        }
    }

    close(thisThread.acceptDescriptor);
}

void serveClients(){
    while(true){
        int rootDescriptor;
        int length = sizeof(topLevel);

        printf("Listening on port %d...\n", AUTHORATIVE_PORT_3);
        fflush(stdout);

        if ((rootDescriptor = accept(socketDescriptor, (struct sockaddr *)&topLevel, &length)) < 0)
        {
            perror("Accept error!\n");
            continue;
        }

        if(idThreadCounter == 0){
            start = (struct ThreadsList*)malloc(sizeof(struct ThreadsList));
            start->threadInfo = (struct ThreadInfo*)malloc(sizeof(struct ThreadInfo));
            start->threadInfo->idThread = idThreadCounter++;
            start->threadInfo->acceptDescriptor = rootDescriptor;
            start->next = NULL;
            last = start;
        } else {
            last->next = (struct ThreadsList*)malloc(sizeof(struct ThreadsList));
            last->next->threadInfo = (struct ThreadInfo*)malloc(sizeof(struct ThreadInfo));
            last->next->threadInfo->idThread = idThreadCounter++;
            last->next->threadInfo->acceptDescriptor = rootDescriptor;
            last->next->next = NULL;
            last = last->next;
        }

        pthread_create(last->thread, NULL, &treat, last->threadInfo);
    }
}

int main()
{
    createAndOpenServer();

    serveClients();
};