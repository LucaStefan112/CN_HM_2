#include "../includes.h"

extern int errno;

typedef struct ThreadInfo
{
    int idThread;
    int acceptDescriptor;
};

struct sockaddr_in server;
struct sockaddr_in from;

int socketDescriptor;
int idThreadCounter;

static void *treat(void *);
void raspunde(void *);

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
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(ROOT_PORT);

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
        int fromDescriptor;
        int length = sizeof(from);

        printf("Listening on port %d...\n", RESOLVER_PORT);
        fflush(stdout);

        if ((fromDescriptor = accept(socketDescriptor, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("Accept error!\n");
            continue;
        }

        if(idThreadCounter == 0){
            start = (struct ThreadsList*)malloc(sizeof(struct ThreadsList));
            start->threadInfo = (struct ThreadInfo*)malloc(sizeof(struct ThreadInfo));
            start->threadInfo->idThread = idThreadCounter++;
            start->threadInfo->acceptDescriptor = fromDescriptor;
            start->next = NULL;
            last = start;
        } else {
            last->next = (struct ThreadsList*)malloc(sizeof(struct ThreadsList));
            last->next->threadInfo = (struct ThreadInfo*)malloc(sizeof(struct ThreadInfo));
            last->next->threadInfo->idThread = idThreadCounter++;
            last->next->threadInfo->acceptDescriptor = fromDescriptor;
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

    int tldAddress = getTLDAddress();

    if(reqType == 'i'){
        if (write(thisThread.acceptDescriptor, tldAddress, sizeof(tldAddress)) <= 0)
        {
            printf("[Thread %d] ", thisThread.idThread);
            perror("Write to resolver error!\n");
        }
    } else {
        char ip[100];
        getIPFromServers(request, tldAddress, ip);

        if (write(thisThread.acceptDescriptor, ip, sizeof(ip)) <= 0)
        {
            printf("[Thread %d] ", thisThread.idThread);
            perror("Write to resolver error!\n");
        }
    }

    close(thisThread.acceptDescriptor);
}