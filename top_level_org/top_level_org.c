#include "../includes.h"

extern int errno;

struct ThreadInfo
{
    int idThread;
    int acceptDescriptor;
};

struct sockaddr_in server;
struct sockaddr_in root;

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
    bzero(&root, sizeof(root));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(TOP_LEVEL_ORG_PORT);

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

int getAuthAddress(char domain[]){
    char data[100];

    FILE *f = fopen(TOP_LEVEL_ORG_INFO_FILE, "r");
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

void getIpFromServers(char request[], int authAddress, char ip[]){
    int authDescriptor;
    struct sockaddr_in auth;

    if ((authDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        exit(errno);
    }

    bzero(&auth, sizeof(auth));

    auth.sin_family = AF_INET;
    auth.sin_addr.s_addr = inet_addr(IP);
    auth.sin_port = htons(authAddress);

    if (connect(authDescriptor, (struct sockaddr *)&auth, sizeof(struct sockaddr)) < 0)
    {
        perror("Connect error!\n");
        exit(errno);
    }

    if (write(authDescriptor, request, strlen(request)) <= 0)
    {
        perror("Write to auth error!\n");
        exit(errno);
    }

    if (read(authDescriptor, ip, 100) <= 0)
    {
        perror("Read from auth error!\n");
        exit(errno);
    }

    close(authDescriptor);
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

    int authAddress = getAuthAddress(domain);

    if(authAddress == -1){
        if(reqType == 'r'){
            if (write(thisThread.acceptDescriptor, NOT_FOUND, sizeof(NOT_FOUND)) <= 0)
            {
                printf("[Thread %d] ", thisThread.idThread);
                perror("Write to resolver error!\n");
            }
        } else {
            if (write(thisThread.acceptDescriptor, &authAddress, sizeof(authAddress)) <= 0)
            {
                printf("[Thread %d] ", thisThread.idThread);
                perror("Write to resolver error!\n");
            }
        }
    } else {
        if(reqType == 'i'){
            if (write(thisThread.acceptDescriptor, &authAddress, sizeof(authAddress)) <= 0)
            {
                printf("[Thread %d] ", thisThread.idThread);
                perror("Write to root error!\n");
            }
        } else {
            char ip[100];
            getIpFromServers(request, authAddress, ip);

            if (write(thisThread.acceptDescriptor, ip, strlen(ip)) <= 0)
            {
                printf("[Thread %d] ", thisThread.idThread);
                perror("Write to root error!\n");
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
        int rootDescriptor;
        int length = sizeof(root);

        printf("Listening on port %d...\n", TOP_LEVEL_ORG_PORT);
        fflush(stdout);

        if ((rootDescriptor = accept(socketDescriptor, (struct sockaddr *)&root, (socklen_t*)&length)) < 0)
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

        pthread_create(&last->thread, NULL, &treat, last->threadInfo);
    }
}

int main()
{
    createAndOpenServer();

    serveClients();
};