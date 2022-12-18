#include "../includes.h"

char domain[100], ip[100];

int sd;
struct sockaddr_in server;

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

void connectToServer()
{
    int sd;
    struct sockaddr_in server;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error!\n");
        exit(errno);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(IP);
    server.sin_port = htons(RESOLVER_PORT);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("Connect error!.\n");
        exit (errno);
    }
}

bool isDomainInCache()
{
    char data[100];

    FILE *f = fopen(CLIENT_CACHE_FILE, "r");
    while (fgets(data, 100, f) != NULL)
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

void writeDomainToServer()
{
    if (write(sd, domain, strlen(domain)) <= 0)
    {
        perror("Write to resolver error!\n");
        exit(errno);
    }
}

void readIpFromServer()
{
    if (read(sd, ip, sizeof(ip)) < 0)
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
        writeDomainToServer();
        readIpFromServer();
        saveIpInCache();
    }

    printIp();

    close(sd);
}