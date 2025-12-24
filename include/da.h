#ifndef DYNAMIC_ARRAY_H_
#define DYNAMIC_ARRAY_H_

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// constants
#define DEFAULT_CAPACITY 8
#define GROWTH_FACTOR    2

////////////////////////////////// DYNAMIC ARRAY //////////////////////////////////

/*
Usage:
T* arr = NULL;

da_free()
da_append()
da_resize()
da_print()

*/

typedef struct {
    size_t capacity;
    size_t count;
    void* items;
} da_header_t;

// cast to header then move to start of header
#define da_header(da) ((da_header_t*) (da) - 1)
// (da) is the same as (da != NULL)
#define da_count(da)           ((da) ? da_header(da)->count : 0)
#define da_capacity(da)        ((da) ? da_header(da)->capacity : 0)
#define da_set_capacity(da, c) (da_header(da)->capacity = c)
#define da_free(da)                                                                                                    \
    do {                                                                                                               \
        fprintf(stderr, "%s:%d:Free\n", __FILE__, __LINE__);                                                           \
        if (da != NULL)                                                                                                \
            free((da_header(da)));                                                                                     \
        da = NULL;                                                                                                     \
    } while (0)

#define da_append(da, item)                                                                                            \
    do {                                                                                                               \
        (da)                         = (da == NULL) ? da_init(da, sizeof *(da)) : da_accommodate(da, sizeof *(da), 1); \
        (da)[da_header(da)->count++] = (item);                                                                         \
    } while (0)

void* da_accommodate(void* arr, size_t item_size, size_t n_items);
void* da_init(void* da, size_t item_size);

////////////////////////////////// QUEUE //////////////////////////////////
typedef struct {
    size_t capacity;
    size_t count;
    size_t front;
    size_t back;
    void* items;
} daq_header_t;

// cast to header then move to start of header
#define daq_header(q) ((daq_header_t*) (q) - 1)

// (da) is the same as (da != NULL)
#define daq_front(da)          ((q) ? daq_header(q)->front : 0)
#define daq_back(da)           ((q) ? daq_header(q)->back : 0)
#define daq_count(da)          ((q) ? daq_header(q)->count : 0)
#define daq_capacity(da)       ((q) ? daq_header(q)->capacity : 0)
#define daq_set_capacity(q, c) (daq_header(q)->capacity = c)
void* daq_accommodate(void* q, size_t item_size, size_t n_items);
void* daq_init(void* q, size_t item_size);

#define enqueue(q, item)                                                                                               \
    do {                                                                                                               \
        (q) = ((q) == NULL) ? daq_init((q), sizeof *(q)) : daq_accommodate((q), sizeof *(q), 1);                       \
        daq_header((q))->count++;                                                                                      \
        (q)[daq_back((q))]    = (item);                                                                                \
        daq_header((q))->back = (daq_back((q)) + 1) % daq_capacity((q));                                               \
    } while (0)

#define dequeue(q, item)                                                                                               \
    do {                                                                                                               \
        assert((q) != NULL && daq_count((q)) > 0 && "cannot dequeue from queue");                                      \
        item                   = (q)[daq_front((q))];                                                                  \
        daq_header((q))->front = (daq_front((q)) + 1) % daq_capacity((q));                                             \
        daq_header((q))->count--;                                                                                      \
    } while (0)

////////////////////////////////// Stack //////////////////////////////////

typedef da_header_t stack_header_t;

#define stack_count      da_count
#define stack_push       da_append
#define stack_pop(stack) (da_header(stack)->count--, (stack)[da_header(stack)->count])
// #define stack_empty(stack) (da_header(stack)->count == 0)
#define stack_empty(stack) ((stack) ? da_header(stack)->count == 0 : true)
#define stack_free(stack)  da_free(free)
#endif // DYNAMIC_ARRAY_H_

#ifdef DYNAMIC_ARRAY_IMPLEMENTATION

void* da_init(void* da, size_t item_size) {

    size_t capacity = DEFAULT_CAPACITY;
    fprintf(stderr, "%s:%d:Allocation\n", __FILE__, __LINE__);
    void* tmp = realloc(NULL, item_size * capacity + sizeof(da_header_t));
    if (tmp == NULL) {
        fprintf(stderr, "(re)Allocation of %p failed at %s:%d\n", tmp, __FILE__, __LINE__);
        return NULL;
    }
    tmp = (char*) tmp + sizeof(da_header_t); // use char* cast to move by bytes
    // da_header(tmp)->items    = 0;
    da_header(tmp)->count    = 0;
    da_header(tmp)->capacity = DEFAULT_CAPACITY;
    da                       = tmp;
    return da;
}

void* da_accommodate(void* da, size_t item_size, size_t n_items) {

    // if da is null we need to make a new header
    size_t cap       = da_capacity(da);
    size_t ocap      = cap;
    size_t count     = da_count(da);
    size_t new_count = count + n_items;

    if (new_count <= cap) {
        return da;
    }

    // if da->count > da_capacity then make capacity = count * growth factor
    while (new_count > cap) {
        cap *= GROWTH_FACTOR;
    }

    // alloc new array -> either init or realloc | Note this will free da if succesful
    fprintf(stderr, "%s:%d:(re)Allocation %zu %zu | %zu %zu\n", __FILE__, __LINE__, count, ocap, new_count, cap);
    void* tmp = realloc(da_header(da), item_size * cap + sizeof(da_header_t));
    if (tmp == NULL) {
        fprintf(stderr, "(re)Allocation of %p failed at %s:%d\n", tmp, __FILE__, __LINE__);
        return NULL;
    }

    tmp = (char*) tmp + sizeof(da_header_t);
    da_set_capacity(tmp, cap);
    return tmp;
}
void* daq_init(void* q, size_t item_size) {

    size_t capacity = DEFAULT_CAPACITY;
    fprintf(stderr, "%s:%d:Allocation\n", __FILE__, __LINE__);
    void* tmp = realloc(NULL, item_size * capacity + sizeof(daq_header_t));
    if (tmp == NULL) {
        fprintf(stderr, "(re)Allocation of %p failed at %s:%d\n", tmp, __FILE__, __LINE__);
        return NULL;
    }
    tmp = (char*) tmp + sizeof(daq_header_t); // use char* cast to move by bytes
    // da_header(tmp)->items    = 0;
    daq_header(tmp)->count    = 0;
    daq_header(tmp)->front    = 0;
    daq_header(tmp)->back     = 0;
    daq_header(tmp)->capacity = DEFAULT_CAPACITY;
    q                         = tmp;
    return q;
}

void* daq_accommodate(void* q, size_t item_size, size_t n_items) {

    // if da is null we need to make a new header
    size_t cap       = daq_capacity(q);
    size_t count     = daq_count(q);
    size_t new_count = count + n_items;

    // if da->count > da_capacity then make capacity = count * growth factor
    if (new_count >= cap) {
        while (new_count >= cap) {
            cap *= GROWTH_FACTOR;
        }
    }

    // alloc new array -> either init or realloc | Note this will free da if succesful
    fprintf(stderr, "%s:%d:(re)Allocation\n", __FILE__, __LINE__);
    void* tmp = realloc(daq_header(q), item_size * cap + sizeof(daq_header_t));
    if (tmp == NULL) {
        fprintf(stderr, "(re)Allocation of %p failed at %s:%d\n", tmp, __FILE__, __LINE__);
        return NULL;
    }
    tmp = (char*) tmp + sizeof(daq_header_t);
    daq_set_capacity(tmp, cap);
    return tmp;
}

#endif // DYNAMIC_ARRAY_IMPLEMENTATION
