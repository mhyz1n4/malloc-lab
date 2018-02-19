/*
 * mm.c
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*
 * If you want debugging output, uncomment the following. Be sure not
 * to have debugging enabled in your final submission
 */
//#define DEBUG

#ifdef DEBUG
/* When debugging is enabled, the underlying functions get called */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated */
#define dbg_printf(...)
#define dbg_assert(...)
#endif /* DEBUG */

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* DRIVER */

/* What is the correct alignment? */
#define ALIGNMENT 16


/*     global variables     */
//char * heap_start;
//void * seglist_start;
void * segregated_free_lists[11];
const int MAX_INDEX = 11;
//const size_t HEADER_SIZE = 8;
//const size_t MIN_BLOCK_SIZE = 32;


/*    functions prototype    */
//helper function
static void * extend_heap(size_t size);
static void insert_blk(char * p, size_t size);
static void delete_list(char * p);
static void * coalesce(char *p);
static int get_list_index (size_t size);

//inline funtions
inline void put(char * p, size_t x);
inline size_t get(char * p);
inline size_t get_size(char * p);
inline size_t get_alloc(char * p);
inline size_t pack(size_t size, size_t alloc);
inline char * get_header(char * p);
inline char * get_footer(char * p);

//mem_checker
static void print_free_blk(void);
static int free_block_in_list(void);
static void print_alloc_block ();




/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}


/*      inline functions define    */
//get size and alloc byte
inline size_t get_size(char * p){
    size_t size = get(p) & ~0x7;
    return size;
}
inline size_t get_alloc(char * p){
    size_t alloc = get(p) & 0x1;
    return alloc;
}

//pack the size and alloc info
inline size_t pack(size_t size, size_t alloc){
    return size | alloc;
}

//get header and footer 8 bytes and write header and footer 8 bytes
inline void put(char * p, size_t x){
    *((size_t *) p) = x;
}
inline size_t get(char * p){
    return *((size_t *) p);
}

//get header and footer address
inline char * get_header(char * p){
    return p - 8;
}
inline char * get_footer(char * p){
    return p + get_size(get_header(p)) - 16;
}

//get adjacent blocks
inline char * next_blk(char * p){
    return p + get_size(get_header(p));
}
inline char * prev_blk(char * p){
    return p - get_size(p - 16);
}

//get the address where prev and succ is stored in a free block
inline char * prev_ptr_addr(char * p){
    return p;
}
inline char * succ_ptr_addr(char * p){
    return p + 8;
}

//get the prev and succ free block address
inline char * get_prev_ptr(char * p){
    return (char *) get(p);
}
inline char * get_succ_ptr(char * p){
    return (char *) get(p + 8);
}

//save pointer
inline void save_ptr(char * p, char * x){
    put(p, (size_t) x);
}

//get if frev alloc

//max
inline size_t max(size_t x, size_t y){
    if (x > y){
        return x;
    }else{
        return y;
    }
}


/*        helper functions define      */
//extend the heap with size
inline static void *extend_heap(size_t size)
{
    void * ptr;
    size_t new_size;
    
    new_size = align(size);
    
    if ((ptr = mem_sbrk(new_size)) == (void *)-1)
        return NULL;
    
    put(get_header(ptr), pack(new_size, 0));
    put(get_footer(ptr), pack(new_size, 0));
    put(get_header(next_blk(ptr)), pack(0, 1));
    insert_blk (ptr, new_size);                     //insert the extended heap into the seg list, link the list with previous and next list
    
    return coalesce(ptr);
    
}

//coaleasce adjacent free blocks
inline static void * coalesce(char *p)
{
    size_t prev_alloc = get_alloc(get_header(prev_blk(p)));
    size_t next_alloc = get_alloc(get_header(next_blk(p)));
    size_t size = get_size(get_header(p));
    
    //mm_checkheap(211);
    //dbg_printf("......before coalesce.....\n");
    //dbg_printf("block size: %lu, prev alloc: %lu, next_alloc: %lu \n", size, prev_alloc, next_alloc);
    //dbg_printf("prev block size: %lu, next block size: %lu\n", get_size(get_header(prev_blk(p))), get_size(get_header(next_blk(p))));
    
    if (prev_alloc && next_alloc){
        return p;
    }else if (prev_alloc && !next_alloc){
        delete_list(p);
        delete_list(next_blk(p));
        size = size + get_size(get_header(next_blk(p)));
        put(get_header(p), pack(size, 0));
        put(get_footer(p), pack(size, 0));
    }else if (!prev_alloc && next_alloc){
        delete_list(p);
        delete_list(prev_blk(p));
        size = size + get_size(get_header(prev_blk(p)));
        put(get_footer(p), pack(size, 0));
        put(get_header(prev_blk(p)), pack(size, 0));
        p = prev_blk(p);
    }else {                                                     //both are free
        delete_list(p);
        delete_list(prev_blk(p));
        delete_list(next_blk(p));
        size = size + get_size(get_header(prev_blk(p))) + get_size(get_header(next_blk(p)));
        put(get_header(prev_blk(p)), pack(size, 0));
        put(get_footer(next_blk(p)), pack(size, 0));
        p = prev_blk(p);
    }
    
    insert_blk(p, size);
    
    //mm_checkheap(243);
    //dbg_printf("......after coalesce.....\n");
    
    return p;
}

//insert free block
static inline void insert_blk(char * p, size_t size){
    
    char * search = p;
    char * insert = NULL;    
    int index = get_list_index(size);
    search = segregated_free_lists[index];
    
    
    //search through the seg list and find suitable size block
    if(index == 6 || index == 5 ){
        while ((search != NULL) && (size < get_size(get_header(search)))){                  //sign change
            insert = search;
            search = get_prev_ptr(search);
        }
    
        //set prev list and next list
        if (search == NULL){
            if (insert != NULL){                        //size is too big
                save_ptr(prev_ptr_addr(p), NULL);
                save_ptr(succ_ptr_addr(p), insert);
                save_ptr(prev_ptr_addr(insert), p);
            }else {                                     //first encounter size block, insert into segregated_free_list
                save_ptr(prev_ptr_addr(p), NULL);
                save_ptr(succ_ptr_addr(p), NULL);
                segregated_free_lists[index] = p;
            }
        }else {
            if (insert != NULL){                        //found insert position between insert and search, update pointers
                save_ptr(prev_ptr_addr(p), search);
                save_ptr(succ_ptr_addr(search), p);
                save_ptr(succ_ptr_addr(p), insert);
                save_ptr(prev_ptr_addr(insert), p);
            }else{                                      //found a position that the succ ptr of the previous free list is NULL, insert new blok behind search
                save_ptr(prev_ptr_addr(p), search);
                save_ptr(succ_ptr_addr(search), p);
                save_ptr(succ_ptr_addr(p), NULL);
                segregated_free_lists[index] = p;
            }
        }
    }else{
        if (search == NULL){
            save_ptr(prev_ptr_addr(p), NULL);
            save_ptr(succ_ptr_addr(p), NULL);
            segregated_free_lists[index] = p;
        }else{
            save_ptr(prev_ptr_addr(p), search);
            save_ptr(succ_ptr_addr(search), p);
            save_ptr(succ_ptr_addr(p), NULL);
            segregated_free_lists[index] = p;
        }  
    }
    
    return;
}

//delete one list from the seg list
static void delete_list(char * p){
    int index = 0;
    size_t size = get_size(get_header(p));
    
    index = get_list_index(size);
    
    if (get_prev_ptr(p) != NULL){
        if (get_succ_ptr(p) != NULL){
            save_ptr(succ_ptr_addr(get_prev_ptr(p)), get_succ_ptr(p));
            save_ptr(prev_ptr_addr(get_succ_ptr(p)), get_prev_ptr(p));
        }else{
            save_ptr(succ_ptr_addr(get_prev_ptr(p)), NULL);
            segregated_free_lists[index] = get_prev_ptr(p);
        }
    }else{
        if (get_succ_ptr(p) != NULL){       //last block in the list
            save_ptr(prev_ptr_addr(get_succ_ptr(p)), NULL);
        }else{
            segregated_free_lists[index] = NULL;
        }
    }
    
}

//get the seg_list_index for given size
static int get_list_index(size_t size){
    
    int i = 0;
    if(size<=32) i= 0;
    else if(size<=64) i= 1;
    else if(size<=128) i= 2;
    else if(size<=256) i= 3;
    else if(size<=512) i= 4;
    else if(size<=1024) i= 5;
    else if(size<=2048) i=6;
    else if(size<=4096) i= 7;
    else if(size<=8192) i= 8;
    else if(size<=16384) i= 9;
    else i = 10;
    
    return i;
    
}

//place
static void * place(void * p, size_t size){
    size_t blk_size = get_size(get_header(p));
    size_t remain = blk_size - size;
    
    //mm_checkheap(363);
    //dbg_printf("......in place function before delete.....\n");
    //dbg_printf("delete pointer: %p\n", p);
    
    delete_list(p);
    
    if (remain <= 32){      //not enough space for new block
     put(get_header(p), pack(blk_size, 1));
     put(get_footer(p), pack(blk_size, 1));
     }
     else{
     put(get_header(p), pack(size, 1));
     put(get_footer(p), pack(size, 1));
     
     put(get_header(next_blk(p)), pack(remain, 0));
     put(get_footer(next_blk(p)), pack(remain, 0));
     insert_blk(next_blk(p), remain);
     //coalesce(next_blk(p));
     }
    
    //put(get_header(p), pack(blk_size, 1));
    //put(get_footer(p), pack(blk_size, 1));
    
    //mm_checkheap(382);
    //dbg_printf("......after place.....\n");
    
    return p;
}

//if the oldsize is bigger than newsize place the block in current pointer and coalesce
static void * realloc_place(void * p, size_t size){
    size_t oldsize = get_size(get_header(p));
    size_t remain = oldsize - size;
    
    /*
     dbg_printf("......in REALLOC before insert.....\n");
     dbg_printf("REALLOC POINTER: %p\n", p);
     dbg_printf("old block size: %lu\n", oldsize);
     dbg_printf("realloc new size: %lu\n", size);
     dbg_printf("realloc remain size: %lu\n", remain);
     //mm_checkheap(385);
     */
    
    
    if (remain <= 32){      //not enough space for new block
        put(get_header(p), pack(oldsize, 1));
        put(get_footer(p), pack(oldsize, 1));
    }else{
        put(get_header(p), pack(size, 1));
        put(get_footer(p), pack(size, 1));
        
        put(get_header(next_blk(p)), pack(remain, 0));
        put(get_footer(next_blk(p)), pack(remain, 0));
        insert_blk(next_blk(p), remain);
        //coalesce(next_blk(p));
    }
    
    //put(get_header(p), pack(oldsize, 1));
    //put(get_footer(p), pack(oldsize, 1));
    
    
    
    /*mm_checkheap(406);
     dbg_printf("......after realloc place.....\n");
     dbg_printf("free blk pointer: %p, header size: %lu, header alloc: %lu\n", next_blk(p), get_size(get_header(next_blk(p))), get_alloc(get_header(next_blk(p))));
     dbg_printf("next block\n");
     dbg_printf("free blk next blcok pointer: %p, header size: %lu, header alloc: %lu\n",
     next_blk(next_blk(p)),
     get_size(get_header(next_blk(next_blk(p)))),
     get_alloc(get_header(next_blk(next_blk(p)))));
     dbg_printf("free blk prev blcok pointer: %p, header size: %lu, header alloc: %lu\n",
     prev_blk(next_blk(p)),
     get_size(get_header(prev_blk(next_blk(p)))),
     get_alloc(get_header(prev_blk(next_blk(p)))));*/
    
    return p;
}

/*
 * Initialize: return false on error, true on success.
 */
bool mm_init(void)
{
    /* IMPLEMENT THIS */
    int index;
    char * heap_start;
    
    // Initialize segregated free lists
    for (index = 0; index < MAX_INDEX; index++) {
        segregated_free_lists[index] = NULL;
    }
    
    //create heap
    if ((long)(heap_start = mem_sbrk(4 * 8)) == -1)
        return false;
    
    //mm_checkheap(0);
    
    put(heap_start, 0);
    put(heap_start + 1 * 8, pack(16, 1));
    put(heap_start + 2 * 8, pack(16, 1));
    put(heap_start + 3 * 8, pack(0, 1));
    
    if (extend_heap(1<<6) == NULL)
        return -1;
    
    //mm_checkheap(0);
    
    return true;
}

/*
 * malloc
 */
void* malloc(size_t size)
{
    /* IMPLEMENT THIS */
    size_t new_size;
    size_t extended;
    int index = 0;
    //int j = 0;
    size_t search_size;
    void * p = NULL;
    
    if (size == 0){
        return NULL;
    }
    
    if (size <= 16){
        new_size = 32;
    }else {
        new_size = align(size + 16);
    }
    search_size = new_size;
    
    index = get_list_index(search_size);
    
    
    //p = segregated_free_lists[index];
    for (int i = index; i < MAX_INDEX; i++){
        p = segregated_free_lists[i];
        while( p != NULL){
            if (get_size(get_header(p)) >= new_size){
                place(p, new_size);
                return p;
            }
            p = get_prev_ptr(p);
        }
    }
    if (p == NULL){
        
        extended = new_size;
        if ((p = extend_heap(extended)) == NULL)
            return NULL;
    }
    
    p = place(p, new_size);
    
    return p;
}

/*
 * free
 */
void free(void* ptr)
{
    /* IMPLEMENT THIS */
    
    size_t size = get_size(get_header(ptr));
    
    put(get_header(ptr), pack(size, 0));
    put(get_footer(ptr), pack(size, 0));
    
    insert_blk(ptr, size);
    coalesce(ptr);
    
    return;
}

/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    /* IMPLEMENT THIS */
    size_t old_size = get_size(get_header(oldptr));
    size_t new_size = 0;
    char * new_ptr;
    size_t copy_size = 0;
    
    if (size == 0){
        mm_free(oldptr);
        return 0;
    }else if (oldptr == NULL){
        return malloc(size);
    }
    if(size <= 16){
        new_size = 32;
    }else{
        new_size = align(size + 16);
    }
    
    if (old_size == new_size){
        return oldptr;
    }else if (old_size > new_size){             //if old block have enough space for realloc size
        return realloc_place(oldptr, new_size);
    }else{
        new_ptr = malloc(new_size);
        if (new_ptr == NULL){
            return NULL;
        }
        copy_size = old_size - 16;
        memcpy(new_ptr, oldptr, copy_size);
        
        mm_free(oldptr);
        return new_ptr;
    }
}

/*
 * calloc
 * This function is not tested by mdriver, and has been implemented for you.
 */
void* calloc(size_t nmemb, size_t size)
{
    void* ret;
    size *= nmemb;
    ret = malloc(size);
    if (ret) {
        memset(ret, 0, size);
    }
    return ret;
}

/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void* p)
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static bool aligned(const void* p)
{
    size_t ip = (size_t) p;
    return align(ip) == ip;
}

/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno)
{
#ifdef DEBUG
    /* Write code to check heap invariants here */
    /* IMPLEMENT THIS */
    printf("heap start: %p\n", mem_heap_lo());
    printf("heap end: %p\n", mem_heap_hi());
    print_free_blk();
    print_alloc_block ();
    
#endif /* DEBUG */
    return true;
}

static void print_free_blk(void){
    int index = 0;
    int check = 0;
    char * p;
    for (index = 0; index < MAX_INDEX; index++){
        p = segregated_free_lists[index];
        if (p != NULL){
            check++;
            dbg_printf("p address: %p \n", p);
            dbg_printf("index: %d, header size : %lu header alloc: %lu \n", index, get_size(get_header(p)), get_alloc(get_header(p)));
            dbg_printf("index: %d, footer size : %lu footer alloc: %lu \n\n", index, get_size(get_footer(p)), get_alloc(get_footer(p)));
            while(get_succ_ptr(p) != NULL){
                p = get_succ_ptr(p);
                dbg_printf("succ address: %p\n", p);
                dbg_printf("succ of index: %d, header size : %lu header alloc: %lu \n", index, get_size(get_header(p)), get_alloc(get_header(p)));
                dbg_printf("succ of index: %d, footer size : %lu footer alloc: %lu \n\n", index, get_size(get_footer(p)), get_alloc(get_footer(p)));
            }
            while(get_prev_ptr(p) != NULL){
                p = get_prev_ptr(p);
                dbg_printf("prev address: %p\n", p);
                dbg_printf("prev of index: %d, header size : %lu header alloc: %lu \n", index, get_size(get_header(p)), get_alloc(get_header(p)));
                dbg_printf("prev of index: %d, footer size : %lu footer alloc: %lu \n\n", index, get_size(get_footer(p)), get_alloc(get_footer(p)));
            }
            
        }
    }
    if(check == 0){
        dbg_printf("\n\nno free list in segregated_free_lists\n\n");
    }
}

static int free_block_in_list(void){
    char * heap_start = mem_heap_lo() + 32;
    int index = 0;
    int check = 0;
    char * p;
    while (heap_start != NULL && get_size(get_header(heap_start)) != 0){
        if (get_alloc(get_header(heap_start)) == 0){ //if it finds a free block
            for (index = 0; index < 14; index++){
                p = segregated_free_lists[index];
                if(p != NULL){
                    if (p == heap_start){
                        check = 1;
                        return check;
                    }
                    p = get_prev_ptr(p);
                    while (get_prev_ptr(p) != NULL){
                        if (p == heap_start){
                            check = 1;
                            return check;
                        }
                        p = get_prev_ptr(p);
                    }
                }
            }
        }
    }
    return check;
}

static void print_alloc_block (){
    char * heap_start = mem_heap_lo() + 32;
    while(heap_start != NULL && get_size(get_header(heap_start)) != 0){
        if (get_alloc(get_header(heap_start)) == 1){
            dbg_printf("alloced p address: %p \n", heap_start);
            dbg_printf("header size : %lu header alloc: %lu \n", get_size(get_header(heap_start)), get_alloc(get_header(heap_start)));
            dbg_printf("footer size : %lu footer alloc: %lu \n\n", get_size(get_footer(heap_start)), get_alloc(get_footer(heap_start)));
        }
        heap_start = next_blk(heap_start);
        
    }
}

static int checkCoalesceAndFree(void){
    //char* current = list_head;
    int i;
    for (i = 0; i < MAX_INDEX; i++){
        char * current = segregated_free_lists[i];
        while(get_prev_ptr(current) != NULL){
            if (get_alloc(get_header(current)) || get_alloc(get_footer(current))){
                return 0; //if either the head or footer are marked allocated
            }
            /* this part checks that the free block's next and prev blocks are allocated (did not escape coalesce) */
            if (next_blk(current) != 0 && !get_alloc(get_header(next_blk(current)))){
                return 0;  //if it has a next and it is free
            }
            if (prev_blk(current+8) != 0 && !get_alloc(get_header(prev_blk(current))))  {
                return 0;  //if it has a prev and it is free
            }
        }
        current = get_prev_ptr(current);
    }
    return 1;
}
