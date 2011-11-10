/** @file payload_manager.h
 *
 * A generic buffer creation and retrieval API
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
 *   A simple buffer management API with reference/usage counts.
 *
 *   Two main uses are expected. Firstly an entity in possession of a block of
 *   initialised memory may wish to register this with the payload manager. The
 *   payload manager will make a copy of this data.
 *
 *   A second option is to request that the payload manager create a block of
 *   memory of a given size. The payload manager will do so, returning both a
 *   pointer to the start of the block and a handle that may be used to refer
 *   to the block later on.
 *
 *   Payload handles may be passed to other entities (e.g. SDL blocks etc).
 *   The implication is that ownership is handed to the receiving entity. When
 *   the receiving entity has finished using the payload it can delete it (this
 *   will recover any resources used by the payload).
 *   Should a transmitting entity wish to maintain ownership it must increment
 *   the reference count prior to sending on. When the receiver calls
 *   payload_release, the payload manager knows not to delete the payload,
 *   instead it just decrements the usage count.
 *
 *   Some primitive statistics gathering has been implemented - total stores,
 *   releases, the amount of active memory (bytes) and number of active handles
 *   are monitored.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/payload_manager/payload_manager.h#1 $
 *
 ****************************************************************************/

#ifndef PAYLOAD_MANAGER_H_
#define PAYLOAD_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "abstractions/osa.h"
#ifdef SME_TRACE_ENABLE
#include "sme_trace/sme_trace.h"
#endif
typedef struct PldContext PldContext;

/**
 * @brief
 *  Payload Manager handle type declaration.
 *
 * @par Description
 *  A payload handle. This comprises three fields of configurable length (see
 *  payload_manager_private_config.h): rotation, usage, and slot ID.
 *
 * @verbatim
 * 16-bit Handle structure:
 *
 *    <-- HDL_BITS_ROTATION --> <-- HDL_BITS_USAGE --> <-- HDL_BITS_SLOTID -->
 *    ------------------------------------------------------------------------
 *   |      ROTATION FIELD     |      USAGE FIELD     |      SLOT ID FIELD    |
 *    ------------------------------------------------------------------------
 * @endverbatim
 */
typedef CsrUint16 PldHdl;

/**
 * @brief
 *  Payload Manager Module statistics structure.
 *
 * @par Description
 *  This structure contains a set of statistics regarding the operation
 *  and status of the payload manager module.
 */
typedef struct {
    /** The maximum number of payloads the system can manage */
    CsrUint16    maxPayloads;
    /** The maximum number of payload tables the system can create */
    CsrUint16    maxTables;
    /** The size of a payload table (in payloads) */
    CsrUint16    tableSz;
    /** Logical Shift scalar to perform Fast multiple/divide operations */
    CsrUint16    tableSzShift;
    /** The number of payloads currently under payload manager control */
    CsrUint16    currentPayloads;
    /** The number of payload tables currently being used */
    CsrUint16    currentTables;
    /** The number of bytes of memory under payload manager control */
    CsrUint32    activeMemory;
    /** The total number of payload_store/_create invocations */
    CsrUint32    totalAllocs;
    /** The total number of payload_release invocations */
    CsrUint32    totalFrees;
    /** The max number of payload Allocations */
    CsrUint16    highPayloads;
    /** The max number of payload Allocations */
    CsrUint32    highActiveMemory;
} PldStats;

/**
 * @brief
 *   Initialises the payload manager subsytem.
 *
 * @par Description
 *   This function initialises the payload manager placing it in a 'ready'
 *   state (i.e. it may service other pld_() requests). This function MUST be
 *   called prior to calling any other pld_() function.
 * @par
 *   Initialisation of the payload manager involves creation of a 'directory'
 *   for tracking payloads and initialisation of a critical section handle to
 *   serialise certain operations on multi-threaded systems.
 * @par
 *   To avoid unnecessary use of resources the payload manager uses a
 *   distributed directory comprised of a number of 'tables'. Tables are only
 *   created when needed and each table contains an integer power of 2 number
 *   of payload 'slots' (e.g. 1, 2, 4, 8, etc). By sizing the table to be an
 *   integer power of two, the payload manager can identify the appropriate
 *   table to use via fast bit-shift operations instead of slower divisions.
 *   As a result of these features, this function uses the input arguments as
 *   hints - the actual table size and max number of payloads may therefore
 *   differ from that requested.
 *
 * @param[in]   tableSize   : size of each directory element in payload slots
 * @param[in]   maxPayloads : maximum number of payloads the system will accept
 *
 * @return
 *   none
 *
 * @remarks
 *   This function assumes that the underlying calls to allocate memory and
 *   initialise the critical section handle complete successfully.
 *
 *   See also pld_shutdown.
 */
PldContext* pld_init(CsrUint16 tableSize, CsrUint16 maxPayloads);

/**
 * @brief
 *   Frees redource for the payload manager subsytem.
 *
 * @par Description
 *   This function frees any resources allocated through the pld_init()
 *   function.  This should be called prior to termination of the program.
 */
void pld_shutdown(PldContext* context);

/**
 * @brief
 *   Retrieves statistics on the operation of the payload manager subsystem
 *
 * @par Description
 *   This populates a calling-context-supplied statistics structure with the
 *   runtime status of the payload manager subsystem. Information such as the
 *   total and available capacity of the subsystem are presented along with
 *   some additional counters.
 *
 * @param[out] userStats : Pointer to a statistics structure
 *
 * @return
 *   none
 */
void pld_get_stats(PldContext* context, PldStats *userStats);

/**
 * @brief
 *   Registers a block of data with the payload management system.
 *
 * @par Description
 *   This function firstly finds a free 'slot' in the payload manager directory
 *   reserves it, and then allocates the required amount of memory (length
 *   bytes). It then copies the data pointed to by buf into this space and
 *   returns a handle (via arg3) to the calling context that may later be used
 *   to access the data.
 *
 * @param[in]   buf    : Pointer to the data to store in the payload manager
 * @param[in]   length : The size of the data to store
 * @param[out]  hdl    : The handle under which the data gets stored
 *
 * @return
 *   none
 *
 * @remarks
 *   The 'source' data may be on the heap; if this is the case, it is the
 *   responsibility of the caller to ensure that this source memory is
 *   released.
 * @par
 *   This function makes the assumption that the underlying memory allocation
 *   function (i.e. CsrPmalloc) successfully services the allocation
 *   request. Failure to implement the underlying malloc as specified causes
 *   undefined behaviour in the event of a failed allocation request.
 */
#ifdef SME_TRACE_ENABLE
#define pld_store(context, buf, length, hdl) payld_store(__FILE__, __LINE__, 0, context, buf, length, hdl)
#define pld_store_extra(context, buf, length, hdl, extra) payld_store(__FILE__, __LINE__, extra, context, buf, length, hdl)
void payld_store(const char *callFile, int callLine, CsrUint16 extra, PldContext* context, void *buf, CsrUint16 length, PldHdl *hdl);
#else
#define pld_store(context, buf, length, hdl) payld_store(context, buf, length, hdl)
#define pld_store_extra(context, buf, length, hdl, extra) payld_store(context, buf, length, hdl)
void payld_store(PldContext* context, void *buf, CsrUint16 length, PldHdl *hdl);
#endif

/**
 * @brief
 *   Creates an (uninitialised) payload that the caller may populate later
 *
 * @par Description
 *   This function allows the caller to have the payload manager create a
 *   buffer of the required size (arg1) thereby allowing the avoidance of a
 *   data copy (see pld_store).
 *
 * @par
 *   The payload manager will return the buffer (arg2) and a handle (arg3).
 *   It is the responsibility of the caller to ensure that the buffer is then
 *   populated.
 *
 * @param[in]   length : The size of the payload
 * @param[out]  buf    : The address of the payload (buffer) upon creation
 * @param[out]  hdl    : The handle associated with this new payload
 *
 * @return
 *   none
 *
 * @remarks
 *   The code in receipt of the new payload must be careful not to overflow
 *   the space allocated while writing data into it. They must not write more
 *   than length bytes into the space pointed to by buf.
 */
#ifdef SME_TRACE_ENABLE
#define pld_create(context, length, buf, hdl) payld_create(__FILE__, __LINE__, context, length, buf, hdl)
void payld_create(const char *callFile, int callLine, PldContext* context, CsrUint16 length, void **buf, PldHdl *hdl);
#else
#define pld_create(context, length, buf, hdl) payld_create(context, length, buf, hdl)
void payld_create(PldContext* context, CsrUint16 length, void **buf, PldHdl *hdl);
#endif

/**
 * @brief
 *   Obtains a pointer to the payload and its length via its handle.
 *
 * @par Description
 *   This function allows the caller to recover a payload registered with the
 *   payload manager via its handle (arg1). The function returns the buffer
 *   address and length via arguments 2 and 3.
 *
 * @param[in]   hdl    : The handle of the payload
 * @param[out]  buf    : The address of the payload (buffer)
 * @param[out]  length : The length (in bytes) of the buffer
 *
 * @return
 *   none
 */
#ifdef SME_TRACE_ENABLE
#define pld_access(context, hdl, buf, length) payld_access(__FILE__, __LINE__, context, hdl, buf, length)
void payld_access(const char *callFile, int callLine, PldContext* context, PldHdl hdl, void **buf, CsrUint16 *length);
#else
#define pld_access(context, hdl, buf, length) payld_access(context, hdl, buf, length)
void payld_access(PldContext* context, PldHdl hdl, void **buf, CsrUint16 *length);
#endif

/**
 * @brief
 *   Deletes a payload recovering all resources used.
 *
 * @par Description
 *   This function allows the caller to delete a payload that is registered
 *   with the payload manager via its handle (arg1).
 *
 * @param[in]   hdl: The handle of the payload
 *
 * @return
 *   none
 */
#ifdef SME_TRACE_ENABLE
#define pld_release(context, hdl) payld_release(__FILE__, __LINE__, context, hdl)
void payld_release(const char *callFile, int callLine, PldContext* context, PldHdl hdl);
#else
#define pld_release(context, hdl) payld_release(context, hdl)
void payld_release(PldContext* context, PldHdl hdl);
#endif

/**
 * @brief
 *   Increments the usage count associated with a particular payload
 *
 * @par Description
 *   Allows the caller to pass the payload to another 'entity' yet still
 *   maintain an ownership of the data. Typically,  when payloads are moved
 *   between entities the implication is that the receiving entity now 'owns'
 *   the data. Once the receiver has finished with the data and calls
 *   payload_release, the data would normally be deleted. In the event that the
 *   transmitter wishes to prevent the data from being deleted, they must call
 *   this function PRIOR to transmitting the payload handle. In this scenario,
 *   two pld_release calls would be necessary before the data was deleted.
 *
 * @param[in]   hdl: The handle of the payload
 *
 * @return
 *   none
 */
#ifdef SME_TRACE_ENABLE
#define pld_add_ref(context, hdl) payld_add_ref(__FILE__, __LINE__, context, hdl)
void payld_add_ref(const char * callFile, int callLine, PldContext* context, PldHdl hdl);
#else
#define pld_add_ref(context, hdl) payld_add_ref(context, hdl)
void payld_add_ref(PldContext* context, PldHdl hdl);
#endif

/**
 * @brief
 *   Dumps the contents of a payload using sme_trace.
 *
 * @par Description
 *   This simple debug function prints out a payload
 *
 * @return
 *   void
 */
#ifdef SME_TRACE_ENABLE
void pld_trace(PldContext* context, sme_trace_module_id id, sme_trace_level level, const char* const message, PldHdl hdl);
#else
#define pld_trace(context, id, level, message, hdl)
#endif

/**
 * @brief
 *   Dumps the status of the payload manager subsystem using sme_trace.
 *
 * @par Description
 *   This simple debug function prints out status information including the
 *   number of payload slots available, the amount of memory registered with
 *   the payload subsystem, and the total number of stores/releases. Output
 *   is performed via calls to sme_trace_info.
 *
 * @return
 *   void
 */
#ifdef SME_TRACE_ENABLE
void pld_dump_stats(PldContext* context);
#else
#define pld_dump_stats(context)
#endif

#ifdef STANDALONE
void payload_manager_dump_diags(PldContext* context);
#endif

#ifdef __cplusplus
}
#endif

#endif /* PAYLOAD_MANAGER_H_ */
