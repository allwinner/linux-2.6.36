/** @file payload_manager.c
 *
 * Payload Manager source file
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
 *   Please see description in payload_manager.h
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/payload_manager/payload_manager.c#4 $
 *
 ****************************************************************************/

/** @{
 * @ingroup payload_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "payload_manager/payload_manager.h"
#include "csr_cstl/csr_wifi_list.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

/* MACROS *******************************************************************/

/* Manage the Reference count as part of the Handle to save space */
#define PLDHANDLEBITS 12
#define PLDREFBITS     4
#define PLDREFERENCEMASK ((CsrUint16)0xF000)

#define PLDGET_HANDLE(handle) ((PldHdl)(handle & ~PLDREFERENCEMASK))
#define PLDGET_REFERENCE(handle) ((handle & PLDREFERENCEMASK) >> PLDHANDLEBITS)
#define PLDADD_REFERENCE(handle) (((PLDGET_REFERENCE(handle) + 1) << PLDHANDLEBITS) | PLDGET_HANDLE(handle))
#define PLDDEL_REFERENCE(handle) (((PLDGET_REFERENCE(handle) - 1) << PLDHANDLEBITS) | PLDGET_HANDLE(handle))

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/
struct PldContext {
#ifdef FSM_MUTEX_ENABLE
    CsrMutexHandle         payloadLock;
#endif

    csr_list               payloads;
    CsrUint16              payloadCount;
    CsrUint16              payloadMax;

    CsrUint16              lastHandle;
    CsrUint16              nextUsedHandle;
#ifdef SME_TRACE_ENABLE
    /** stats */
    PldStats               stats;
#endif
};


#ifdef SME_TRACE_ENABLE

#define PAYLOAD_TRACE_RECORD_MAX 8
typedef enum {
    PayloadTraceRecordTypeCreate,
    PayloadTraceRecordTypeAddRef,
    PayloadTraceRecordTypeDelete
} PayloadTraceRecordType;

typedef struct
{
    PayloadTraceRecordType type;
    /** Pointer to the file that first requested this payload storage */
    const char      * file;
    /** Pointer to the line that first requested this payload storage */
    int               line;
    /** Extra debug data associated with the payload allocation */
    CsrUint16            extra;
} PayloadTraceRecord;

typedef struct
{
    CsrUint16 recordsAdded;
    CsrUint16 recordsCount;
    PayloadTraceRecord records[PAYLOAD_TRACE_RECORD_MAX];
} PayloadTraceRecords;

void pld_trace_record_add(PayloadTraceRecords* records, PayloadTraceRecordType type, const char* file, int line, CsrUint16 extra)
{
    records->recordsAdded++;
    if (records->recordsCount == PAYLOAD_TRACE_RECORD_MAX)
    {
        return;
    }
    records->records[records->recordsCount].type  = type;
    records->records[records->recordsCount].file  = file;
    records->records[records->recordsCount].line  = line;
    records->records[records->recordsCount].extra = extra;
    records->recordsCount++;
}
#else
#define pld_trace_record_add(records, type, file, line, extra)
#endif

/**
* @brief
*  A payload tracker structure
*
* @par Description
*  This structure ties a block of memory, along with its size, to a handle.
*  Users of the payload manager need only keep track of the handle in order
*  to access the data.
*/
typedef struct
{
    /** Linked list overhead */
    csr_list_node     node;

    /** A 16-bit handle that contains payload-control information */
    CsrUint16            handle;
    /** The size (in bytes) of the data associated with the handle */
    CsrUint16            size;
#ifdef SME_TRACE_ENABLE
    PayloadTraceRecords records;
#endif

    /** Payload data
     * This node will be malloced to the correct size to accommodate the buffer */
    CsrUint8            data[4];
} PayloadEntry;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/
#ifdef SME_TRACE_ENABLE
    static void pld_dump_owners(PldContext* context);
#endif

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static PayloadEntry* payld_get_entry(PldContext* context, PldHdl handle)
{
    PayloadEntry* entry;
    require(TR_PAYLOAD_MGR, handle != 0);

    sme_trace_entry((TR_PAYLOAD_MGR, "payld_get_entry() handle 0x%04x", handle));

    for (entry = csr_list_gethead_t(PayloadEntry *, &context->payloads); entry != NULL;
         entry = csr_list_getnext_t(PayloadEntry *, &context->payloads, &entry->node))
    {
        if(PLDGET_HANDLE(entry->handle) == PLDGET_HANDLE(handle))
        {
            return entry;
        }
    }

    return NULL;
}

static CsrUint16 getNextHandle(PldContext* context)
{
    PayloadEntry* entry;

    /* Check for handle wrap around */
    if ((context->lastHandle + 1) & PLDREFERENCEMASK)
    {
        context->lastHandle = 0;
        context->nextUsedHandle = 1;
    }

    context->lastHandle++;

    /*
     * Generally speaking, we expect the 'new' handle to differ from
     * the next handle that's in use
     */
    if (context->lastHandle != context->nextUsedHandle)
    {
        return (CsrUint16)(context->lastHandle | (1 << PLDHANDLEBITS));
    }

    /*
     * Our 'new' handle is already in use, we must now search out list of
     * handles to find the first (lowest numbered) handle that's free
     *
     * NB: Because of the pre-increment in the while loop, we MUST firstly
     *     check that we will not test for a handle with value 0.
     */
    if ((context->lastHandle + 1) & PLDREFERENCEMASK)
    {
        context->lastHandle = 0;    /* 'advance' to zero, pre-increment will push it to 1 */
    }

    /*
     * Find the first unused handle
     */
    while(payld_get_entry(context, ++context->lastHandle))
    {
    }

    /*
     * Find the next free handle BEYOND the value of lastHandle
     */
    context->nextUsedHandle = PLDREFERENCEMASK;
    entry = csr_list_gethead_t(PayloadEntry *, &context->payloads);
    while (entry)
    {
        if (PLDGET_HANDLE(entry->handle) < context->nextUsedHandle &&
            PLDGET_HANDLE(entry->handle) > context->lastHandle)
        {
            context->nextUsedHandle = PLDGET_HANDLE(entry->handle);
        }
        entry = csr_list_getnext_t(PayloadEntry *, &context->payloads, &entry->node);
    }

    return (CsrUint16)(context->lastHandle | (1 << PLDHANDLEBITS));
}

static PayloadEntry* payld_allocate_entry(PldContext* context, CsrUint16 length)
{
    PayloadEntry* entry;
    require(TR_PAYLOAD_MGR, length != 0);

    sme_trace_entry((TR_PAYLOAD_MGR, "payld_allocate_entry() bytes %d payloadMax %d payloadCount %d", length, context->payloadMax, context->payloadCount));

    /* Check for Payload limit */
    if (context->payloadCount + 1 > context->payloadMax)
    {
#ifdef SME_TRACE_ENABLE
        pld_dump_owners(context);
#endif
        require(TR_PAYLOAD_MGR, context->payloadCount + 1 <= context->payloadMax);
        return NULL;
    }

    /* Create a list entry sized to the PayloadEntry + the buffer */
    entry = (PayloadEntry*)CsrPmalloc(sizeof(PayloadEntry) + length - 4 /* Size of the data array in PayloadEntry */); /*lint !e433 */
    entry->handle = getNextHandle(context);
    entry->size = length;

    verify(TR_PAYLOAD_MGR, payld_get_entry(context, PLDGET_HANDLE(entry->handle)) == NULL); /*lint !e666 */

    /* Insert at Head as the last 1 created is the most likely 1 to be accessed */
    csr_list_insert_head(&context->payloads, list_owns_value, &entry->node, entry);

    context->payloadCount++;

#ifdef SME_TRACE_ENABLE
    entry->records.recordsAdded = 0;
    entry->records.recordsCount = 0;

    /* Update payload manager statistics, output a diagnostic and return. */
    context->stats.totalAllocs++;
    context->stats.activeMemory += length;
    context->stats.currentPayloads++;
    if (context->stats.currentPayloads > context->stats.highPayloads)
    {
        context->stats.highPayloads = context->stats.currentPayloads;
    }
    if (context->stats.activeMemory > context->stats.highActiveMemory)
    {
        context->stats.highActiveMemory = context->stats.activeMemory;
    }
#endif

    return entry;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/* --------------------------------------------------------------------------
 *
 * Initialises the payload manager subsystem.
 */
PldContext* pld_init(CsrUint16 tableSize, CsrUint16 maxPayloads)
{
    PldContext* context = (PldContext*)CsrPmalloc(sizeof(PldContext));

    sme_trace_entry((TR_PAYLOAD_MGR, "pld_init() maxPayloads %d", maxPayloads));

    /* Initialise the context */
#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexCreate(&context->payloadLock);
#endif
    csr_list_init(&context->payloads);
    context->payloadMax = maxPayloads;
    context->payloadCount = 0;
    context->lastHandle = 0; /* handle 0 should never be used */
    context->nextUsedHandle = PLDREFERENCEMASK;

#ifdef SME_TRACE_ENABLE
    CsrMemSet(&context->stats, 0, sizeof(context->stats));
    context->stats.maxPayloads = maxPayloads;
#endif

    return context;
}

/* --------------------------------------------------------------------------
 *
 * Releases all resources allocated by pld_init().
 */
void pld_shutdown(PldContext* context)
{
#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->payloadLock);
#endif

    sme_trace_entry((TR_PAYLOAD_MGR, "pld_shutdown()"));

#ifdef SME_TRACE_ENABLE
    /* Display payload usage statistics */
    pld_dump_stats(context);

    /* Display the owners of any remaining payloads (suggesting that other
     * parts of the SME have not cleaned up properly) */
    pld_dump_owners(context);
#endif

    /* This will delete the buffers as well */
    csr_list_clear(&context->payloads);

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->payloadLock);
#endif

#ifdef FSM_MUTEX_ENABLE
    CsrMutexDestroy(context->payloadLock);
#endif
    CsrPfree(context);
}

/* --------------------------------------------------------------------------
 *
 * Retrieves statistics on the operation of the payload manager subsystem
 */
void pld_get_stats(PldContext* context, PldStats *userStats)
{
    require(TR_PAYLOAD_MGR, userStats != NULL);
#ifdef SME_TRACE_ENABLE
#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->payloadLock);
#endif
    CsrMemCpy(userStats, (void *)&context->stats, sizeof(context->stats));
#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->payloadLock);
#endif
#else
    CsrMemSet(userStats, 0x00, sizeof(PldStats));
#endif
}

/* --------------------------------------------------------------------------
 *
 * Creates an (uninitialised) payload that the caller may populate later
 */
#ifdef SME_TRACE_ENABLE
void payld_create(const char *callFile, int callLine, PldContext* context, CsrUint16 length, void **buf, PldHdl *hdl)
#else
void payld_create(PldContext* context, CsrUint16 length, void **buf, PldHdl *hdl)
#endif
{
    PayloadEntry* entry;

    sme_trace_entry((TR_PAYLOAD_MGR, "payld_create() bytes %d (%s:%d)", length, callFile, callLine));
    require(TR_PAYLOAD_MGR, buf != NULL);
    require(TR_PAYLOAD_MGR, hdl != NULL);

    *buf = NULL;
    *hdl = 0;

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->payloadLock);
#endif
    entry = payld_allocate_entry(context, length);
    if (entry)
    {
        sme_trace_info((TR_PAYLOAD_MGR, "payld_create() handle 0x%04x bytes %d (%s:%d)", PLDGET_HANDLE(entry->handle), length, callFile, callLine));

        pld_trace_record_add(&entry->records, PayloadTraceRecordTypeCreate, callFile, callLine, 0);
        *buf = entry->data;
        *hdl = entry->handle;
    }
    else
    {
        sme_trace_crit((TR_PAYLOAD_MGR, "payld_create() Max Payload Exceeded (%s:%d)", callFile, callLine));
    }
    require(TR_PAYLOAD_MGR, entry != NULL);
#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->payloadLock);
#endif
}

/* --------------------------------------------------------------------------
 *
 * Registers a block of data with the payload management system
 */
#ifdef SME_TRACE_ENABLE
void payld_store(const char *callFile, int callLine, CsrUint16 extra, PldContext* context, void *buf,  CsrUint16 length, PldHdl *hdl)
#else
void payld_store(PldContext* context, void *buf, CsrUint16 length, PldHdl *hdl)
#endif
{
    PayloadEntry* entry;

    require(TR_PAYLOAD_MGR, buf != NULL);
    require(TR_PAYLOAD_MGR, hdl != NULL);

    *hdl = 0;

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->payloadLock);
#endif

    sme_trace_entry((TR_PAYLOAD_MGR, "payld_store() bytes %d (%s:%d)", length, callFile, callLine));

    entry = payld_allocate_entry(context, length);
    if (entry)
    {
        sme_trace_info((TR_PAYLOAD_MGR, "payld_store() handle 0x%04x bytes %d (%s:%d)", PLDGET_HANDLE(entry->handle), length, callFile, callLine));

        pld_trace_record_add(&entry->records, PayloadTraceRecordTypeCreate, callFile, callLine, extra);
        CsrMemCpy(entry->data, buf, length);
        *hdl = entry->handle;
    }
    else
    {
        sme_trace_crit((TR_PAYLOAD_MGR, "payld_store() Max Payload Exceeded (%s:%d)", callFile, callLine));
    }
    require(TR_PAYLOAD_MGR, entry != NULL);
#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->payloadLock);
#endif
}

/* --------------------------------------------------------------------------
 *
 * Obtains the pointer to the payload (and its length) associated with a handle
 */
#ifdef SME_TRACE_ENABLE
void payld_access(const char *callFile, int callLine, PldContext* context, PldHdl hdl, void **buf, CsrUint16 *length)
#else
void payld_access(PldContext* context, PldHdl hdl, void **buf, CsrUint16 *length)
#endif
{
    PayloadEntry* entry;
    *length = 0;
    *buf = NULL;

    sme_trace_entry((TR_PAYLOAD_MGR, "pld_access() handle 0x%04x (%s:%d)", hdl, callFile, callLine));

    require(TR_PAYLOAD_MGR, buf != NULL);
    require(TR_PAYLOAD_MGR, length != NULL);
    require(TR_PAYLOAD_MGR, hdl != 0);

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->payloadLock);
#endif

    entry = payld_get_entry(context, hdl);

    if (entry)
    {
        *buf = entry->data;
        *length = entry->size;
        sme_trace_info((TR_PAYLOAD_MGR, "payld_access() handle 0x%04x size %d (%s:%d)", PLDGET_HANDLE(hdl), *length, callFile, callLine));
    }
    else
    {
        sme_trace_crit((TR_PAYLOAD_MGR, "payld_access(0x%04x) Invalid Handle (%s:%d)", PLDGET_HANDLE(hdl), callFile, callLine));
    }
    require(TR_PAYLOAD_MGR, entry != NULL);

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->payloadLock);
#endif
}

/* --------------------------------------------------------------------------
 *
 * Increments the usage count associated with a particular payload
 */
#ifdef SME_TRACE_ENABLE
void payld_add_ref(const char *callFile, int callLine, PldContext* context, PldHdl hdl)
#else
void payld_add_ref(PldContext* context, PldHdl hdl)
#endif
{
    PayloadEntry* entry;
    require(TR_PAYLOAD_MGR, hdl != 0);

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->payloadLock);
#endif

    sme_trace_entry((TR_PAYLOAD_MGR, "pld_add_ref() handle 0x%04x (%s:%d)", PLDGET_HANDLE(hdl), callFile, callLine));
    entry = payld_get_entry(context, hdl);

    if (entry)
    {
        entry->handle = PLDADD_REFERENCE(entry->handle);
        sme_trace_entry((TR_PAYLOAD_MGR, "pld_add_ref() handle 0x%04x ref %d (%s:%d)", PLDGET_HANDLE(entry->handle), PLDGET_REFERENCE(entry->handle), callFile, callLine));
        pld_trace_record_add(&entry->records, PayloadTraceRecordTypeAddRef, callFile, callLine, 0);
    }
    else
    {
        sme_trace_crit((TR_PAYLOAD_MGR, "pld_add_ref(0x%04x) Invalid Handle (%s:%d)", PLDGET_HANDLE(hdl), callFile, callLine));
    }
    require(TR_PAYLOAD_MGR, entry != NULL);

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->payloadLock);
#endif
}

/* --------------------------------------------------------------------------
 *
 * Deletes a payload recovering all resources used.
 */
#ifdef SME_TRACE_ENABLE
void payld_release(const char *callFile, int callLine, PldContext* context, PldHdl hdl)
#else
void payld_release(PldContext* context, PldHdl hdl)
#endif
{
    PayloadEntry* entry;

    if (hdl == 0)
    {
        /* Special case for handle of 0.
         * Allow it and just return. */
        return;
    }

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->payloadLock);
#endif

    sme_trace_entry((TR_PAYLOAD_MGR, "pld_release() handle 0x%04x (%s:%d)", PLDGET_HANDLE(hdl), callFile, callLine));

    entry = payld_get_entry(context, hdl);

    if (entry)
    {
        entry->handle = PLDDEL_REFERENCE(entry->handle);
        sme_trace_info((TR_PAYLOAD_MGR, "pld_release() handle 0x%04x ref %d , (%s:%d)", PLDGET_HANDLE(entry->handle), PLDGET_REFERENCE(entry->handle), callFile, callLine));
        pld_trace_record_add(&entry->records, PayloadTraceRecordTypeDelete, callFile, callLine, 0);
        if (!PLDGET_REFERENCE(entry->handle))
        {
#ifdef SME_TRACE_ENABLE
            /* Update payload manager statistics, output a diagnostic and return. */
            context->stats.activeMemory -= entry->size;
            context->stats.currentPayloads--;
#endif

            context->payloadCount--;
            (void)csr_list_remove(&context->payloads, &entry->node);
        }
    }
    else
    {
        sme_trace_crit((TR_PAYLOAD_MGR, "pld_release(0x%04x) Invalid Handle (%s:%d)", PLDGET_HANDLE(hdl), callFile, callLine));
    }
    require(TR_PAYLOAD_MGR, entry != NULL);

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->payloadLock);
#endif

}

#ifdef SME_TRACE_ENABLE
static void pld_dump_owners(PldContext* context)
{
    PayloadEntry* entry;

    if (context->payloadCount == 0)
    {
        return;
    }

    sme_trace_error((TR_PAYLOAD_MGR, "Payload Owners"));

    for (entry = csr_list_gethead_t(PayloadEntry *, &context->payloads); entry != NULL;
         entry = csr_list_getnext_t(PayloadEntry *, &context->payloads, &entry->node))
    {
        CsrUint16 i;
        sme_trace_error((TR_PAYLOAD_MGR, "hdl 0x%04x ref %d, size %05d, file %s, line %d, extra 0x%.4X",
                        PLDGET_HANDLE(entry->handle), PLDGET_REFERENCE(entry->handle),
                        entry->size, entry->records.records[0].file, entry->records.records[0].line, entry->records.records[0].extra));

        for (i = 1; i < entry->records.recordsCount; i++)
        {
            if (entry->records.records[i].type == PayloadTraceRecordTypeAddRef)
            {
                sme_trace_error((TR_PAYLOAD_MGR, "                   %s, file %s, line %d",
                    "PldAddRef", entry->records.records[i].file, entry->records.records[i].line));
            }
            else
            {
                sme_trace_error((TR_PAYLOAD_MGR, "                   %s, file %s, line %d",
                    "PldDelete", entry->records.records[i].file, entry->records.records[i].line));
            }


            if (entry->records.recordsAdded > PAYLOAD_TRACE_RECORD_MAX)
            {
                sme_trace_error((TR_PAYLOAD_MGR, "                   Max Records Exceeded"));
            }
        }

    }
}

/* --------------------------------------------------------------------------
 *
 * Dumps a summary of the subsystem stats via sme_trace_info
 */
void pld_dump_stats(PldContext* context)
{
    sme_trace_error((TR_PAYLOAD_MGR, "      Payload Manager Statistics Dump:"));
    sme_trace_error((TR_PAYLOAD_MGR, "      ------- ------- ---------- ----:"));
    sme_trace_error((TR_PAYLOAD_MGR, "           Maximum    Current    Free"));
    sme_trace_error((TR_PAYLOAD_MGR, "Payloads:    %3d       %3d        %3d",
                    context->stats.maxPayloads, context->stats.currentPayloads,
                    context->stats.maxPayloads - context->stats.currentPayloads));
    sme_trace_error((TR_PAYLOAD_MGR, "Current Active Memory %d kB (%d bytes)",
                    context->stats.activeMemory >> 10, context->stats.activeMemory));
    sme_trace_error((TR_PAYLOAD_MGR, "Payload High Tide: %d", context->stats.highPayloads));
    sme_trace_error((TR_PAYLOAD_MGR, "Memory  High Tide: %d", context->stats.highActiveMemory));
}

void pld_trace(PldContext* context, sme_trace_module_id id, sme_trace_level level, const char* const message, PldHdl hdl)
{
    PayloadEntry* entry;

    if (hdl == 0)
    {
        /* Special case for handle of 0.
         * Allow it and just return. */
        return;
    }

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->payloadLock);
#endif

    entry = payld_get_entry(context, hdl);

    if (entry)
    {
        sme_trace_hex((TR_SCAN, TR_LVL_INFO, message, entry->data, entry->size));
    }
    else
    {
        sme_trace_hex((TR_SCAN, TR_LVL_INFO, message, NULL, 0));
    }

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->payloadLock);
#endif
}


#endif /* SME_TRACE_ENABLE */

#ifdef PLDTEST
void pldTest(PldContext* context)
{
    int i,j;
    void* buf;
    PldHdl firsthdl[10];
    PldHdl secondhdl[10];
    PldHdl thirdhdl[10];

    PldHdl wraphdl[50];


    /* Create handles 1 to 10 */
    sme_trace_error((TR_PAYLOAD_MGR, "pldTest First 10"));
    for (i = 0; i < 10; ++i)
    {
        pld_create(context, 10, &buf, &firsthdl[i]);
    }

    sme_trace_error((TR_PAYLOAD_MGR, "pldTest Second 10"));
    /* Create handles 11 to 20 */
    for (i = 0; i < 10; ++i)
    {
        pld_create(context, 10, &buf, &secondhdl[i]);
    }

    sme_trace_error((TR_PAYLOAD_MGR, "pldTest Third 10"));
    /* Create handles 21 to 30 */
    for (i = 0; i < 10; ++i)
    {
        pld_create(context, 10, &buf, &thirdhdl[i]);
    }

    sme_trace_error((TR_PAYLOAD_MGR, "pldTest Second 10 Delete"));
    /* delete handles 11 to 20 */
    for (i = 0; i < 10; ++i)
    {
        pld_release(context, secondhdl[i]);
    }

    sme_trace_error((TR_PAYLOAD_MGR, "pldTest Wrap"));
    /* Allocate a wrap around + some */
    /* This should allocate and around to handle 40 missing the first and third blocks */
    for (i = 0; i < ((0xFFF*2) + 15); ++i)
    {
        pld_create(context, 10, &buf, &wraphdl[i % 50]);
        if ((i+1) % 50 == 0)
        {
            for (j = 0; j < 50; ++j)
            {
                pld_add_ref(context, wraphdl[j]);
                pld_release(context, wraphdl[j]);
                pld_release(context, wraphdl[j]);
            }
        }
    }
}
#endif /* PLDTEST */

/** @}
 */

