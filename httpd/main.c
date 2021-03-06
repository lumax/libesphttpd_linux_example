#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <libesphttpd/linux.h>
#include <libesphttpd/httpdespfs.h>
#include <libesphttpd/cgiwebsocket.h>
#include <libesphttpd/espfs.h>
#include <libesphttpd/webpages-espfs.h>
#include <libesphttpd/httpd-freertos.h>
#include <libesphttpd/cgiredirect.h>
#include <libesphttpd/route.h>

HttpdFreertosInstance httpdFreertosInstance;

//Broadcast the uptime in seconds every second over connected websockets
static void* websocketBcast(void *arg) {
    static int ctr=0;
    char buff[128];
    while(1) {
        ctr++;
        sprintf(buff, "Up for %d minutes %d seconds!\n", ctr/60, ctr%60);
        cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, "/websocket/ws.cgi", buff, strlen(buff), WEBSOCK_FLAG_NONE);
        sleep(1);
    }
}

//On reception of a message, send "You sent: " plus whatever the other side sent
static void myWebsocketRecv(Websock *ws, char *data, int len, int flags) {
    int i;
    char buff[128];
    sprintf(buff, "You sent: ");
    for (i=0; i<len; i++) buff[i+10]=data[i];
    buff[i+10]=0;
    cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, buff, strlen(buff), WEBSOCK_FLAG_NONE);
}

//Websocket connected. Install reception handler and send welcome message.
static void myWebsocketConnect(Websock *ws) {
    ws->recvCb = myWebsocketRecv;
    cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, "Hi, Websocket!", 14, WEBSOCK_FLAG_NONE);
}

//On reception of a message, echo it back verbatim
void myEchoWebsocketRecv(Websock *ws, char *data, int len, int flags) {
    printf("EchoWs: echo, len=%d\n", len);
    cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, data, len, flags);
}

//Echo websocket connected. Install reception handler.
void myEchoWebsocketConnect(Websock *ws) {
    printf("EchoWs: connect\n");
    ws->recvCb = myEchoWebsocketRecv;
}

CgiStatus ICACHE_FLASH_ATTR cgiUploadTest(HttpdConnData *connData) {

    printf("connData->post->len %d\n", connData->post.len);
    printf("connData->post->received %d\n", connData->post.received);

    if(connData->post.len == connData->post.received)
    {
        printf("Upload done. Sending response.\n");
        httpdStartResponse(connData, 200);
        httpdHeader(connData, "Content-Type", "text/plain");
        httpdEndHeaders(connData);
        httpdSend(connData, "Data received", -1);
        httpdSend(connData, "\n", -1);

        return HTTPD_CGI_DONE;
    } else
    {
        return HTTPD_CGI_MORE;
    }
}

CgiStatus ICACHE_FLASH_ATTR cgiTest(HttpdConnData *connData)
{
    httpdStartResponse(connData, 200);
    httpdEndHeaders(connData);
    httpdSend(connData, "some test response", -1);

    return HTTPD_CGI_DONE;
}

void printUsage(char* appName)
{
    printf("%s() usage:\n", appName);
    printf("\t--help, -h\n");
    printf("\t--ssl, -s\n");
}

static int print_usage_flag;
static int ssl_flag;

/** @return true if successful, false otherwise */
bool parseInput(int argc, char **argv)
{
    static struct option long_options[] =
    {
        {"help", no_argument, &print_usage_flag, 'h'},
        {"ssl", no_argument, &ssl_flag, 's'},
        {0, 0, 0, 0}
    };

    while(1)
    {
        int option_index = 0;

        char c = getopt_long(argc, argv, "sh", long_options, &option_index);

        if(c == -1)
        {
            break;
        }

        switch(c)
        {
        case 0:
            // if this option set a flag nothing else to do
            if(long_options[option_index].flag != 0)
            {
                break;
            }
            break;
        case 's':
            ssl_flag = 1;
            break;
        case 'h':
            print_usage_flag = 1;
            break;
        default:
            return false;
        }
    }

    return true;
}

int main(int argc, char** argv)
{
    HttpdBuiltInUrl builtInUrls[]={
        ROUTE_REDIRECT("/", "/index.html"),

        ROUTE_REDIRECT("/websocket", "/websocket/index.html"),
        ROUTE_CGI_ARG("/websocket/ws.cgi", cgiWebsocket, myWebsocketConnect),
        ROUTE_CGI_ARG("/websocket/echo.cgi", cgiWebsocket, myEchoWebsocketConnect),

        ROUTE_CGI("/test", cgiTest),

        ROUTE_CGI("/upload", cgiUploadTest),

        ROUTE_FILESYSTEM(),
        ROUTE_END()
    };

    if(!parseInput(argc, argv))
    {
        printf("Error parsing input arguments\n");
        return 1;
    }

    if(print_usage_flag)
    {
        printUsage(argv[0]);
        return 0;
    }

    espFsInit((void*)(webpages_espfs_start));
    int listenPort = 9000;
    printf("creating httpd on port %d\n", listenPort);

    int maxConnections = 32;
    char connectionMemory[sizeof(RtosConnType) * maxConnections];

    HttpdFlags flags;

    if(ssl_flag)
    {
        flags = HTTPD_FLAG_SSL;
    } else
    {
        flags = HTTPD_FLAG_NONE;
    }

    httpdFreertosInit(&httpdFreertosInstance,
                        builtInUrls,
                        listenPort,
                        connectionMemory, maxConnections,
                        flags);

    printf("creating websocket broadcast thread\n");
    pthread_t websocketThread;
    pthread_create(&websocketThread, NULL, websocketBcast, NULL);

    while(true)
    {
        printf("Running\n");
        sleep(5);
    }

    return 0;
}
