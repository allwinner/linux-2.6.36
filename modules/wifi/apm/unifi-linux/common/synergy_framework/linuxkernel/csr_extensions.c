/****************************************************************************

               (c) Cambridge Silicon Radio Limited 2009

               All rights reserved and confidential information of CSR

REVISION:      $Revision: #1 $
****************************************************************************/

#include "csr_pmalloc.h"
#include "csr_framework_ext.h"
#include "csr_panic.h"
#include <linux/semaphore.h>


/* --------------------------------------------------------------------------
 * Initialise the mutex so that it can be used to lock
 * areas of code */
CsrErrorCode CsrMutexCreate(CsrMutexHandle* mutex)
{
    if (mutex == NULL)
    {
        return CSR_ERROR_INVALID_POINTER;
    }

    *mutex = (struct semaphore*)CsrPmalloc(sizeof(struct semaphore));
    init_MUTEX(*mutex);

    return CSR_ERROR_SUCCESS;
}

/* --------------------------------------------------------------------------
 * Destroys the mutex so that the associate resources are freed
 */
void CsrMutexDestroy(CsrMutexHandle mutex)
{
    if (mutex == NULL)
    {
        return;
    }

    CsrPfree(mutex);
}

/* --------------------------------------------------------------------------
 * Marks the code following this function as critical. This means no other
 * context that uses the same critical section handle may execute the code
 */
CsrErrorCode CsrMutexLock(CsrMutexHandle mutex)
{
    if (mutex == NULL)
    {
        return CSR_ERROR_INVALID_HANDLE;
    }

    if (down_interruptible(mutex))
    {
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE, "CsrMutexLock Failed");
        return CSR_ERROR_INVALID_HANDLE;
    }

    return CSR_ERROR_SUCCESS;
}

/* --------------------------------------------------------------------------
 * Marks the end of the critical section - many execution contexts may
 * execute the code after this call.
 */
CsrErrorCode CsrMutexUnlock(CsrMutexHandle mutex)
{
    if (mutex == NULL)
    {
        return CSR_ERROR_INVALID_HANDLE;
    }

    up(mutex);

    return CSR_ERROR_SUCCESS;
}

void CsrPanic(CsrUint8 tech, CsrUint16 reason, const char *p)
{
    WARN_ON(1);
}


void CsrThreadSleep(CsrUint32 sleepTimeInMs)
{
    unsigned long t, j;

    /* Convert t in ms to jiffies and round up */
    t = ((sleepTimeInMs * HZ) + 999) / 1000;
    j = schedule_timeout_interruptible(t);

}




void *CsrMemAlloc(size_t size)
{
    return kmalloc(size, GFP_KERNEL);
} /* CsrMemAlloc() */


void CsrMemFree(void *pointer)
{
    kfree(pointer);
} /* CsrMemFree() */
