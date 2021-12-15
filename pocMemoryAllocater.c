#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef char ALIGN[32];

union header {
    struct {
        size_t size;
        unsigned int is_free;
        union header *next;
    } s;
    ALIGN stub;
};

typedef union header header_t;

header_t *head, *tail;



header_t *myGetFreeBlock(size_t size)
{
    header_t *current_block = head;
    printf("Search for block of size: %ul\n", size);
    while(current_block) {
        if (current_block->s.is_free && current_block->s.size >= size)
        {
            printf("\tFound block of size %lu\n", current_block->s.size);
            return current_block;
        }
        current_block = current_block->s.next;
    }
    printf("No suitable free block not found\n");
    return NULL;
}

void *myMalloc(size_t size)
{
    header_t *header;
    size_t total_size;
    void *block;
    if (!size)
    {
        printf("No bytes requested for allocation.\n");
        return NULL;
    }
    header = myGetFreeBlock(size);
    /* if (header) { */
    /*     header->s.is_free = 0; */
    /*     return (void*)(header+1); */
    /* } */
    total_size = sizeof(header_t) + size;
    printf("Allocating %zu bytes total. %lu bytes for the header, and %zu bytes requested\n", total_size, sizeof(header_t), size);

    block = sbrk(total_size);
    if (block == (void *) -1)
    {
        printf("Error allocating block\n");
        return NULL;
    }

    printf("Allocated new block at %x\n", block);
    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;

    if (!head)
    {
        printf("No nodes found. Setting block as head.\n");
        head = header;
    }
    if (tail)
    {
        printf("Pointing the previous tail block to this one.\n");
        tail->s.next = header;
    }
    printf("Setting current block as tail\n");
    tail = header;

    return (void*)(header+1);
}

void myFree(void *block)
{
    header_t *header, *tmp;
    void *program_break;

    if (!block)
    {
        printf("No blocks found to free.\n");
        return;
    }
    header = (header_t*)block -1;

    program_break = sbrk(0);
    printf("Current Break: 0x%x\n", program_break);
    if ((char*)block + header->s.size == program_break) {
        printf("Block: %x\n Block size: %x\n", block, header->s.size);
        if (head == tail) {
            printf("Settings head and tail to NULL\n");
            head = tail = NULL;
        } else {
            tmp = head;
            while(tmp) {
                if(tmp->s.next == tail){
                    printf("Reassigning tail block\n");
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        printf("Reducing sbrk from: %x\n", sbrk(0));
        sbrk(0 - sizeof(header_t) - header->s.size);
        printf("New sbrk at %x\n", sbrk(0));
        return;
    }
    header->s.is_free = 1;
}

void *myCalloc(size_t num, size_t nsize)
{
    size_t size;
    void *block;
    printf("Allocating %lu of size %lu\n", nsize, num);
    if (!num || !nsize)
    {
        printf("Cannot allocate 0 of anything\n");
        return NULL;
    }
    size = num * nsize;
    printf("Total space needed: %lu\n", size);
    if (nsize != size/num){
        printf("Potential multiplication overflow. Cancelling calloc\n");
        return NULL;
    }
    block = myMalloc(size);
    if (!block)
    {
        return NULL;
    }
    memset(block, 0, size);
    return block;
}

void *myRealloc(void *block, size_t size)
{
    header_t *header;
    void *ret;
    printf("Reallocating block at location: %x\n", block);
    if (!block || !size)
    {
        printf("No block passed. Allocating new block instead");
        return myMalloc(size);
    }
    header = (header_t*)block - 1;
    printf("Current size: %zu\n", header->s.size);
    printf("New size: %zu\n", size);
    if (header->s.size >= size)
    {
        printf("Current block already large enough. Block not reallocated\n");
        return block;
    }
    ret = myMalloc(size);
    if (ret)
    {
        printf("New block allocated. Copyinf data and freeing old block\n");
        memcpy(ret, block, header->s.size);
        myFree(block);
    }
}

void listBlocks()
{
    header_t *current_block;
    if (!head)
    {
        printf("No blocks found");
        return;
    }

    current_block = head;

    while (current_block)
    {
        printf("Block found at 0x%x\n", current_block);
        printf("\tSize: %zu\n", current_block->s.size);
        printf("\tFree?: %u\n", current_block->s.is_free);
        printf("\tNext block: 0x%x\n", current_block->s.next);
        int offset = *(int *)(current_block + 1);
        printf("\tContents: %x\n", offset);
        current_block = current_block->s.next;
    }
}



int main() {
    int *a = myMalloc(sizeof(int));
    int *b = myMalloc(sizeof(int));
    int *c = myMalloc(sizeof(int));
    *a = 1;
    *b = 2;
    *c = 3;
    myRealloc(a, 80);
    listBlocks();
    return 0;
}
