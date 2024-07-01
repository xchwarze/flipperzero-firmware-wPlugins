/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of http://www.FreeRTOS.org for more information.
 */

#include "memmgr_heap.h"
#include "check.h"
#include <stdlib.h>
#include <stdio.h>
#include <stm32wbxx.h>
#include <stm32wb55_linker.h>
#include <core/log.h>
#include <core/common_defines.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include <FreeRTOS.h>
#include <task.h>

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#ifdef HEAP_PRINT_DEBUG
#error This feature is broken, logging transport must be replaced with RTT
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE ((size_t)(xHeapStructSize << 1))

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE ((size_t)8)

/* Heap start end symbols provided by linker */
uint8_t* ucHeap = (uint8_t*)&__heap_start__;

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK {
    struct A_BLOCK_LINK* pxNextFreeBlock; /*<< The next free block in the list. */
    size_t xBlockSize; /*<< The size of the free block. */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList(BlockLink_t* pxBlockToInsert);

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */
static void prvHeapInit(void);

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize = (sizeof(BlockLink_t) + ((size_t)(portBYTE_ALIGNMENT - 1))) &
                                      ~((size_t)portBYTE_ALIGNMENT_MASK);

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

/* Furi heap extension */
#include <m-dict.h>

extern const void __heap_start__;
extern const void __heap_end__;

static tlsf_t tlsf = NULL;
static size_t heap_used = 0;
static size_t heap_max_used = 0;

// Allocation tracking types
DICT_DEF2(MemmgrHeapAllocDict, uint32_t, uint32_t) //-V1048

DICT_DEF2( //-V1048
    MemmgrHeapThreadDict,
    uint32_t,
    M_DEFAULT_OPLIST,
    MemmgrHeapAllocDict_t,
    DICT_OPLIST(MemmgrHeapAllocDict))

// Thread allocation tracing storage
static MemmgrHeapThreadDict_t memmgr_heap_thread_dict = {0};
static volatile uint32_t memmgr_heap_thread_trace_depth = 0;

static inline void memmgr_lock(void) {
    vTaskSuspendAll();
}

static inline void memmgr_unlock(void) {
    xTaskResumeAll();
}

static inline size_t memmgr_get_heap_size(void) {
    return (size_t)&__heap_end__ - (size_t)&__heap_start__;
}

// Initialize tracing storage
static void memmgr_heap_init(void) {
    MemmgrHeapThreadDict_init(memmgr_heap_thread_dict);
}

__attribute__((constructor)) static void memmgr_init(void) {
    size_t pool_size = (size_t)&__heap_end__ - (size_t)&__heap_start__;
    tlsf = tlsf_create_with_pool((void*)&__heap_start__, pool_size, pool_size);
    memmgr_heap_init();
}

void memmgr_heap_enable_thread_trace(FuriThreadId thread_id) {
    memmgr_lock();
    {
        memmgr_heap_thread_trace_depth++;
        furi_check(MemmgrHeapThreadDict_get(memmgr_heap_thread_dict, (uint32_t)thread_id) == NULL);
        MemmgrHeapAllocDict_t alloc_dict;
        MemmgrHeapAllocDict_init(alloc_dict);
        MemmgrHeapThreadDict_set_at(memmgr_heap_thread_dict, (uint32_t)thread_id, alloc_dict);
        MemmgrHeapAllocDict_clear(alloc_dict);
        memmgr_heap_thread_trace_depth--;
    }
    memmgr_unlock();
}

void memmgr_heap_disable_thread_trace(FuriThreadId thread_id) {
    memmgr_lock();
    {
        memmgr_heap_thread_trace_depth++;
        furi_check(MemmgrHeapThreadDict_erase(memmgr_heap_thread_dict, (uint32_t)thread_id));
        memmgr_heap_thread_trace_depth--;
    }
    memmgr_unlock();
}

static inline void memmgr_heap_trace_malloc(void* pointer, size_t size) {
    FuriThreadId thread_id = furi_thread_get_current_id();
    if(thread_id && memmgr_heap_thread_trace_depth == 0) {
        memmgr_heap_thread_trace_depth++;
        MemmgrHeapAllocDict_t* alloc_dict =
            MemmgrHeapThreadDict_get(memmgr_heap_thread_dict, (uint32_t)thread_id);
        if(alloc_dict) {
            MemmgrHeapAllocDict_set_at(*alloc_dict, (uint32_t)pointer, (uint32_t)size);
        }
        memmgr_heap_thread_trace_depth--;
    }
}

static inline void memmgr_heap_trace_free(void* pointer) {
    FuriThreadId thread_id = furi_thread_get_current_id();
    if(thread_id && memmgr_heap_thread_trace_depth == 0) {
        memmgr_heap_thread_trace_depth++;
        MemmgrHeapAllocDict_t* alloc_dict =
            MemmgrHeapThreadDict_get(memmgr_heap_thread_dict, (uint32_t)thread_id);
        if(alloc_dict) {
            // In some cases thread may want to release memory that was not allocated by it
            const bool res = MemmgrHeapAllocDict_erase(*alloc_dict, (uint32_t)pointer);
            UNUSED(res);
        }
        memmgr_heap_thread_trace_depth--;
    }
}

size_t memmgr_heap_get_thread_memory(FuriThreadId thread_id) {
    size_t leftovers = MEMMGR_HEAP_UNKNOWN;
    memmgr_lock();
    {
        memmgr_heap_thread_trace_depth++;
        MemmgrHeapAllocDict_t* alloc_dict =
            MemmgrHeapThreadDict_get(memmgr_heap_thread_dict, (uint32_t)thread_id);
        if(alloc_dict) {
            leftovers = 0;
            MemmgrHeapAllocDict_it_t alloc_dict_it;
            for(MemmgrHeapAllocDict_it(alloc_dict_it, *alloc_dict);
                !MemmgrHeapAllocDict_end_p(alloc_dict_it);
                MemmgrHeapAllocDict_next(alloc_dict_it)) {
                MemmgrHeapAllocDict_itref_t* data = MemmgrHeapAllocDict_ref(alloc_dict_it);
                if(data->key != 0) {
                    block_header_t* block = block_from_ptr((uint8_t*)data->key);
                    if(!block_is_free(block)) {
                        leftovers += data->value;
                    }
                }
            }
        }
        memmgr_heap_thread_trace_depth--;
    }
    memmgr_unlock();
    return leftovers;
}

static bool tlsf_walker_max_free(void* ptr, size_t size, int used, void* user) {
    UNUSED(ptr);

    bool free = !used;
    size_t* max_free_block_size = (size_t*)user;
    if(free && size > *max_free_block_size) {
        *max_free_block_size = size;
    }

    return true;
}

size_t memmgr_heap_get_max_free_block(void) {
    size_t max_free_block_size = 0;

    memmgr_lock();

    pool_t pool = tlsf_get_pool(tlsf);
    tlsf_walk_pool(pool, tlsf_walker_max_free, &max_free_block_size);

    memmgr_unlock();

    return max_free_block_size;
}

typedef struct {
    BlockWalker walker;
    void* context;
} BlockWalkerWrapper;

static bool tlsf_walker_wrapper(void* ptr, size_t size, int used, void* user) {
    BlockWalkerWrapper* wrapper = (BlockWalkerWrapper*)user;
    return wrapper->walker(ptr, size, used, wrapper->context);
}

void memmgr_heap_walk_blocks(BlockWalker walker, void* context) {
    memmgr_lock();

    BlockWalkerWrapper wrapper = {walker, context};
    pool_t pool = tlsf_get_pool(tlsf);
    tlsf_walk_pool(pool, tlsf_walker_wrapper, &wrapper);

    memmgr_unlock();
}

void* pvPortMalloc(size_t xSize) {
    // memory management in ISR is not allowed
    if(FURI_IS_IRQ_MODE()) {
        furi_crash("memmgt in ISR");
    }

    memmgr_lock();

    // allocate block
    void* data = tlsf_malloc(tlsf, xSize);
    if(data == NULL) {
        if(xSize == 0) {
            furi_crash("malloc(0)");
        } else {
            furi_crash("out of memory");
        }
    }

    // update heap usage
    heap_used += tlsf_block_size(data);
    heap_used += tlsf_alloc_overhead();
    if(heap_used > heap_max_used) {
        heap_max_used = heap_used;
    }

    // trace allocation
    memmgr_heap_trace_malloc(data, xSize);

    memmgr_unlock();

    // clear block content
    memset(data, 0, xSize);

    return data;
}

void vPortFree(void* pv) {
    // memory management in ISR is not allowed
    if(FURI_IS_IRQ_MODE()) {
        furi_crash("memmgt in ISR");
    }

    // ignore NULL pointer
    if(pv != NULL) {
        memmgr_lock();

        // get block size
        size_t block_size = tlsf_block_size(pv);

        // clear block content
        memset(pv, 0, block_size);

        // update heap usage
        heap_used -= block_size;
        heap_used -= tlsf_alloc_overhead();

        // free
        tlsf_free(tlsf, pv);

        // trace free
        memmgr_heap_trace_free(pv);

        memmgr_unlock();
    }
}

extern void* pvPortAllocAligned(size_t xSize, size_t xAlignment) {
    // memory management in ISR is not allowed
    if(FURI_IS_IRQ_MODE()) {
        furi_crash("memmgt in ISR");
    }

    // alignment must be power of 2
    if((xAlignment & (xAlignment - 1)) != 0) {
        furi_crash("invalid alignment");
    }

    memmgr_lock();

    // allocate block
    void* data = tlsf_memalign(tlsf, xAlignment, xSize);
    if(data == NULL) {
        if(xSize == 0) {
            furi_crash("malloc_aligned(0)");
        } else {
            furi_crash("out of memory");
        }
    }

    // update heap usage
    heap_used += tlsf_block_size(data);
    heap_used += tlsf_alloc_overhead();
    if(heap_used > heap_max_used) {
        heap_max_used = heap_used;
    }

    // trace allocation
    memmgr_heap_trace_malloc(data, xSize);

    memmgr_unlock();

    // clear block content
    memset(data, 0, xSize);

    return data;
}

extern void* pvPortRealloc(void* pv, size_t xSize) {
    // realloc(ptr, 0) is equivalent to free(ptr)
    if(xSize == 0) {
        vPortFree(pv);
        return NULL;
    }

    // realloc(NULL, size) is equivalent to malloc(size)
    if(pv == NULL) {
        return pvPortMalloc(xSize);
    }

    /* realloc things */

    // memory management in ISR is not allowed
    if(FURI_IS_IRQ_MODE()) {
        furi_crash("memmgt in ISR");
    }

    memmgr_lock();

    // trace old block as free
    size_t old_size = tlsf_block_size(pv);

    // trace free
    memmgr_heap_trace_free(pv);

    // reallocate block
    void* data = tlsf_realloc(tlsf, pv, xSize);
    if(data == NULL) {
        furi_crash("out of memory");
    }

    // update heap usage
    heap_used -= old_size;
    heap_used += tlsf_block_size(data);
    if(heap_used > heap_max_used) {
        heap_max_used = heap_used;
    }

    // trace allocation
    memmgr_heap_trace_malloc(data, xSize);

    memmgr_unlock();

    // clear remain block content, if the new size is bigger
    // can't guarantee that all data will be zeroed, cos tlsf_block_size is not always the same as xSize
    if(xSize > old_size) {
        memset((uint8_t*)data + old_size, 0, xSize - old_size);
    }

    return data;
}

size_t xPortGetFreeHeapSize(void) {
    return memmgr_get_heap_size() - heap_used - tlsf_size(tlsf);
}

size_t xPortGetTotalHeapSize(void) {
    return memmgr_get_heap_size();
}

size_t xPortGetMinimumEverFreeHeapSize(void) {
    return memmgr_get_heap_size() - heap_max_used - tlsf_size(tlsf);
}