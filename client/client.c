#include "../includes.h"

char domain[100], ip[100];

int socketDescriptor;
struct sockaddr_in resolver;

extern int errno;

void readDomainFromStdin()
{
    printf("Enter the domain: ");
    fflush(stdout);
    read(0, domain, sizeof(domain));

    for(int i = 0; i < strlen(domain); i++)
    {
        if('A' <= domain[i] && domain[i] <= 'Z')
        {
            domain[i] += 32;
        }
    }

    if(domain[0] == 'w' && domain[1] == 'w' && domain[2] == 'w' && domain[3] == '.')
    {
        strcpy(domain, domain + 4);
    }
}

void connectToResolver()
{
    if ((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        exit(errno);
    }

    resolver.sin_family = AF_INET;
    resolver.sin_addr.s_addr = inet_addr(IP);
    resolver.sin_port = htons(RESOLVER_PORT);

    if (connect(socketDescriptor, (struct sockaddr *)&resolver, sizeof(struct sockaddr)) == -1)
    {
        perror("Connect error!.\n");
        exit (errno);
    }
}

bool isDomainInCache()
{
    char data[100];

    FILE *f = fopen(CLIENT_CACHE_FILE, "r");
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

void writeDomainToServer(int argc)
{
    char request[100];

    if(argc > 2){
        request[0] = 'r';
    } else {
        request[0] = 'i';
    }

    strcpy(request + 1, domain);

    if (write(socketDescriptor, request, strlen(request)) <= 0)
    {
        perror("Write to resolver error!\n");
        exit(errno);
    }
}

void readIpFromServer()
{
    if (read(socketDescriptor, ip, sizeof(ip)) < 0)
    {
        perror("Read error!\n");
        exit(errno);
    }
}

void printIp()
{
    printf("IP: %s", ip);
}

void saveIpInCache()
{
    FILE *f = fopen(CLIENT_CACHE_FILE, "a");
    fprintf(f, "%s %s", domain, ip);
    fclose(f);
}

int main(int argc, char *argv[]){
    connectToServer();

    readDomainFromStdin();

    if(!isDomainInCache())
    {
        writeDomainToServer(argc);
        readIpFromServer();
        saveIpInCache();
    }

    printIp();

    close(socketDescriptor);
}