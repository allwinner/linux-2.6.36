#ifndef CSR_EXTENSIONS_H__
#define CSR_EXTENSIONS_H__
/****************************************************************************

               (c) Cambridge Silicon Radio Limited 2009

               All rights reserved and confidential information of CSR

REVISION:      $Revision: #2 $
****************************************************************************/

#include <pthread.h>
#include <semaphore.h>

#include "csr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Result codes */
typedef CsrInt16 CsrErrorCode;
#define CSR_ERROR_SUCCESS          ( 0)
#define CSR_ERROR_NO_MORE_EVENTS   (-1)
#define CSR_ERROR_INVALID_POINTER  (-2)
#define CSR_ERROR_INVALID_HANDLE   (-3)
#define CSR_ERROR_NO_MORE_MUTEXES  (-4)
#define CSR_ERROR_TIMEOUT          (-5)
#define CSR_ERROR_NO_MORE_THREADS  (-6)

#define CSR_EVENT_WAIT_INFINITE   0xFFFF

/* CSR ext log levels */
#define CSR_EXT_LOG_LEVEL_FATAL_ERROR    0
#define CSR_EXT_LOG_LEVEL_CRITICAL_ERROR 1
#define CSR_EXT_LOG_LEVEL_MINOR_ERROR    2
#define CSR_EXT_LOG_LEVEL_WARNING        3
#define CSR_EXT_LOG_LEVEL_INFORMATION    4
#define CSR_EXT_LOG_LEVEL_DBG_1          5
#define CSR_EXT_LOG_LEVEL_DBG_2          6
#define CSR_EXT_LOG_LEVEL_DBG_3          7
#define CSR_EXT_LOG_LEVEL_DBG_4          8
#define CSR_EXT_LOG_LEVEL_DBG_5          9
#define CSR_EXT_LOG_LEVEL_DBG_6         10
#define CSR_EXT_LOG_LEVEL_DBG_7         11
#define CSR_EXT_LOG_LEVEL_DBG_8         12

#define CSR_TOUPPER(character)    (((character) >= 'a') && ((character) <= 'z') ? ((character) - 0x20) : (character))
#define CSR_TOLOWER(character)    (((character) >= 'A') && ((character) <= 'Z') ? ((character) + 0x20) : (character))

typedef sem_t*             CsrEventHandle;
typedef pthread_mutex_t*   CsrMutexHandle;
typedef pthread_t*         CsrThreadHandle;
typedef CsrUint32          CsrTickCounter; /* Should this be a CsrUint32??? */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrEventCreate
 *
 *  DESCRIPTION
 *      Creates an event and returns a handle to the created event.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_ERROR_SUCCESS          in case of success
 *          CSR_ERROR_NO_MORE_EVENTS   in case of out of event resources
 *          CSR_ERROR_INVALID_POINTER  in case the eventHandle pointer is invalid
 *
 *----------------------------------------------------------------------------*/
CsrErrorCode CsrEventCreate(CsrEventHandle *eventHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrEventWait
 *
 *  DESCRIPTION
 *      Wait fore one or more of the event bits to be set.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_ERROR_SUCCESS              in case of success
 *          CSR_ERROR_TIMEOUT              in case of timeout
 *          CSR_ERROR_INVALID_HANDLE       in case the eventHandle is invalid
 *          CSR_ERROR_INVALID_POINTER      in case the eventBits pointer is invalid
 *
 *----------------------------------------------------------------------------*/
CsrErrorCode CsrEventWait(CsrEventHandle eventHandle, CsrUint16 timeoutInMs, CsrUint32 *eventBits);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrEventSet
 *
 *  DESCRIPTION
 *      Set an event.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_ERROR_SUCCESS              in case of success
 *          CSR_ERROR_INVALID_HANDLE       in case the eventHandle is invalid
 *
 *----------------------------------------------------------------------------*/
CsrErrorCode CsrEventSet(CsrEventHandle eventHandle, CsrUint32 eventBits);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrEventDestroy
 *
 *  DESCRIPTION
 *      Destroy the event associated.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrEventDestroy(CsrEventHandle eventHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMutexCreate
 *
 *  DESCRIPTION
 *      Create a mutex and return a handle to the created mutex.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_ERROR_SUCCESS           in case of success
 *          CSR_ERROR_NO_MORE_MUTEXES   in case of out of mutex resources
 *          CSR_ERROR_INVALID_POINTER   in case the mutexHandle pointer is invalid
 *
 *----------------------------------------------------------------------------*/
CsrErrorCode CsrMutexCreate(CsrMutexHandle *mutexHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMutexLock
 *
 *  DESCRIPTION
 *      Lock the mutex refered to by the provided handle.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_ERROR_SUCCESS           in case of success
 *          CSR_ERROR_INVALID_HANDLE    in case the mutexHandle is invalid
 *
 *----------------------------------------------------------------------------*/
CsrErrorCode CsrMutexLock(CsrMutexHandle mutexHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMutexUnlock
 *
 *  DESCRIPTION
 *      Unlock the mutex refered to by the provided handle.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_ERROR_SUCCESS           in case of success
 *          CSR_ERROR_INVALID_HANDLE    in case the mutexHandle is invalid
 *
 *----------------------------------------------------------------------------*/
CsrErrorCode CsrMutexUnlock(CsrMutexHandle mutexHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMutexDestroy
 *
 *  DESCRIPTION
 *      Destroy the previously created mutex.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrMutexDestroy(CsrMutexHandle mutexHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrThreadCreate
 *
 *  DESCRIPTION
 *      Create thread function and return a handle to the created thread.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_ERROR_SUCCESS           in case of success
 *          CSR_ERROR_NO_MORE_THREADS   in case of out of thread resources
 *          CSR_ERROR_INVALID_POINTER   in case one of the supplied pointers is invalid
 *
 *----------------------------------------------------------------------------*/
CsrErrorCode CsrThreadCreate(void (* threadFunction)(void *pointer), void *pointer, CsrUint32 stackSize, CsrUint16 priority,
                             CsrString *threadName, CsrThreadHandle *threadHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrThreadSleep
 *
 *  DESCRIPTION
 *      Sleep for a given period.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrThreadSleep(CsrUint32 sleepTimeInMs);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMemAlloc
 *
 *  DESCRIPTION
 *      Allocate dynamic memory of a given size.
 *
 *  RETURNS
 *      Pointer to allocated memroy, or NULL in case of failure.
 *      Allocated memory is not initialised.
 *
 *----------------------------------------------------------------------------*/
#define CsrMemAlloc(s) malloc(s)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMemFree
 *
 *  DESCRIPTION
 *      Free dynamic allocated memory.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
#define CsrMemFree(p) free(p)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeGetTickCount
 *
 *  DESCRIPTION
 *      Return the current tick count.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_ERROR_SUCCESS           in case of success
 *          CSR_ERROR_INVALID_POINTER   in case the tickCounter pointer is invalid
 *
 *----------------------------------------------------------------------------*/
CsrErrorCode CsrTimeGetTickCount(CsrTickCounter *tick);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrExtLog
 *
 *  DESCRIPTION
 *      Logs the provided message.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrExtLog(CsrUint16 logLevel, CsrString *prefixString, const CsrString *format, ...);

#ifdef __cplusplus
}
#endif

#endif
