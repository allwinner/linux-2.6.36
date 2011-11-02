/** @file memprofile.c
 *
 * Linux hooks for profiling mem use
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   Linux hooks for profiling mem use
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_synergy_framework/linux/memprofile.c#1 $
 *
 ****************************************************************************/

/** @{
 * @ingroup abstractions
 */



/* STANDARD INCLUDES ********************************************************/
#include <stdio.h>
#include <pthread.h>
#include <malloc.h>

#ifdef OSA_MEMORY_PROFILE

/* PROJECT INCLUDES *********************************************************/

void __cyg_profile_func_enter (void *, void *) __attribute__((no_instrument_function));
void __cyg_profile_func_exit  (void *, void *) __attribute__((no_instrument_function));

typedef struct threaddata_s {
    pthread_t id;
    int depth;
    int maxDepth;
    void* funct;
    void* stackBase;
    int stackSize;
    int maxStackSize;
} threaddata_s;

#define MAXTHREADS 10

static int numthreads = 0;
static threaddata_s threads[MAXTHREADS] = {{-1, -1, -1, NULL, NULL, -1, -1},
                                           {-1, -1, -1, NULL, NULL, -1, -1},
                                           {-1, -1, -1, NULL, NULL, -1, -1},
                                           {-1, -1, -1, NULL, NULL, -1, -1},
                                           {-1, -1, -1, NULL, NULL, -1, -1},
                                           {-1, -1, -1, NULL, NULL, -1, -1},
                                           {-1, -1, -1, NULL, NULL, -1, -1},
                                           {-1, -1, -1, NULL, NULL, -1, -1},
                                           {-1, -1, -1, NULL, NULL, -1, -1},
                                           {-1, -1, -1, NULL, NULL, -1, -1}};

static pthread_mutex_t mallocMutex;


static unsigned int currentMallocBytes = 0;
static unsigned int currentMallocCount = 0;

static unsigned int maxMallocBytes = 0;
static unsigned int maxMallocBytesCount = 0;

static unsigned int maxMallocCount = 0;
static unsigned int maxMallocCountBytes = 0;


/**
 * @brief
 *      This function is called on function entry when -finstrument-functions is
 *      used.

 * @par Description:
 *      This function is used to report the stack use data used.
 *      Special :: Uses Printf so that this data is reported
 *      even in minimum config
 *
 * @return
 *      void
 */
void stack_profile_report()
{
    int i;
    printf("\n\n");
    printf("-----------------------------------------------------------------------------\n");
    printf("Heap & Stack Use Report\n");
    printf("-----------------------------------------------------------------------------\n");
    printf("Peak Heap Use\n");
    printf("    Bytes at Peak   = %u\n", maxMallocBytes);
    printf("    Count at Peak   = %u\n", maxMallocBytesCount);
    printf("    Heap Overhead  ~= %u (16 bytes per malloc) This will vary for different systems!\n\n", maxMallocBytesCount * 16);

    printf("Peak Heap Count\n");
    printf("    Count at Peak   = %u\n", maxMallocCount);
    printf("    Bytes at Peak   = %u\n", maxMallocCountBytes);
    printf("    Heap Overhead  ~= %u (16 bytes per malloc) This will vary for different systems!\n\n", maxMallocCount * 16);

    printf("Peak Stack Use Per Thread\n");
    for (i = 0; i < MAXTHREADS; i++)
    {
        if (threads[i].id == -1) break;
        printf("    Thread %d of %d : ThreadId(0x%.8X) Stack Base(%p) Max Call Depth(%.3d) MaxBytes(%d)\n",
                    i+1,
                    numthreads,
                    (int)threads[i].id,
                    threads[i].stackBase,
                    threads[i].maxDepth,
                    threads[i].maxStackSize);
    }
    printf("-----------------------------------------------------------------------------\n\n\n");
}

/**
 * @par Description:
 * This function is called on function entry when -finstrument-functions is
 * used.
 *
 * @param[in]     func
 * @param[in]     caller
 *
 * @return
 *      None
 *
 * @brief
 *      This function is called on function entry when -finstrument-functions is
 *      used.
 */
void __cyg_profile_func_enter (void *func,  void *caller)
{
    int stackvar = 0;
    int i;

    pthread_t base = pthread_self();
    (void)pthread_mutex_lock(&mallocMutex);
    for (i = 0; i < MAXTHREADS; i++)
    {
        if (threads[i].id == base || threads[i].id == -1)
        {
            if (threads[i].id == -1)
            {
                threads[i].stackBase = &stackvar; /*lint !e789 */
                threads[i].funct = func;
            }

            if (threads[i].id == -1)
                numthreads++;

            threads[i].id = base;

            threads[i].depth++;
            if (threads[i].depth > threads[i].maxDepth)
                threads[i].maxDepth = threads[i].depth;

            threads[i].stackSize = (int)threads[i].stackBase - (int)&stackvar;
            if (threads[i].stackSize > threads[i].maxStackSize)
            {
                threads[i].maxStackSize = threads[i].stackSize;
            }

            (void)pthread_mutex_unlock(&mallocMutex);
            return;
        }
    }
    (void)pthread_mutex_unlock(&mallocMutex);

}

/**
 * @par Description:
 * This function is called on function exit when -finstrument-functions is
 * used.
 *
 * @param[in]     func
 * @param[in]     caller
 *
 * @return
 *      None
 *
 * @brief
 *      This function is called on function exit when -finstrument-functions is
 *      used.
 */
void __cyg_profile_func_exit (void *func, void *caller)
{
    int stackvar = 0; /*lint -e550 */
    int i;
    pthread_t base = pthread_self();

    (void)pthread_mutex_lock(&mallocMutex);
    for (i = 0; i < MAXTHREADS; i++)
    {
        if (threads[i].id == base)
        {
            threads[i].depth--;
            threads[i].stackSize = (int)threads[i].stackBase - (int)&stackvar;
            if (threads[i].stackSize > threads[i].maxStackSize)
                threads[i].maxStackSize = threads[i].stackSize;

            (void)pthread_mutex_unlock(&mallocMutex);
            return;
        }
    }
    (void)pthread_mutex_unlock(&mallocMutex);

}

static void my_init_malloc_hook(void);

/* Variables to save original hooks. */
static void *(*old_malloc_hook)(size_t, const void *); /*lint !e955 */

/* Override initialising hook from the C library. */
void (*__malloc_initialize_hook) (void) = my_init_malloc_hook;

static void * my_malloc_hook (size_t size, const void *caller) {
    void *result;

    (void)pthread_mutex_lock(&mallocMutex);
    /* Restore all old hooks */
    __malloc_hook = old_malloc_hook;

    /* Call recursively */
    result = malloc (size + sizeof(size_t));
    *(size_t*)result = size;

    /* Save underlying hooks */
    old_malloc_hook = __malloc_hook;

    /* Restore our own hooks */
    __malloc_hook = my_malloc_hook;

    currentMallocBytes += size;
    currentMallocCount++;

    if (currentMallocBytes > maxMallocBytes)
    {
        maxMallocBytes = currentMallocBytes;
        maxMallocBytesCount = currentMallocCount;
    }

    if (currentMallocCount > maxMallocCount)
    {
        maxMallocCount = currentMallocCount;
        maxMallocCountBytes = currentMallocBytes;
    }
    (void)pthread_mutex_unlock(&mallocMutex);

    return (void*)((unsigned int)result+sizeof(size_t)); /*lint !e429 */
}


/* Variables to save original hooks. */
static void(*old_free_hook)(void *, const void *);

static void my_free_hook (void *ptr, const void *caller)
{
    size_t size;
    if (ptr == NULL)
    {
        return;
    }

    size = *(size_t*)((unsigned int)ptr - sizeof(size_t));

    (void)pthread_mutex_lock(&mallocMutex);
    /* Restore all old hooks */
    __free_hook = old_free_hook;

    /* Call recursively */
    free((void*)((unsigned int)ptr - sizeof(size_t)));

    /* Save underlying hooks */
    old_free_hook = __free_hook;

    /* Restore our own hooks */
    __free_hook = my_free_hook;

    currentMallocBytes -= size;
    currentMallocCount--;
    (void)pthread_mutex_unlock(&mallocMutex);
}

static void my_init_malloc_hook(void) {
    (void)pthread_mutex_init(&mallocMutex, NULL);
    old_malloc_hook = __malloc_hook;
    __malloc_hook = my_malloc_hook;

    old_free_hook = __free_hook;
    __free_hook = my_free_hook;
}

#endif /* OSA_MEMORY_PROFILE */

/** @}
 */
