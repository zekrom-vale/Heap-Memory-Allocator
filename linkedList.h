#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
//Only to provide print node nad linked list functions for other files
#include <stdio.h>
#include "init.h"
#include "mem.h"
#include "util.h"
#include "listFind.h"
#include "config.h"

/**
 * The free space node
 * @member next the next node in the list
 * @member prev the previous node in the list
 * @member size the size of the free space
 * @member end the end of the free space, if USE_END
 */
struct node{
	struct node* next;
	struct node* prev;
	size_t size;
#if USE_END
	struct nodeEnd* end;
#endif
};

#if USE_END
/**
 * The end of the free space to point used to linararly get the previous node
 */
struct nodeEnd{
	struct node* start;
};
#endif

/**
 * The base of the linked list and other information
 * @member first the first node of the linked list
 * @member last the last node of the linked list
 * @member the size of the linked list
 * @member MODE the mode of the allocation
 */
struct linkedList{
	struct node* first;
	struct node* last;
	int size;
	void* end;
	int MODE;
};

/**
 * The sudo constant pointer to the linked list
 */
extern struct linkedList* LIST;


bool linked_list_validate(struct node* start);
bool linked_list_validateEnd(struct nodeEnd* end);
struct node* linked_list_add(void* start, size_t size);
void linked_list_remove(struct node* n);
void linked_list_shift(struct node* start,size_t size);
void linked_list_sort();
void linked_list_print();
void linked_list_printLinkedList();
void linked_list_printNode(struct node* cur);
void linked_list_printNodeMore(struct node* cur);
#endif
