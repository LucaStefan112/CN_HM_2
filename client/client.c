#include "../includes.h"

char domain[100], ip[100], lookupType;

int socketDescriptor;
struct sockaddr_in resolver;

extern int errno;

void readDomainFromStdin()
{
    system("clear");
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
        char aux[100];
        strcpy(aux, domain + 4);
        strcpy(domain, aux);
    }

    domain[strlen(domain) - 1] = '\0';

    while(lookupType != 'i' && lookupType != 'r'){
        printf("Enter the lookup type (i/r): ");
        fflush(stdout);
        char buffer[100];
        read(0, buffer, sizeof(buffer));
        lookupType = buffer[0];
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

void writeDomainToResolver()
{
    char request[100];

    request[0] = lookupType;

    strcpy(request + 1, domain);

    if (write(socketDescriptor, request, strlen(request)) <= 0)
    {
        perror("Write to resolver error!\n");
        exit(errno);
    }
}

void readIpFromResolver()
{
    if (read(socketDescriptor, ip, sizeof(ip)) < 0)
    {
        perror("Read error!\n");
        exit(errno);
    }
}

void printIp()
{
    printf("IP: %s\n", ip);
}

void saveIpInCache()
{
    FILE *f = fopen(CLIENT_CACHE_FILE, "a");
    fprintf(f, "%s %s\n", domain, ip);
    fclose(f);
}

int main(){
    readDomainFromStdin();

    // if(!isDomainInCache())
    // {
        connectToResolver();
        writeDomainToResolver();
        readIpFromResolver();
        // saveIpInCache();
    // }

    printIp();

    close(socketDescriptor);
}