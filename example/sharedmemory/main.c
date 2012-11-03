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

#define GSCALE_LOG_ON 1
#define GSCALE_TRACE_ON 1

#include "gscale.h"
#include "backend/localsharedmemory.h"

void printError (int level, const char *file, int line, const char *function, const char *format, ...);
void printError(int level, const char *file, int line, const char *function, const char *format, ...) {
    /*if(level<=GSCALE_LOG_LEVEL_WARN){ return; }*/
    int ecpy = errno;

    int i=0;
    for(i=0;i<GSCALE_TRACE_CURRENT;i++) {
        printf("%s:%d:%s\n", GSCALE_BACKTRACE[i].file, GSCALE_BACKTRACE[i].line, GSCALE_BACKTRACE[i].function);
    }

    va_list args;
    va_start( args, format );
    fprintf(stderr, "[%s:%d:%s] (%d:%s) ", file, line, function, ecpy, strerror(ecpy));
    vfprintf( stderr, format, args );
    fprintf( stderr, "\n" );
    va_end( args );
}

void OnRead(GScale_Node *node, const GScale_Packet *packet);
void OnNodeAvailable(GScale_Node *dst, const GScale_Node_FQIdentifier *availnode);
void OnNodeUnavailable(GScale_Node *dst, const GScale_Node_FQIdentifier *unavailnode);

void OnRead(GScale_Node *node, const GScale_Packet *packet) {
    if(packet==NULL) {
        return;
    }

    fprintf(stderr, "node '%s' ", *UUID_toString(&packet->header.dst.id.uuid));
    fprintf(stderr, "from '%s'\n", *UUID_toString(&packet->header.src.id.uuid));
    fwrite(packet->data.buffer, packet->data.length , sizeof(char), stdout);
}

void OnNodeAvailable(GScale_Node *dst, const GScale_Node_FQIdentifier *availnode) {
    fprintf(stderr, "node '%s' has been notified that ", *UUID_toString((const GScale_UUID*)&dst->id.uuid));
    fprintf(stderr, "'%s' has become available\n", *UUID_toString(&availnode->id.uuid));

}

void OnNodeUnavailable(GScale_Node *dst, const GScale_Node_FQIdentifier *unavailnode) {
    fprintf(stderr, "node '%s' has been notified that ", *UUID_toString((const GScale_UUID*)&dst->id.uuid));
    fprintf(stderr, "'%s' has become unavailable\n", *UUID_toString(&unavailnode->id.uuid));

}

static const char *optString = "g:n:h?";

static const struct option longOpts[] = {
    {   "group", required_argument, NULL, 'g' },
    {   "namespace",required_argument, NULL, 'n'},
    {"help", no_argument, NULL, 'h'},
    {NULL, no_argument, NULL, 0}
};

static int run = 1;

void printhelp();
void printhelp() {
    printf("Shared Memory Group Scale\n");
    printf("main [-h] [-f FILE] [-o FILE]\n\n");

    printf("  -h    print this help and exit\n");

    printf("  -g NAME         the group name to connect\n");
    printf("  -n NAMESPACE    the groups namespace to connect\n\n");
}

/* The signal handler function */
void exithandler( int signal );
void exithandler( int signal ) {
    run = 0;
}


int main(int argc, char **argv) {
    GScale_Group g;

    signal( SIGINT, exithandler );
    signal( SIGTERM, exithandler );

    int argsmissing = 2;

    int option_index = 0;
    int opt = getopt_long( argc, argv, optString, longOpts, &option_index );
    while( opt != -1 ) {
        switch( opt ) {
            case 'n':
            argsmissing--;
            GScale_Group_setNamespace(&g, optarg, strlen(optarg));
            break;
            case 'g':
            argsmissing--;
            GScale_Group_setGroupname(&g , optarg, strlen(optarg));
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

    GScale_Log = printError;

    /* init group */
    GScale_NodeCallbacks cbs = {&OnRead, &OnNodeAvailable, &OnNodeUnavailable};

    if(!GScale_Group_init(&g)) {
        exit(-1);
    }
    if(!GScale_Group_initBackend(&g, GScale_Backend_LocalSharedMemory)) {
        exit(-1);
    }

    struct timeval timeout = {0, 100000};

    GScale_Node *node = NULL;
/*
    node = GScale_Group_timedConnect(&g, cbs, &timeout);
    if(node==NULL) {
        exit(-1);
    }
*/

    char buffer[1024] = {'\0'};
    uint32_t counter = 0;
/*
    int stdindesc = fileno(stdin);
    int oldfl;
    oldfl = fcntl(stdindesc, F_GETFL);
    fcntl(stdindesc, F_SETFL, oldfl | O_NONBLOCK);

    char ch = '\0';
    while(ch!='q' && run){
        ch = getchar();
        switch(ch){
            case 'q':
                break;
            case 'w':
                counter++;
                snprintf(buffer,sizeof(buffer)-1, "some message (%u)", counter);
            default:
                GScale_Group_Worker(&g, &timeout);
                break;
        }
        sleep(1);
    }
*/

    while(buffer[0]!='q' && run){
        printf("command (q,c,m,w,\\n): ");
        scanf("%s", buffer);
        switch(buffer[0]){
            case 'q':
                break;
            case 'c':
                node = GScale_Group_connect(&g, cbs, NULL);
                printf("connected '%s'\n", *UUID_toString((const GScale_UUID*)&node->id.uuid));
                if(node==NULL) {
                    exit(-1);
                }
                break;
            case 'm':
                if(node==NULL){
                    printf("ERROR: not connected yet\n");
                    break;
                }
                counter++;
                snprintf(buffer,sizeof(buffer)-1, "some message (%u)", counter);

                GScale_Packet_Payload data;
                data.buffer = buffer;
                data.length = strlen(buffer);

                GScale_Group_write(GSCALE_DESTINATION_TYPE_GROUP, node, NULL, &data);
            case 'w':
                GScale_Group_Worker(&g, &timeout);
                break;
        }
        printf("\n");
    }

    if(node!=NULL){
        GScale_Group_disconnect(node);
    }

    return 0;
}

#endif /* GSCALE_H_ */
