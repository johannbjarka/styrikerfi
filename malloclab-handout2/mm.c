/*
 * mm.c -  Allocator based on an explicit free list, first fit placement
 * and boundary tag coalescing. 
 *
 * Each block has a header and footer of the form:
 * 
 *      31                     3  2  1  0 
 *      -----------------------------------
 *     | s  s  s  s  ... s  s  s  0  0  a/f
 *      ----------------------------------- 
 * 
 * where s are the meaningful size bits and a/f is set 
 * iff the block is allocated. The list has the following form:
 *
 * begin                                                          end
 * heap                                                           heap  
 *  -----------------------------------------------------------------   
 * |  pad   | hdr(8:a) | ftr(8:a) | zero or more usr blks | hdr(8:a) |
 *  -----------------------------------------------------------------
 *          |       prologue      |                       | epilogue |
 *          |         block       |                       | block    |
 *
 * The allocated prologue and epilogue blocks are overhead that
 * eliminate edge conditions during coalescing.
 *
 * An allocated block has the following format:
 *  ------------------------------------------------------------------
 * | hdr(8:a) | payload (at least 8 bytes ) | possible pad | ftr(8:a) |  
 *  ------------------------------------------------------------------
 *
 * A free block has the following format:
 *  -------------------------------------------------------------------
 * | hdr(8:f) |  prev ptr (4) | next ptr (4) | prev payload | ftr(8:f) | 
 *  -------------------------------------------------------------------
 *
 * The free list is a doubly linked list with NULL pointers at each end
 * of the list. When blocks are freed they are inserted at the front of 
 * the list. The list is traversed using a struct that consists of two
 * pointers, prev and next.
 * When memory is allocated the list is searched for a block big enough
 * using a first fit search. 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in below _AND_ in the
 * struct that follows.
 *
 * === User information ===
 * Group: Super_Dario_Bros. 
 * User 1: johannob01
 * SSN: 081281-5059
 * User 2: carl13
 * SSN: 211087-2939
 * === End User Information ===
 ********************************************************/
team_t team = {
    /* Group name */
    "Super_Dario_Bros.",
    /* First member's full name */
    "Jóhann Örn Bjarkason",
    /* First member's email address */
    "johannob01@ru.is",
    /* Second member's full name (leave blank if none) */
    "Carl Andreas Sveinsson",
    /* Second member's email address (leave blank if none) */
    "carl13@ru.is",
    /* Leave blank */
    "",
    /* Leave blank */
    ""
};

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* word size (bytes) */  
#define DSIZE       8       /* doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */
#define OVERHEAD    8       /* overhead of header and footer (bytes) */
#define MINIMUM		16		/* minimum size of block */

#define MAX(x, y) ((x) > (y)? (x) : (y)) 

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(size_t *)(p))
#define PUT(p, val)  (*(size_t *)(p) = (val))  

/* (which is about 54/100).* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)  
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
/* $end mallocmacros */

/* Global variables */
static char *heap_listp;  /* pointer to first block */
static char *free_listp; /* pointer to first free block */

/* function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp); 
static void checkblock(void *bp);
static void insertBlock(void *bp);
static void removeBlock(void *bp);
typedef struct pointers blockPtr;
static void checkfreeblock(blockPtr *p);
static int inFreelist(void *bp);


/* Structure for our doubly linked list */
struct pointers {
	blockPtr *prev;
	blockPtr *next;
};

/* 
 * mm_init - Initialize the memory manager 
 */
/* $begin mminit */
int mm_init(void) 
{
    /* create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == NULL)
        return -1;
    PUT(heap_listp, 0);                        /* alignment padding */
    PUT(heap_listp+WSIZE, PACK(OVERHEAD, 1));  /* prologue header */ 
    PUT(heap_listp+DSIZE, PACK(OVERHEAD, 1));  /* prologue footer */ 
    PUT(heap_listp+WSIZE+DSIZE, PACK(0, 1));   /* epilogue header */
    heap_listp += DSIZE;
	
	free_listp = NULL; /* Initialize free_listp */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}
/* $end mminit */

/* 
 * mm_malloc - Allocate a block with at least size bytes of payload 
 */
/* $begin mmmalloc */
void *mm_malloc(size_t size) 
{
    size_t asize;      /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char *bp;      

    /* Ignore spurious requests */
    if (size <= 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = DSIZE + OVERHEAD;
    else
        asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);
    
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
} 
/* $end mmmalloc */

/* 
 * mm_free - Free a block 
 */
/* $begin mmfree */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/* $end mmfree */

/*
 * mm_realloc - Reallocate a block.
 * Accepts a pointer to a block and a size.
 * If the size is smaller than the block was before the block is shrunk.
 * If the size is greater than before the block is extended.  
 */
void *mm_realloc(void *ptr, size_t size)
{
	/* If ptr is NULL we just call malloc */
	if(ptr == NULL)
	{
		return mm_malloc(size);
	}
	/* If size is less than or eaqual to 0 we just call free */
	if(size <= 0)
	{
		free(ptr);
		return 0;
	}
	/* We adjust block size to include overhead and alignment requirements */
	size_t newsize;
	if (size <= DSIZE)
	{
		newsize = DSIZE + OVERHEAD;
	}  
    else
	{
		newsize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);
	}
	
    void *newp;
    size_t copySize;
	copySize = GET_SIZE(HDRP(ptr));
	
	/* Do nothing if size is the same */
	if(newsize == copySize)
	{
		return ptr;
	}
	/* If the payload is reduced in size and there is sufficient space
	   for another block we create a free block after the block. */
	if(newsize + MINIMUM <= copySize)
	{
		PUT(HDRP(ptr), PACK(newsize, 1));
		PUT(FTRP(ptr), PACK(newsize, 1));
		size_t asize = copySize - newsize;
		PUT(HDRP(NEXT_BLKP(ptr)), PACK(asize, 0));
		PUT(FTRP(NEXT_BLKP(ptr)), PACK(asize, 0));
		newp = NEXT_BLKP(ptr);
		coalesce(newp);
		return ptr;
	}
	
	/* If the payload size is increased, the next block is free
	   and has sufficient space to hold the payload, we coalesce the two blocks. */
	if((newsize > copySize) && !(GET_ALLOC(HDRP(NEXT_BLKP(ptr)))) 
		&& ((newsize - copySize) <= (GET_SIZE(HDRP(NEXT_BLKP(ptr))))))
	{
		size_t freesize = GET_SIZE(HDRP(NEXT_BLKP(ptr))) - (newsize - copySize);
		
		removeBlock(NEXT_BLKP(ptr));
		PUT(HDRP(ptr), PACK(newsize + freesize, 1));
		PUT(FTRP(ptr), PACK(newsize + freesize, 1));
		return ptr;
	}
	
	/* If the payload size is increased and the previous block is free
	   and has sufficient space to hold the payload, we coalesce the two blocks. */
	if((newsize > copySize) && !(GET_ALLOC(HDRP(PREV_BLKP(ptr)))) 
		&& ((newsize - copySize) <= (GET_SIZE(HDRP(PREV_BLKP(ptr))))))
	{
		size_t freesize = GET_SIZE(HDRP(PREV_BLKP(ptr))) - (newsize - copySize);
		
		removeBlock(PREV_BLKP(ptr));
		newp = PREV_BLKP(ptr);
		PUT(HDRP(newp), PACK(newsize + freesize, 1));
		PUT(FTRP(newp), PACK(newsize + freesize, 1));
		memcpy(newp, ptr, copySize);
		return newp;
	}
	
	/* If the payload size is increased and both the previous and
	   the next blocks are free and have sufficient space to hold
	   the payload we coalesce the three blocks. */
	if((newsize > copySize) && !(GET_ALLOC(HDRP(PREV_BLKP(ptr)))) 
		&& !(GET_ALLOC(HDRP(NEXT_BLKP(ptr)))) 
		&& ((newsize - copySize) <= ((GET_SIZE(HDRP(PREV_BLKP(ptr)))) + (GET_SIZE(HDRP(NEXT_BLKP(ptr)))))))
	{
		size_t freesize = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + GET_SIZE(HDRP(PREV_BLKP(ptr))) - (newsize - copySize);
		
		removeBlock(PREV_BLKP(ptr));
		removeBlock(NEXT_BLKP(ptr));
		newp = PREV_BLKP(ptr);
		PUT(HDRP(newp), PACK(newsize + freesize, 1));
		PUT(FTRP(newp), PACK(newsize + freesize, 1));
		memcpy(newp, ptr, copySize);
		return newp;
	}
		
	/* If the payload size is increased and the block is the last 
	   block on the heap we manually extend the heap, increase
	   the size of the block and create a free block from the leftover space. */
	if((newsize > copySize) && (GET_SIZE(HDRP(NEXT_BLKP(ptr))) == 0))
	{
		if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		{
			return NULL;
		}
		size_t freesize = GET_SIZE(HDRP(NEXT_BLKP(ptr))) - (newsize - copySize);
		
		removeBlock(NEXT_BLKP(ptr));
		PUT(HDRP(ptr), PACK(newsize, 1));
		PUT(FTRP(ptr), PACK(newsize, 1));
		PUT(HDRP(NEXT_BLKP(ptr)), PACK(freesize, 1));
		PUT(FTRP(NEXT_BLKP(ptr)), PACK(freesize, 1));
		mm_free(NEXT_BLKP(ptr));
		return ptr;
	}
	/* If none of the previous cases apply, we call malloc and then free
       the old block. */
	else
	{
		if ((newp = mm_malloc(size)) == NULL) {
			printf("ERROR: mm_malloc failed in mm_realloc\n");
			exit(1);
		}
		
		if (size < copySize)
			copySize = size;
		memcpy(newp, ptr, copySize);
		mm_free(ptr);
		return newp;
	}
}

/* 
 * mm_checkheap - Check the heap for consistency 
 */
void mm_checkheap(int verbose) 
{
    char *bp = heap_listp;

    if (verbose)
        printf("Heap (%p):\n", heap_listp);
	
	/* Checks if the prologue header is allocated and is the correct size */
    if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
        printf("Bad prologue header\n");
	checkblock(heap_listp);

	/* Checks if every block on the heap is aligned correctly and has a
	   matching header and footer. Checks for contiguous free blocks. 
	   Checks if every free block is in the free list. */
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (verbose) 
            printblock(bp);
        checkblock(bp);
    }
     
    if (verbose)
        printblock(bp);
	/* Checks if the epilogue header is allocated and is the correct size */
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
        printf("Bad epilogue header\n");
	
	blockPtr *p = (blockPtr *)free_listp;
	/* Checks if every block in the free list is marked as free */
    for (; p != NULL; p = p->next) 
	{
		if(GET_ALLOC(p))
		{
			printf("Error: %p is not free\n", p);
		}
		/* Checks if prev and next point to addresses within heap bounds
		  and if the blocks they point to are actually free. */
		checkfreeblock(p);
    } 
}

/* The remaining routines are internal helper routines */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
/* $begin mmextendheap */
static void *extend_heap(size_t words) 
{
    char *bp;
    size_t size;
        
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((bp = mem_sbrk(size)) == (void *)-1) 
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}
/* $end mmextendheap */

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
/* $begin mmplace */
/* $begin mmplace-proto */
static void place(void *bp, size_t asize)
/* $end mmplace-proto */
{
    size_t csize = GET_SIZE(HDRP(bp));   

    if ((csize - asize) >= (DSIZE + OVERHEAD)) { 
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
		removeBlock(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
		coalesce(bp);
    }
    else { 
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
		removeBlock(bp);
    }
}
/* $end mmplace */

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
static void *find_fit(size_t asize)
{
    /* first fit search */
    blockPtr *p = (blockPtr *)free_listp;
	
    for (; p != NULL; p = p->next) {
        if (!GET_ALLOC(HDRP(p)) && (asize <= GET_SIZE(HDRP(p)))) {
            return (void *)p;
        }
    }
    return NULL; /* no fit */
}

/*
 * coalesce - boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce(void *bp) 
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
	
	/* If the previous and next block are both allocated 
	   we insert the block into the free list and return */
    if (prev_alloc && next_alloc) {            
		insertBlock(bp);
        return bp;
    }
	/* If only the next block is free we coalesce the block and
	   the next block */
    else if (prev_alloc && !next_alloc) {
		removeBlock(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }
	/* If only the previous block is free we coalesce the block and
	   the previous block */
    else if (!prev_alloc && next_alloc) {
		removeBlock(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
	/* If both the next and the previous blocks are free we coalesce 
	   all three blocks */
    else {
		removeBlock(NEXT_BLKP(bp));
		removeBlock(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
	
	insertBlock(bp);
    return bp;
}

/* 
 *  Function that takes a block and inserts it at the
 *  front of the free list.
 */
static void insertBlock(void *bp)
{
	/* If the free list is empty we initialize it by having it point 
	   to the block and setting prev and next to NULL. */
	if(free_listp == NULL)
	{
		free_listp = bp;
		blockPtr *p = (blockPtr *)free_listp;
		p->prev = NULL;
		p->next = NULL;
	}
	/* Inserts bp at the front of the list and updates free_listp. */
	else
	{
		blockPtr *p = (blockPtr *)free_listp;
		p->prev = bp; 
		blockPtr *p2 = bp;
		p2->prev = NULL;
		p2->next = p;
		free_listp = (char *)p2;
	}
}

/* Function that removes a block from the free list */
static void removeBlock(void *bp)
{
	blockPtr *p = bp;
	if(p->next != NULL)
	{
		p->next->prev = p->prev;
	}
	if(p->prev != NULL)
	{
		p->prev->next = p->next;
	}
	/* If prev is NULL that means the bp is at the front 
	   of the list so we must update free_listp. */ 
	else
	{
		free_listp = (char *)p->next;
		if(p->next != NULL)
		{
			p->next->prev = NULL;
		}
	}
}

/* 
 *  Function to help with debugging. Goes through the free list 
 *  and prints every block in the free list.
 */
void printfreelist()
{
	blockPtr *p = (blockPtr *)free_listp;
	
    for (; p != NULL; p = p->next) 
	{
		printblock((void *)p);
    } 
}

/* 
 * Checks if prev and next point to addresses within heap bounds
 * and if the blocks they point to are actually free.
 */
static void checkfreeblock(blockPtr *p)
{
	if(p->prev != NULL) 
	{
		/* Check if prev points to a block within heap bounds */
		if(p->prev < (blockPtr *)mem_heap_lo() || p->prev  > (blockPtr *)mem_heap_hi())
		{
			printf("Error: pointer %p is not within heap bounds \n", p->prev);
		}
		/* Check if prev points to a free block */
		if(GET_ALLOC(HDRP(p->prev)))
		{
			printf("Error: pointer %p points to an allocated block \n", p->prev);
		}
	}	
	
	if(p->next != NULL )
	{
		/* Check if next points to a block within heap bounds */
		if(p->next < (blockPtr *)mem_heap_lo() || p->next  > (blockPtr *)mem_heap_hi())
		{
			printf("Error: pointer %p is not within heap bounds \n", p->next);
		}
		/* Check if next points to a free block */
		if(GET_ALLOC(HDRP(p->next)))
		{
			printf("Error: pointer %p points to an allocated block \n", p->next);
		}
	}
}

/* Accepts a pointer to a block and prints it out */
static void printblock(void *bp) 
{
    size_t hsize, halloc, fsize, falloc;

    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));  
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));  
    
    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }

    printf("%p: header: [%d:%c] footer: [%d:%c]\n", bp, 
           hsize, (halloc ? 'a' : 'f'), 
           fsize, (falloc ? 'a' : 'f')); 
}

/* 
 * Checks if a block is aligned correctly and has a matching header and footer.
 * Checks for contiguous free blocks. Checks if a free block is in the free list. 
 */
static void checkblock(void *bp) 
{
    if ((size_t)bp % 8)
        printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
        printf("Error: header does not match footer\n");
	if(!GET_ALLOC(HDRP(bp)))
	{
		if(!GET_ALLOC(HDRP(PREV_BLKP(bp))) || !GET_ALLOC(HDRP(NEXT_BLKP(bp))))
		{
			printf("Error: contiguous free blocks next to %p\n", bp);
		}
		
		if(!inFreelist(bp))
		{
			printf("Error: free block at %p is not in free list\n", bp);
		}
	}
}

/* Checks if a block is in the free list or not */
static int inFreelist(void *bp)
{
	blockPtr *p = (blockPtr *)free_listp;
	
    for (; p != NULL; p = p->next) 
	{
		if(p == bp)
		{
			return 1;
		}
    } 
	return 0;
}