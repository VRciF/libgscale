/*
reset; gcc -g -I../src/ -I. -o berkleysockets ./berkleysockets.c
*/

#include <stdio.h>
#include <string.h>

#include "gscale.h"
#include "backend_berkleysockets.h"
#include "backend_berkleysockets_constants.h"

#include "minunit.h"

int tests_run = 0;

GScale_Group client1,client2;

static int test_init() {
	char *nspace = "ntest";
	char *gname = "gtest";

	GScale_Group_setNamespace(&client1, nspace, strlen(nspace));
	GScale_Group_setGroupname(&client1, gname, strlen(gname));

	GScale_Group_setNamespace(&client2, nspace, strlen(nspace));
	GScale_Group_setGroupname(&client2, gname, strlen(gname));
/*
    GScale_NodeCallbacks cbs = {NULL, NULL, NULL};
*/
    GScale_Group_init(&client1);
    GScale_Group_initBackend(&client1, GScale_Backend_BerkleySockets);
    uint16_t port = 0;

    GScale_Group_Backend_setOption(&client1, GScale_Backend_BerkleySockets, GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_LISTENPORT, "8000", 4);
    port = 8001;
    GScale_Group_Backend_setOption(&client1, GScale_Backend_BerkleySockets, GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_BROADCASTPORT, &port, sizeof(uint16_t));

    GScale_Group_Backend_setOption(&client2, GScale_Backend_BerkleySockets, GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_LISTENPORT, "8001", 4);
    port = 8000;
    GScale_Group_Backend_setOption(&client2, GScale_Backend_BerkleySockets, GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_BROADCASTPORT, &port, sizeof(uint16_t));

    return 0;
}

static int test_shutdown() {
	/*
    if(node!=NULL){
        GScale_Group_disconnect(node);
    }
    */

    GScale_Group_shutdownBackend(&client1, GScale_Backend_BerkleySockets);
    GScale_Group_shutdown(&client1);

    GScale_Group_shutdownBackend(&client2, GScale_Backend_BerkleySockets);
    GScale_Group_shutdown(&client2);

    return 0;
}


static int test_worker() {

    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    GScale_Group_Worker(&client1, &timeout);

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    GScale_Group_Worker(&client2, &timeout);

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    GScale_Group_Worker(&client1, &timeout);

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    GScale_Group_Worker(&client2, &timeout);

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    GScale_Group_Worker(&client1, &timeout);

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    GScale_Group_Worker(&client2, &timeout);

    return 0;
}

static int all_tests() {
    mu_run_test(test_init);

    mu_run_test(test_worker);

    mu_run_test(test_shutdown);

    return 0;
}

int main(int argc, char **argv) {
    int result = all_tests();
    if (result != 0) {
        printf("%d\n", result);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
