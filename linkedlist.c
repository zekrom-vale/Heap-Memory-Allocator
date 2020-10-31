#include "linkedList.h"

struct linkedList* LIST;

#define LIST_HEAD sizeof(struct linkedList)

#if USE_END
/**
 *The size of the end node
 */
const size_t END = sizeof(struct nodeEnd);


/**
 *The absolute minumum of the free space
 *It is posible to allocate if the size is the same as struct node, but that
 *requres more logic
 */
#define RAW_ATOMIC sizeof(struct node) + END + 1
#else
#define RAW_ATOMIC sizeof(struct node)
#endif
#define ATOMIC RAW_ATOMIC + ALIGN - RAW_ATOMIC % ALIGN

/**
*gets the next location of the node
*@param start the original node
*@param size the offset
*/
struct node* linked_list_offset(struct node* start, size_t size){
	assert(size >= ATOMIC);
	struct node* next = (struct node*)util_ptrAdd(start, size + 1);
	return next;
}

#if USE_END
/**
*returns the location of the end of the node
*@param start the node to get the end of
*/
struct nodeEnd* linked_list_getNodeEnd(struct node* start){
	return (struct nodeEnd*)util_ptrAdd(start, start->size-END);
	//|x|x|x|x|x|x|x|x|x| | | | | | | | | | | | |e|e|e|e| |
	// 0                 1                   2
	// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
}

struct node* linked_list_getPrevNode(struct node* start) {
  struct nodeEnd* end = util_ptrSub((struct nodeEnd*)start - 1, 1);
  if(linked_list_validateEnd(end))return end->start;
  return NULL;
}
#endif

/**
*validates the given node
*@code{validate(node, node->end);}
*@param start the node to verify
*@param end the corisponding end node to validate
*/
bool linked_list_validate(struct node* start){
#if VALIDATE
	if(start==NULL||start<LIST)return false;
#if USE_END
	struct nodeEnd* end=start->end;
	if(end<LIST)return false;
	return !(start==NULL || end==NULL || end->start != start);
#endif
	return true;
#endif
}

bool linked_list_validateEnd(struct nodeEnd* end) {
#if VALIDATE
  if (end == NULL || end < LIST) return false;
#if USE_END
  struct node* start = end->start;
  if(start<LIST)return false;
  return !(start == NULL || end == NULL || start->end != end);
#endif
  return true;
#endif
}

/**
*Creates the linkedList
*/
struct linkedList* linked_list_init(struct linkedList* ptr) {
	LIST = ptr;
	LIST->size=0;
	LIST->first=NULL;
	LIST->last=NULL;
	return LIST;
}

void linked_list_readd(struct node* n){
	LIST->size++;
	//WARNING:
	//Needs to be done first due to corruption
	if(LIST->first == NULL){
		n->next = NULL;
		n->prev = NULL;
		LIST->first = n;
		LIST->last = n;
	}
	else{
		struct node* last = LIST->last;
		LIST->last = n;
		n->next = NULL;
		n->prev = last;
		last->next = n;
	}
}

/**
*adds the given location with the given size
*this DOES NOT coalesce
*@param start the space to add
*@param size the size of the node
*/
struct node* linked_list_add(void* start, size_t size){
  if (size > MAX_SIZE) exit(E_NO_SPACE);
  struct node* n = (struct node*)start;
	n->size = size;
#if USE_END
	n->end=linked_list_getNodeEnd(start);
	n->end->start=n;
#endif
	linked_list_readd(n);
	return n;
}

/**
*removes the given node
*@param n the node to remove
*/
void linked_list_remove(struct node* n){
	assert(LIST->size!=0);
	assert(n!=NULL);
	//Clear the end node
#if USE_END
	struct nodeEnd* end=n->end;
	linked_list_validate(n);
	end->start=NULL;
#endif
	LIST->size--;
	//Attach the next and perv to eachother if they exist
	//If not update the LIST
	struct node* prev = n->prev;
	struct node* next=n->next;
	if(next!=NULL)next->prev=prev;
	else LIST->last=prev;
	if(prev!=NULL)prev->next=next;
	else LIST->first=next;
	//dealocate node
}

/**
*shifts the given node the given size
*@param start the node to shift
*@param size the size to offest
*/
void linked_list_shift(struct node* start, size_t size){
	struct node* newstart = linked_list_offset(start,size);
	struct node* next = start->next;
	struct node* prev = start->prev;
	//Update end node
#if USE_END
	struct nodeEnd* end=start->end;
	end->start=newstart;
	newstart->end=end;
#endif
	//copy to new node
	newstart->next=next;
	newstart->prev=prev;
	newstart->size=start->size-size;

	//re-point other nodes

	if(next!=NULL)next->prev=newstart;
	else LIST->last=newstart;
	if(prev!=NULL)prev->next=newstart;
	else LIST->first=newstart;
}

/**
*coalesces the linked list
*@param start the node to coalesce arround
*/
void linked_list_coalesce(struct node* start){
  struct node* next = util_ptrAdd(linked_list_getNodeEnd(start)+1,1);
	if(linked_list_validate(next)){
		start->size+=next->size;
		linked_list_remove(next);
#if USE_END
		start->end=next->end;
		start->end->start=start;
#endif
	}
#if USE_END
	struct node* prev = linked_list_getPrevNode(start);
	if(prev!=NULL){
		prev->size+=start->size;
		linked_list_remove(start);
		prev->end=next->end;
		prev->end->start=prev;
	}
#endif
}

/**
*Finds the next space avalbe
*@param s the size to find, updated if remaning space is not attomic
*@return a pointer to the allocated space, NULL if not found (Does not expand memory)
*/
void* linked_list_find(size_t* s){
	switch(LIST->MODE){
		case FIRSTFIT:
			return linked_list_findFirstFit(s);
		case BESTFIT:
			return linked_list_findBestFit(s);
		case WORSTFIT:
			return linked_list_findWorstFit(s);
	}
	return NULL;
}

void* linked_list_process(size_t* s, struct node* start){
	if(start->size < *s){
		//Failure space is too small
		return NULL;
	}
	// If the remaning space is not attomic allocate more and update s
	size_t min = *s + ATOMIC;
	if(start->size - *s < min){
		*s = min;
		linked_list_remove(start);
	}
	//Otherwise just sift it
	else{
		if(*s < ATOMIC) *s=ATOMIC;
		linked_list_shift(start, *s);
	}
	//Return the location of the space
	return start;
}

void* linked_list_findFirstFit(size_t* s){
	struct node* cur=LIST->first;
	size_t size=*s;
	while(cur!=NULL){
		//If the size is larger than requested
		if(cur->size>=size){
			//If a perfect match
			if(cur->size==size){
				linked_list_remove(cur);
				return cur;
			}
			return linked_list_process(s, cur);
		}
		cur=cur->next;
	}
	//Failure
	return NULL;
}

void* linked_list_findWorstFit(size_t* s){
	struct node* cur = LIST->first;
	struct node* large = cur;
	size_t size = *s;
	while(cur != NULL){
		//If the size is larger than requested
		if(cur->size >= size){
			//If a perfect match
			if(cur->size == size){
				linked_list_remove(cur);
				return cur;
			}
			//If cur is larger replace large
			if(cur->size > large->size)large=cur;
		}
		cur=cur->next;
	}
	return linked_list_process(s, large);
}


void* linked_list_findBestFit(size_t* s){
	struct node* cur = LIST->first;
	struct node* small = cur;
	size_t size = *s;
	while(cur != NULL){
		//If the size is larger than requested
		if(cur->size >= size){
			//If a perfect match
			if(cur->size == size){
				linked_list_remove(cur);
				return cur;
			}
			//If cur is larger replace large
			if(
				cur->size > size
				&& 
				cur->size < small->size
			)small = cur;
		}
		cur=cur->next;
	}
	return linked_list_process(s, small);
}

void linked_list_swap(struct node* A, struct node* B){
	if(A == B)return;
	struct node* Aprev = A->prev;
	struct node* Anext = A->next;
	if(Anext != NULL)Anext->prev = B;
	else LIST->last = B;
	if(Aprev != NULL)Aprev->next = B;
	else LIST->first = B;

	struct node* Bprev = B->prev;
	struct node* Bnext = B->next;
	if(Bnext != NULL)Bnext->prev = A;
	else LIST->last = A;
	if(Bprev != NULL)Bprev->next = A;
	else LIST->first = A;
}

void linked_list_addAt(struct node* before, struct node* n){
	assert(before != NULL);
	struct node* after = before->next;
	before->next = n;
	n->prev = before;
	if(after!=NULL){
		after->prev = n;
		n->next = after;
	}
	else{
		n->next = NULL;
		LIST->last = n;
	}
}

void linked_list_sortInsert(struct node* cur, struct node* new){
	if(cur == NULL)linked_list_readd(new);
	else if(cur > new)linked_list_addAt(cur, new);
	else{
		while(cur->next != NULL && cur->next < new)
			cur = cur->next;
		linked_list_addAt(cur, new);
	}
}

void linked_list_sort(){
	struct node* sort = NULL;
	struct node* cur = LIST->first;
	while(cur!=NULL){
		struct node* next = cur->next;
		linked_list_remove(cur);
		linked_list_sortInsert(sort, cur);
		cur = next;
	}
}

#include <stdio.h>
void linked_list_print(){
	struct node* cur = LIST->first;
	while(cur!=NULL){
		printf("%p\n",cur);
		cur = cur->next;
	}
}