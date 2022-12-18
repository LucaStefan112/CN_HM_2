#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>

#define IP "0.0.0.0"

#define RESOLVER_PORT 4001
#define ROOT_PORT 4002
#define TOP_LEVEL_COM_PORT 4003
#define TOP_LEVEL_NET_PORT 4004
#define TOP_LEVEL_ORG_PORT 4005
#define AUTHORATIVE_PORT_1 4006
#define AUTHORATIVE_PORT_2 4007
#define AUTHORATIVE_PORT_3 4008

#define CLIENT_CACHE_FILE "client.cache"
#define RESOLVER_CACHE_FILE "resolver.cache"

#define ROOT_INFO_FILE "root.info"
#define TOP_LEVEL_COM_INFO_FILE "top_level_com.info"
#define TOP_LEVEL_NET_INFO_FILE "top_level_net.info"
#define TOP_LEVEL_ORG_INFO_FILE "top_level_org.info"
#define AUTHORATIVE_INFO_FILE_1 "authorative_1.info"
#define AUTHORATIVE_INFO_FILE_2 "authorative_2.info"
#define AUTHORATIVE_INFO_FILE_3 "authorative_3.info"