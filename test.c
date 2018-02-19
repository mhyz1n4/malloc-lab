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


#ifdef DRIVER
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* DRIVER */
static size_t get_size(void * p) {return *(unsigned int *) p & ~0x7;}
static size_t get_alloc(void * p) {return *(unsigned int *) p & 0x1;}

static unsigned long pack(unsigned long x, unsigned long y) { return x | y; }

static void put(char * p, size_t x){
    memcpy(p, &x, 8);
}

inline size_t get(char * p){
    size_t temp;
    memcpy(&temp, p, 8);
    return temp;
}

int main(void)
{
    size_t b = 0x1;
    size_t a = 0x1234567012345678;
    size_t c;
    char * m ;
    c = pack(b, a);
    //m = &c;
    size_t addr = (unsigned long) m;
    char * temp = (char * ) addr;
    printf("%p\n", m);
    printf("%p\n", addr);
    printf("%p\n", temp);
}
