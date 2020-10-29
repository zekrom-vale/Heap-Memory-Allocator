#include "alloc.h"

const int MAGIC = 0x144b36e8;
const int CHUNK = 0x4000;
const size_t HEADER_SIZE = sizeof(struct header);

/**
*Returns the starting location of the space
*@param start the location of the heaer
*/
void* alloc_getVoid(struct header* start){
	return start + 1;
}

void* Mem_Alloc(int size){
	if(size <= 0)exit(E_BAD_ARGS);
	size_t s=size+HEADER_SIZE;
	void* start=linked_list_find(&s);
	if(start!=NULL){
		struct header* head=(struct header*)start;
		head->magic=MAGIC;
		head->size=s; //If atomic size is too small s is updated
		return alloc_getVoid(start);
	}
	else{
		#if EXPAND
		return alloc_getVoid(alloc_extend(size));
		#else
			exit(E_NO_SPACE);
		#endif
	}
}

int alloc_roundUp(int num, int mult){
	return num + mult - num % mult;
}

size_t alloc_calcSpace(size_t size){
  if (size < CHUNK / 0x40)
    return CHUNK;
  if (size < CHUNK / 0x4)
	  return alloc_roundUp(0x10 * size, CHUNK);
  return alloc_roundUp(0x4 * size, CHUNK);
}

#define BUFFER 0x20

struct header* alloc_extend(size_t size){
	size_t s = alloc_calcSpace(size + BUFFER);
	
	//Request
	void* allocated = init_request(s);

	if(allocated == NULL)exit(E_NO_SPACE);
    if(LIST!=NULL){
		//If size is smaller than ATTOMIC use that instead
		if(size < ATOMIC)size = ATOMIC;
		//If the allocated space is the same as the ATOMIC size
		//This should never happen with the given settings
		if(s!=ATOMIC){
			//Get the location for the free space
			linked_list_add(
				linked_list_offset((struct node*)allocated, size),
				s - size
			);
		}
		//Init header
		struct header* head = (struct header*)allocated;

		head->magic = MAGIC;
		head->size = size;
		return head;
	}
    return allocated;
}