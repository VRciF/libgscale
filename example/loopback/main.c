/*
 * main.c
 *
 *  Created on: Oct 20, 2010
 */

#ifndef MAIN_C_
#define MAIN_C_

#include <stdio.h>

#include "gscale.h"
#include "backend/localloopback.h"

uint8_t conversationindex = 0;

char
        *conversation[] =
                {
                        "firstnode: Hello node!", /* the second nodes connects - first node replies */
                        "secondnode: Greetings back to you!",
                        "", /* marks end of talk */
                        "firstnode: Hello node!", /* third node connects - first node replies*/
                        "thirdnode: Hi, howre u?",
                        "firstnode: Fine thx",
                        "", /* marks end of talk */
                        "secondnode: Ehlo!", /* third node connects - second node replies */
                        "thirdnode: Oh, nice, a second node to communicate, howre u?",
                        "secondnode: Great, things seem to work",
                        "thirdnode: good to hear",
                        "", /* marks end of talk */
                        "firstnode: ure leaving? too bad", /* secondnode disconnects - firstnode replies */
                        "secondnode: maybe im back soon, bye",
                        "firstnode: cya",
                        "",
                        "thirdnode: hope to cya soon", /* thirdnodes replies on secondnodes disconnect */
                        "secondnode: me too, bye",
                        "",
                        "firstnode: ure leaving too, omfg shall i handle the work all alone now?", /* thirdnodes disconnects - firstnode replies */
                        "thirdnode: im sure another node will be online soon, cause everything is working so great ^^",
                        "firstnode: me hopes, cya", "thirdnode: cu"

                };

/*
 * in a production environment you shouldnt use something
 * like a handleCommunication like i do here
 * its just for demonstration purpose! a very cheap option
 */
void handleCommunication(GScale_Node *dst, const GScale_Node_FQIdentifier *src);
void handleCommunication(GScale_Node *dst, const GScale_Node_FQIdentifier *src) {

    if (conversationindex >= sizeof(conversation) / sizeof(char*)) {
        return;
    }

    if (strlen(conversation[conversationindex]) > 0) {
        GScale_Packet_Payload data;
        data.buffer = conversation[conversationindex];
        /* +1 cause of the binary 0 */
        data.length = strlen(conversation[conversationindex]) + 1;

        conversationindex++;

        if (GScale_Group_write(GSCALE_DESTINATION_TYPE_NODE, dst, src, &data)
                != data.length) {
            printf("writing '%s' failed!\n",
                    conversation[conversationindex - 1]);
        }
    } else {
        conversationindex++;
    }
}

void OnRead(GScale_Node *node, const GScale_Packet *packet);
void OnNodeAvailable(GScale_Node *dst,
        const GScale_Node_FQIdentifier *availnode);
void OnNodeUnavailable(GScale_Node *dst,
        const GScale_Node_FQIdentifier *unavailnode);

void OnRead(GScale_Node *node, const GScale_Packet *packet) {
    if (packet == NULL) {
        return;
    }

    printf("node '%s' ", *UUID_toString(&packet->header.dst.id.uuid));
    printf("received '%s' ", packet->data.buffer);
    printf("from '%s'\n", *UUID_toString(&packet->header.src.id.uuid));

    handleCommunication(node, &packet->header.src);
}

void OnNodeAvailable(GScale_Node *dst,
        const GScale_Node_FQIdentifier *availnode) {
    printf("node '%s' has been notified that ", *UUID_toString((const GScale_UUID*)&dst->id.uuid));
    printf("'%s' has become available\n", *UUID_toString(&availnode->id.uuid));

    handleCommunication(dst, availnode);
}
void OnNodeUnavailable(GScale_Node *dst,
        const GScale_Node_FQIdentifier *unavailnode) {
    printf("node '%s' has been notified that ", *UUID_toString((const GScale_UUID*)&dst->id.uuid));
    printf("'%s' has become unavailable\n", *UUID_toString(&unavailnode->id.uuid));

    handleCommunication(dst, unavailnode);
}

int main(int argc, char **argv) {
    GScale_NodeCallbacks cbs =
            { &OnRead, &OnNodeAvailable, &OnNodeUnavailable };

    /* very simple groupidentifier */
    GScale_Group g = { { "broadcast", "state" } };

    /* init group */
    GScale_Group_init(&g);
    GScale_Group_initBackend(&g, GScale_Backend_LocalLoopback);

    /* add nodes to group */
    /* since connect triggers the available callback via the localloopback
     * one can create some kind of conversation just by adding nodes ^^
     * not sure if this could be of any use
     *
     * but this example shows that the local loopback backend doesnt need a GScale_work(); call
     * to do its magic
     */
    GScale_Node_Alias n1alias = "Carol";
    GScale_Node *node1 = GScale_Group_connect(&g, cbs, &n1alias);
    GScale_Node_Alias n2alias = "Carlos";
    GScale_Node *node2 = GScale_Group_connect(&g, cbs, &n2alias);
    GScale_Node_Alias n3alias = "Charlie";
    GScale_Node *node3 = GScale_Group_connect(&g, cbs, &n3alias);

    /*printf("nodes: %p, %p, %p\n", node1, node2, node3); */

    GScale_Group_disconnect(node2);
    GScale_Group_disconnect(node3);
    GScale_Group_disconnect(node1);

    return 0;
}

#endif /* GSCALE_H_ */
