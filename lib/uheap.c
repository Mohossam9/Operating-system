
#include <inc/lib.h>

// malloc()
//	This function use BEST FIT strategy to allocate space in heap
//  with the given size and return void pointer to the start of the allocated space

//	To do this, we need to switch to the kernel, allocate the required space
//	in Page File then switch back to the user again.
//
//	We can use sys_allocateMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls allocateMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the allocateMem function is empty, make sure to implement it.


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

struct freespace
{
	uint32 startaddress;
	unsigned int size;
};

struct allocatedspace
{
	uint32 startaddress;
	unsigned int size;
};

int freeindex=-1;
int allocindex=0;

struct freespace freespaces[10000];
struct allocatedspace allocatedspaces[10000];

void initilize()
{
	uint32 startadd = USER_HEAP_START;
	uint32 endadd = USER_HEAP_MAX;

	freespaces[freeindex].startaddress=startadd;
    freespaces[freeindex].size=0;
	while(startadd<endadd)
	{
	   freespaces[freeindex].size+=PAGE_SIZE;
	   startadd+=PAGE_SIZE;
	}
	freeindex++;
}

void compact(uint32 freeaddress,unsigned int size)
{
       int previndex=-1;
	   int nextindex=-1;
	   uint32 prevadd=USER_HEAP_START-1;
	   uint32 nextadd=USER_HEAP_MAX+1;

       for(int i=0;i<freeindex;i++)
       {
    	   if(freespaces[i].startaddress<freeaddress && prevadd<freespaces[i].startaddress)
    	   {
    		   prevadd=freespaces[i].startaddress;
    		   previndex=i;
    	   }

    	   if(freespaces[i].startaddress>freeaddress && freespaces[i].startaddress<nextadd)
    	   {
    		   nextadd=freespaces[i].startaddress;
    		   nextindex=i;
    	   }
       }


       if(freespaces[previndex].startaddress+freespaces[previndex].size==freeaddress && freespaces[nextindex].startaddress==freeaddress+size)
       {
    	   freespaces[previndex].size+=size;
    	   freespaces[previndex].size+=freespaces[nextindex].size;
    	   freespaces[nextindex].startaddress=0;
    	   freespaces[nextindex].size=0;
       }

       else if(freespaces[previndex].startaddress+freespaces[previndex].size==freeaddress)
       {
    	   freespaces[previndex].size+=size;
       }

       else if(freespaces[nextindex].startaddress==freeaddress+size)
       {
    	   freespaces[nextindex].startaddress=freeaddress;
    	   freespaces[nextindex].size+=size;
       }
       else
       {
    	   freespaces[freeindex].startaddress=freeaddress;
    	   freespaces[freeindex].size=size;
    	   freeindex++;
       }
}



void* malloc(uint32 size)
{
	//TODO: [PROJECT 2019 - MS2 - [5] User Heap] malloc() [User Side]
	// Write your code here, remove the panic and write your code


	if(freeindex==-1)
	{
		freeindex++;
		initilize();
	}

	struct freespace *ptr_best=NULL;

	if(size<=0)
		return NULL;

	unsigned int num_pages=ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;
	int i=0;
	uint32 virtualadd;
	while(i<freeindex)
	{
		if(freespaces[i].size==size)
		{
			ptr_best=&freespaces[i];
			break;
		}

		if(freespaces[i].size>size)
		{
			if(ptr_best==NULL)
				ptr_best=&freespaces[i];
			else
			{
				if(ptr_best->size>freespaces[i].size)
				{
					ptr_best=&freespaces[i];
				}
			}
		}
		i++;
	}

	if(ptr_best==NULL)
	{
		return NULL;
	}

	else
	{
	  sys_allocateMem(ptr_best->startaddress,size);
	}

  	virtualadd=(uint32)ptr_best->startaddress;
	allocatedspaces[allocindex].startaddress=virtualadd;
    uint32 returnadd=virtualadd;

    for(int j=0;j<num_pages;j++)
    {
    	ptr_best->size-=PAGE_SIZE;
    	ptr_best->startaddress+=PAGE_SIZE;

   		allocatedspaces[allocindex].size+=PAGE_SIZE;
    	virtualadd+=PAGE_SIZE;
    }
    allocindex++;

    return (void *)returnadd;

	// Steps:
	//	1) Implement BEST FIT strategy to search the heap for suitable space
	//		to the required allocation size (space should be on 4 KB BOUNDARY)
	//	2) if no suitable space found, return NULL
	//	 Else,
	//	3) Call sys_allocateMem to invoke the Kernel for allocation
	// 	4) Return pointer containing the virtual address of allocated space,
	//

	//This function should find the space of the required range
	// ******** ON 4KB BOUNDARY ******************* //

	//Use sys_isUHeapPlacementStrategyBESTFIT() to check the current strategy

	//change this "return" according to your answer
}

void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//TODO: [PROJECT 2019 - MS2 - [6] Shared Variables: Creation] smalloc() [User Side]
	// Write your code here, remove the panic and write your code

	if(freeindex==-1)
    {
		freeindex++;
		initilize();
    }

	struct freespace *ptr_best=NULL;

	if(size<=0)
		return NULL;

	int i=0;
	while(i<freeindex)
	{
		if(freespaces[i].size==size)
		{
			ptr_best=&freespaces[i];
			break;
		}

		if(freespaces[i].size>size)
		{
			if(ptr_best==NULL)
				ptr_best=&freespaces[i];
			else
			{
				if(ptr_best->size>freespaces[i].size)
				{
					ptr_best=&freespaces[i];
				}
			}
		}
		i++;
	}

	if(ptr_best==NULL)
	{
		return NULL;
	}

	else
	{
		int ID=sys_createSharedObject(sharedVarName,size,isWritable,(void *)ptr_best->startaddress);
		if(ID<0)
		{
			return NULL;
		}

		    unsigned int num_pages=ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;

		    uint32 virtualadd=(uint32)ptr_best->startaddress;
			allocatedspaces[allocindex].startaddress=virtualadd;
		    uint32 returnadd=virtualadd;

		    for(int j=0;j<num_pages;j++)
		    {
		    	ptr_best->size-=PAGE_SIZE;
		    	ptr_best->startaddress+=PAGE_SIZE;

		   		allocatedspaces[allocindex].size+=PAGE_SIZE;
		    	virtualadd+=PAGE_SIZE;
		    }
		    allocindex++;

		return (void *)returnadd;
	}



	// Steps:
	//	1) Implement BEST FIT strategy to search the heap for suitable space
	//		to the required allocation size (space should be on 4 KB BOUNDARY)
	//	2) if no suitable space found, return NULL
	//	 Else,
	//	3) Call sys_createSharedObject(...) to invoke the Kernel for allocation of shared variable
	//		sys_createSharedObject(): if succeed, it returns the ID of the created variable. Else, it returns -ve
	//	4) If the Kernel successfully creates the shared variable, return its virtual address
	//	   Else, return NULL

	//This function should find the space of the required range
	// ******** ON 4KB BOUNDARY ******************* //

	//Use sys_isUHeapPlacementStrategyBESTFIT() to check the current strategy

	//change this "return" according to your answer
}

void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//TODO: [PROJECT 2019 - MS2 - [6] Shared Variables: Get] sget() [User Side]
	// Write your code here, remove the panic and write your code

	if(freeindex==-1)
	 {
         freeindex++;
	     initilize();
	}

	unsigned int size=sys_getSizeOfSharedObject(ownerEnvID,sharedVarName);

	if(size==E_SHARED_MEM_NOT_EXISTS)
	{
		return NULL;
	}

	struct freespace *ptr_best=NULL;

		if(size<=0)
			return NULL;

		int i=0;
		while(i<=freeindex)
		{
			if(freespaces[i].size==size)
			{
				ptr_best=&freespaces[i];
				break;
			}

			if(freespaces[i].size>size)
			{
				if(ptr_best==NULL)
					ptr_best=&freespaces[i];
				else
				{
					if(ptr_best->size>freespaces[i].size)
					{
						ptr_best=&freespaces[i];
					}
				}
			}
			i++;
		}

		if(ptr_best==NULL)
		{
			return NULL;
		}

		else
		{
			int sharedID=sys_getSharedObject(ownerEnvID,sharedVarName,(void *)ptr_best->startaddress);

			if(sharedID<0)
			{
				return NULL;
			}

			unsigned int num_pages=ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;

			uint32 virtualadd=(uint32)ptr_best->startaddress;
		    allocatedspaces[allocindex].startaddress=virtualadd;
			uint32 returnadd=virtualadd;

			for(int j=0;j<num_pages;j++)
			{
			    ptr_best->size-=PAGE_SIZE;
			    ptr_best->startaddress+=PAGE_SIZE;

			    allocatedspaces[allocindex].size+=PAGE_SIZE;
			    virtualadd+=PAGE_SIZE;
			 }
			 allocindex++;


			return (void *)returnadd;
		}


	// Steps:
	//	1) Get the size of the shared variable (use sys_getSizeOfSharedObject())
	//	2) If not exists, return NULL
	//	3) Implement BEST FIT strategy to search the heap for suitable space
	//		to share the variable (should be on 4 KB BOUNDARY)
	//	4) if no suitable space found, return NULL
	//	 Else,
	//	5) Call sys_getSharedObject(...) to invoke the Kernel for sharing this variable
	//		sys_getSharedObject(): if succeed, it returns the ID of the shared variable. Else, it returns -ve
	//	6) If the Kernel successfully share the variable, return its virtual address
	//	   Else, return NULL
	//

	//This function should find the space for sharing the variable
	// ******** ON 4KB BOUNDARY ******************* //

	//Use sys_isUHeapPlacementStrategyBESTFIT() to check the current strategy

	//change this "return" according to your answer
}

// free():
//	This function frees the allocation of the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from page file and main memory then switch back to the user again.
//
//	We can use sys_freeMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls freeMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the freeMem function is empty, make sure to implement it.

void free(void* virtual_address)
{
	//TODO: [PROJECT 2019 - MS2 - [5] User Heap] free() [User Side]
	// Write your code here, remove the panic and write your code

	for(int i=0;i<allocindex;i++)
	{
		if((void *)allocatedspaces[i].startaddress==virtual_address)
		{
			unsigned int size=allocatedspaces[i].size;
			allocatedspaces[i].size=0;
			allocatedspaces[i].startaddress=0;
			compact((uint32)virtual_address,size);
			sys_freeMem((uint32)virtual_address,size);
			break;
		}
	}

	//you should get the size of the given allocation using its address
	//you need to call sys_freeMem()
	//refer to the project presentation and documentation for details
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=============
// [1] sfree():
//=============
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	//TODO: [PROJECT 2019 - BONUS4] Free Shared Variable [User Side]
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");

	//	1) you should find the ID of the shared variable at the given address
	//	2) you need to call sys_freeSharedObject()

}


//===============
// [2] realloc():
//===============

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_moveMem(uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		which switches to the kernel mode, calls moveMem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		in "memory_manager.c", then switch back to the user mode here
//	the moveMem function is empty, make sure to implement it.

void *realloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT 2019 - BONUS3] User Heap Realloc [User Side]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");

}
