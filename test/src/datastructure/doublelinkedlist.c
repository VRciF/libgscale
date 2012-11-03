/*
reset; gcc -ansi -I../../../src/ -I../../ -o doublelinkedlist ./doublelinkedlist.c
*/

#include <stdio.h>

#include "datastructure/doublelinkedlist.h"
#include "minunit.h"

int tests_run = 0;

DoubleLinkedList list;

static char * test_init() {
	mu_assert("error, init != 1", DoubleLinkedList_init(&list)==1);
    return 0;
}

static char * test_fillList() {
	char *node1 = NULL, *node2 = NULL, *node3 = NULL;

	mu_assert("error, list not empty", DoubleLinkedList_isEmpty(&list));

    node1 = (char*)DoubleLinkedList_allocNode(&list, 5);
    mu_assert("error, node1 == NULL", node1 != NULL);
    snprintf(node1, 5, "test%c",'\0');

    mu_assert("error, list empty", DoubleLinkedList_isEmpty(&list)==0);

    node2 = (char*)DoubleLinkedList_allocNode(&list, 5);
    mu_assert("error, node2 == NULL", node2 != NULL);
    snprintf(node2, 5, "abcd%c",'\0');

    mu_assert("error, list empty", DoubleLinkedList_isEmpty(&list)==0);

    node3 = (char*)DoubleLinkedList_allocNode(&list, 5);
    mu_assert("error, node3 == NULL", node3 != NULL);
    snprintf(node3, 5, "cba%c",'\0');

    mu_assert("error, list empty", DoubleLinkedList_isEmpty(&list)==0);

   return 0;
}

static char * test_iteratelist() {
	int i = 0;
	void *iterator = DoubleLinkedList_iterBegin(&list);
	while(DoubleLinkedList_iterIsValid(iterator)){
		switch(i){
			case 0:
				mu_assert("error, list entry != test", strcmp("test", (char*)iterator)==0);
				i++;
				break;
			case 1:
				mu_assert("error, list entry != abcd", strcmp("abcd", (char*)iterator)==0);
				i++;
				break;
			case 2:
				mu_assert("error, list entry != cba", strcmp("cba", (char*)iterator)==0);
				i++;
				break;
		}

		iterator = DoubleLinkedList_iterNext(iterator);
	}

	mu_assert("error, listcount != 3", i);
    return 0;
}

static char * test_iteratefreelist() {
	int i = 0;

	void *iterator = NULL;

	iterator = DoubleLinkedList_iterBegin(&list);
	iterator = DoubleLinkedList_iterNext(iterator);
	mu_assert("error, list entry != abcd", strcmp("abcd", (char*)iterator)==0);
	DoubleLinkedList_freeNode(&list, iterator);

	iterator = DoubleLinkedList_iterBegin(&list);
	mu_assert("error, list entry != test", strcmp("test", (char*)iterator)==0);
	DoubleLinkedList_freeNode(&list, iterator);

	iterator = DoubleLinkedList_iterBegin(&list);
	mu_assert("error, list entry != cba", strcmp("cba", (char*)iterator)==0);
	DoubleLinkedList_freeNode(&list, iterator);

	mu_assert("error, list not empty", DoubleLinkedList_isEmpty(&list));

    return 0;
}

static char * test_freelist() {
	int i = 0;
	void *iterator = DoubleLinkedList_iterBegin(&list);
	while(DoubleLinkedList_iterIsValid(iterator)){

		i++;
		DoubleLinkedList_freeNode(&list, iterator);

		iterator = DoubleLinkedList_iterNext(iterator);
	}
	mu_assert("error, listcount != 3", i);

    return 0;
}

static char * all_tests() {
    mu_run_test(test_init);

    mu_run_test(test_fillList);
    mu_run_test(test_iteratelist);
    mu_run_test(test_iteratefreelist);
    mu_run_test(test_fillList);
    mu_run_test(test_freelist);

    return 0;
}

int main(int argc, char **argv) {
    char *result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
