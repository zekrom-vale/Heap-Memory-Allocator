#include "alloc.h"

/**
 * Returns the starting location of the space
 * @param start the location of the heaer
 * @return the pointer of the free/allocd space
 */
void* alloc_getVoid(struct header* start){
	return start + 1;
}

/**
 * Alocates new space to be used
 * @param size the minimum size to allocate
 * If the size is less than the ATOMIC size it will allocate the atomic size
 * If not a factor of ALIGN (0x8) it will round up
 * If the remaning free space is less than ATOMIC it will include that space
 * @return The pointer to the starting space of the allocated space
 */
void* Mem_Alloc(int size){
	if(size <= 0)exit(E_BAD_ARGS);
	size_t s = (size_t)util_roundUp(
	  size + sizeof(struct header),
	  ALIGN
	);
	void* start=list_find_find(&s);
	if(start!=NULL){
		struct header* head=(struct header*)start;
		head->magic=MAGIC;
		head->size=s; //If atomic size is too small s is updated
		return alloc_getVoid(start);
	}
	else{
		#if EXPAND
		return alloc_getVoid(alloc_extend((size_t)size));
		#else
			exit(E_NO_SPACE);
		#endif
	}
}

/**
 * Calculates the extended space
 * @param size the size to modify
 * @return the modified size
 */
size_t alloc_calcSpace(size_t size){
  if (size < CHUNK / 0x40)
    return CHUNK;
  if (size < CHUNK / 0x4)
	  return util_roundUp(0x10 * size, CHUNK);
  return util_roundUp(0x4 * size, CHUNK);
}

#define BUFFER 0

/** 
 * Extends the space by using mmap
 * @param size the size hint to expand by
 * @return the header of the expanded size
 */
struct header* alloc_extend(size_t size){
	size_t s = alloc_calcSpace(size + BUFFER);
	
	//Request
	void* allocated = init_request(s);

	if(allocated == NULL)exit(E_NO_SPACE);
	//If size is smaller than ATTOMIC use that instead
	if(size < ATOMIC)size = ATOMIC;
	//If the allocated space is the same as the ATOMIC size
	//This should never happen with the given settings
	if(s!=ATOMIC){
		//Get the location for the free space
		linked_list_add(
			list_find_offset((struct node*)allocated, size),
			s - size
		);
	}
	//Init header
	struct header* head = (struct header*)allocated;

	head->magic = MAGIC;
	head->size = size;
	return head;
}

/** 
 * mmaps a new space for the start of the allocations
 * @param size the minimum size to expand
 * @return the header of the new mmaped space
 */
struct header* alloc_extendInit(size_t size){
	size_t s = util_roundUp(
		size + BUFFER
		+ sizeof(struct linkedList),
		CHUNK
	);
	
	// Request
	struct linkedList* allocated =
		(struct linkedList*)init_request(s);

	if(allocated == NULL)exit(E_NO_SPACE);
	init_list(allocated);
	linked_list_add(
		allocated + 1,
		s - sizeof(struct linkedList)
	);
	return (struct header*)allocated;
}