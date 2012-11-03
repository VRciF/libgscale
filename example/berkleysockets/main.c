/*
 * main.c
 *
 *  Created on: Oct 20, 2010
 */

#ifndef MAIN_C_
#define MAIN_C_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>

#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/time.h>

#define GSCALE_LOG_ON 1
/* #define GSCALE_TRACE_ON 1 */

#include "gscale.h"
#include "backend/berkleysockets.h"
#include "backend/berkleysockets/constants.h"
#include "backend/berkleysockets/types.h"

int rewritepromt = 0;

void printError(GScale_Error *error);
void printError(GScale_Error *error){
	char msgbuff[512];
	uint16_t i=0;

	strerror_r(error->except.code, msgbuff, sizeof(msgbuff));
    printf("[%s:%d] %d:%s:%s\n",error->except.file, error->except.line,
    		                 error->except.code, error->except.message, msgbuff);

    printf("Trace:\n");
    for(i=0;i<sizeof(error->except.trace)/sizeof(error->except.trace[0]);i++){
        if(strlen(error->except.trace[i])<=0){ break; }
        printf("    %02d:%s\n", i, error->except.trace[i]);
    }

    rewritepromt = 1;
}

void OnRead(GScale_Node *node, const GScale_Packet *packet);
void OnNodeAvailable(GScale_Node *dst, const GScale_Node_FQIdentifier *availnode);
void OnNodeUnavailable(GScale_Node *dst, const GScale_Node_FQIdentifier *unavailnode);

void OnRead(GScale_Node *node, const GScale_Packet *packet) {
    if(packet==NULL || node==NULL) {
        return;
    }

    /*
    fprintf(stderr, "node '%s' ", *UUID_toString((const GScale_UUID*)&node->id.uuid));
    fprintf(stderr, "from '%s'\n", *UUID_toString(&packet->header.src.id.uuid));
    */
    printf("\n%.*s#: ", sizeof(packet->header.src.id.alias), packet->header.src.id.alias);
    fwrite(packet->data.buffer, packet->data.length , sizeof(char), stdout);
    printf("\n");

    rewritepromt = 1;
}

void OnNodeAvailable(GScale_Node *dst, const GScale_Node_FQIdentifier *availnode) {
    if(dst==NULL || availnode==NULL){ return; }
    printf("\n# (%.*s has become available)\n", sizeof(availnode->id.alias), availnode->id.alias);
/*
    fprintf(stderr, "node '%s' has been notified that ", *UUID_toString((const GScale_UUID*)&dst->id.uuid));
    fprintf(stderr, "'%s' has become available\n", *UUID_toString(&availnode->id.uuid));
*/

    rewritepromt = 1;
}

void OnNodeUnavailable(GScale_Node *dst, const GScale_Node_FQIdentifier *unavailnode) {
    if(dst==NULL || unavailnode==NULL){ return; }
    printf("\n# (%.*s has become unavailable)\n", sizeof(unavailnode->id.alias), unavailnode->id.alias);
/*
    fprintf(stderr, "node '%s' has been notified that ", *UUID_toString((const GScale_UUID*)&dst->id.uuid));
    fprintf(stderr, "'%s' has become unavailable\n", *UUID_toString(&unavailnode->id.uuid));
*/

    rewritepromt = 1;
}

void OnServerSocketFailsResultCallback(GScale_FutureResult *result, GScale_Group *g, GScale_Backend_Init initcallback){
    GScale_Backend_Berkleysockets_Options_CmdPort cmd;
    static int failcount = 0;

    if(result==NULL || initcallback!=GScale_Backend_BerkleySockets){ return; }
    /*printf("result: %d VS %d\n\n", result->success, GSCALE_OK);*/
    if(failcount>=1 || result->success==GSCALE_OK){
        return;
    }

    cmd.fr.resultcallback=NULL;
    cmd.flags.del |= 1;
    cmd.flags.listen_stream |= 1;
    cmd.flags.listen_dgram |= 1;
    cmd.flags.broadcast |= 0;
    cmd.addr.ss_family = AF_UNSPEC;
    ((struct sockaddr_in*)&cmd.addr)->sin_port = htons(8000);
    GScale_Group_Backend_setOption(g, GScale_Backend_BerkleySockets, GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_PORT, &cmd, sizeof(cmd));

    cmd.fr.resultcallback=NULL;
    cmd.flags.del = 0;
    cmd.flags.add |= 1;
    cmd.flags.listen_stream |= 1;
    cmd.flags.listen_dgram |= 1;
    cmd.flags.broadcast |= 1;
    cmd.addr.ss_family = AF_UNSPEC;
    ((struct sockaddr_in*)&cmd.addr)->sin_port = htons(8001);
    GScale_Group_Backend_setOption(g, GScale_Backend_BerkleySockets, GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_PORT, &cmd, sizeof(cmd));

    failcount++;
}

static const char *optString = "a:g:n:h?";

static const struct option longOpts[] = {
    {   "alias", required_argument, NULL, 'a' },
    {   "group", required_argument, NULL, 'g' },
    {   "namespace",required_argument, NULL, 'n'},
    {"help", no_argument, NULL, 'h'},
    {NULL, no_argument, NULL, 0}
};

static int run = 1;
static int lastsignal = -1;

void printhelp();
void printhelp() {
    printf("Berkleysocket Group Scale\n");
    printf("main [-h] -n NAMESPACE -g NAME -a NAME\n\n");

    printf("  -h    print this help and exit\n");

    printf("  -g NAME         the group name to connect\n");
    printf("  -n NAMESPACE    the groups namespace to connect\n\n");
    printf("  -a NAME         alias-name for local node\n");
}

/* The signal handler function */
void exithandler( int signal );
void exithandler( int signal ) {
    lastsignal = signal;
    switch(signal){
        case SIGINT:
        case SIGTERM:
            run = 0;
            break;
        default:
            break;
    }
}


int main(int argc, char **argv) {
/*
    char lockfile[8192] = {'\0'};
    FILE *lock = NULL;
*/
    GScale_Group g;
    struct rlimit rl = { RLIM_INFINITY, RLIM_INFINITY };
    int argsmissing = 3;

    int option_index = 0;
    int opt = 0;

    GScale_Node_Alias alias = {'\0'};
    char gnspname[1024];
    char gname[1024];
    int gnspnamelen=0, gnamelen=0;

    char buffer[1024] = {'\0'};
    uint32_t counter = 0;
    struct timeval timeout = {0, 100000};
    GScale_Node *node = NULL;
    GScale_NodeCallbacks cbs = {&OnRead, &OnNodeAvailable, &OnNodeUnavailable};
    GScale_Packet_Payload data;
    int gscalefd=0,stdinfd=0, maxfd=0;
    int rval=0;
    struct timeval tv;

    fd_set fset;

    opt = getopt_long( argc, argv, optString, longOpts, &option_index );

    signal( SIGINT, exithandler );
    signal( SIGTERM, exithandler );
    setrlimit(RLIMIT_CORE, &rl);

    /* parse command line arguments */
    while( opt != -1 ) {
        switch( opt ) {
            case 'n':
                argsmissing--;

                gnspnamelen = strlen(optarg);
                snprintf(gnspname, sizeof(gnspname), "%.*s", gnspnamelen, optarg);
                break;
            case 'g':
                argsmissing--;

                gnamelen = strlen(optarg);
                snprintf(gname, sizeof(gname), "%.*s", gnamelen, optarg);
            break;
            case 'a':
                argsmissing--;
                snprintf(alias, sizeof(alias), "%.*s", strlen(optarg), optarg);
                break;

            case ':':
            case 'h':
            case '?':
            default:
            printhelp();
            exit(0);
        }

        opt = getopt_long( argc, argv, optString, longOpts, &option_index );
    }

    if(argsmissing) {
        printhelp();
        exit(-1);
    }

    /* init group */
    GScale_setErrorHandler(printError);
    if(!GScale_Group_init(&g)) {
    	printf(__FILE__ ":%d\n",__LINE__);
        exit(-1);
    }
    GScale_Group_setNamespace(&g, gnspname, gnspnamelen);
    GScale_Group_setGroupname(&g , gname, gnamelen);

    if(!GScale_Group_initBackend(&g, GScale_Backend_BerkleySockets)) {
    	printf(__FILE__ ":%d\n",__LINE__);
        exit(-1);
    }

    /*
    cmd.addr.sin_family = AF_INET;
    ((struct sockaddr_in*)&cmd.addr)->sin_addr.s_addr = INADDR_ANY;

    cmd.addr.sin6_family = AF_INET6;
    ((struct sockaddr_in6*)&cmd.addr)->sin6_addr = in6addr_any;
    */
    /*
    {
        int port = 8000;
        GScale_Group_Backend_setOption(&g, GScale_Backend_BerkleySockets, GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_PORT, &cmd, sizeof(cmd));
    }
    */

    /* connect */
    if((node=GScale_Group_connect(&g, cbs, (GScale_Node_Alias*)alias))==NULL){
        printf(__FILE__ ":%d\n",__LINE__);
        exit(-1);
    }
    printf("connected '%s'\n", UUID_toString(node->locality.local.id.uuid));

    GScale_Group_Worker(&g, &timeout);

    /* handle message input */
    gscalefd = GScale_Group_getEventDescriptor(&g);
    stdinfd = fileno(stdin);
    maxfd = gscalefd>stdinfd ? gscalefd : stdinfd;
    printf("\n(%.*s)# ", sizeof(alias), alias);
    do{
        fflush(stdout);
        FD_ZERO(&fset);

        FD_SET(stdinfd, &fset);
        FD_SET(gscalefd, &fset);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        rval = select(maxfd + 1, &fset, NULL, NULL, &tv);
        if(rval<0){
            perror("select failed");
            break;
        }
        else if(rval==0){
            FD_SET(gscalefd, &fset);
        }

        if(FD_ISSET(gscalefd, &fset)){
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;
            GScale_Group_Worker(&g, &timeout);
        }
        if(FD_ISSET(stdinfd, &fset)){
            if(fgets(buffer, sizeof(buffer)-1, stdin)==NULL){ continue; }
            if(buffer[strlen(buffer)-1]=='\n'){
                buffer[strlen(buffer)-1] = '\0';
            }

            if(strcmp(buffer, "quit")==0 || strcmp(buffer, "exit")==0){
                printf("Leaving group\n");
                break;
            }
            else{
                counter++;

                data.buffer = buffer;
                data.length = strlen(buffer);

                GScale_Group_write(GSCALE_DESTINATION_TYPE_GROUP, node, NULL, &data);
                rewritepromt = 1;
            }
        }
        if(rewritepromt){
            printf("\n(%.*s)# ", sizeof(alias), alias);
            rewritepromt = 0;
        }
    }while(1);

    printf("disconnect\n");
    GScale_Group_disconnect(node);
    node = NULL;

    printf("shutdownbackend\n");
    GScale_Group_shutdownBackend(&g, GScale_Backend_BerkleySockets);
    printf("shutdown");
    GScale_Group_shutdown(&g);
/*
    fclose(lock);
*/
    return 0;
}

#endif /* GSCALE_H_ */
