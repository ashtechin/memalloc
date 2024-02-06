// Includes
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

// MACROS



// Structs and classes

typedef char ALIGN[16];

union header
{
    struct
    {
        size_t size;
        unsigned is_free;
        union header* next;

    } s;
    ALIGN stub;
};
typedef union header header_t;
header_t *head=NULL, *tail = NULL;
pthread_mutex_t global_malloc_lock;


// Functions

header_t *get_free_block(size_t size){
    header_t *curr = head;
    while(curr){
        if(curr->s.is_free && curr->s.size>=size)
            return curr;
        curr = curr->s.next;
    }
    return NULL;
}

void free(void* block){
    header_t *header, *tmp;
    void* programbreak;  // end of current heap of memory, given by sbrk(0)

    if(!block)
        return;
    
    pthread_mutex_lock(&global_malloc_lock);

    header = (header_t*)block - 1;  // header points just before the block we want to free
    programbreak = sbrk(0);

    // if the block is just at the end of heap, we free the memory and give it to OS
    if((char*)block + header->s.size == programbreak){
        if(head == tail)
            head = tail = NULL;
        else{
            tmp = head;
            while(tmp){
                if(tmp == tail){
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        // -ve sbrk arg == free heap memory
        sbrk(0 - header->s.size - sizeof(header_t));
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }
    // just mark the block as free to use later
    header->s.is_free = 1;
    pthread_mutex_unlock(&global_malloc_lock);
    
}

void *malloc(size_t size){
    size_t total_size;
    void* block;
    header_t* header;

    if(size == 0)
        return NULL;
    
    pthread_mutex_lock(&global_malloc_lock);
    header = get_free_block(size);  // find if a fitting block is already free

    if(header){
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void*)(header + 1);
    }

    // Allot new memory if a fitting block is not found
    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);
    if(block == (void*) -1){
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }

    header = (header_t*)block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;

    if(!head)
        head = header;
    if(tail)
        tail->s.next = header;
    tail = header;
    pthread_mutex_unlock(&global_malloc_lock);
    return (void*)(header + 1);

}

void *calloc(size_t num, size_t nsize){
    size_t size;
    void *block;

    if(num == 0 || nsize == 0)
        return NULL;
    
    size = num*nsize;
    if(nsize!=size/num)
        return NULL;
    
    block = malloc(size);
    if(!block)
        return NULL;
    memset(block, 0, size);
    return block;
    
}

void *realloc(void *block, size_t size){
    header_t *header;
    void* curr;
    if(block == NULL || size == 0)
        return NULL;
    header = (header_t*)block - 1;
    if(header->s.size >= size)
        return block;
    
    curr = malloc(size);
    if(curr){
        memcpy(curr, block, header->s.size);
        free(block);
    }
    return curr;
}

void print_mem_list(){
    header_t *curr = head;
    printf("head = %p, tail = %p \n", (void*)head, (void*)tail);
    while(curr){
        printf("addr = %p, size = %zu, is_free=%u, next=%p\n",
			(void*)curr, curr->s.size, curr->s.is_free, (void*)curr->s.next);
		curr = curr->s.next;
    }
}



/* Commands to replace your current memalloc in c with this
1. $ gcc -o memalloc.so -fPIC -shared memalloc.c
2. $ export LD_PRELOAD=$PWD/memalloc.so
*/