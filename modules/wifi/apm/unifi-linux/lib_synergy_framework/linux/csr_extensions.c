/****************************************************************************

               (c) Cambridge Silicon Radio Limited 2009

               All rights reserved and confidential information of CSR

REVISION:      $Revision: #3 $
****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "csr_sched.h"
#include "csr_panic.h"
#include "csr_pmalloc.h"
#include "csr_framework_ext.h"

/* --------------------------------------------------------------------------
 * Initialise the mutex so that it can be used to lock
 * areas of code */
CsrErrorCode CsrMutexCreate(CsrMutexHandle* mutex)
{
    if (mutex == NULL)
    {
        return CSR_ERROR_INVALID_POINTER;
    }

    *mutex = (pthread_mutex_t*)CsrPmalloc(sizeof(pthread_mutex_t));

    if (pthread_mutex_init(*mutex, NULL) != 0)
    {
        return CSR_ERROR_NO_MORE_MUTEXES;
    }
    return CSR_ERROR_SUCCESS;
}

/* --------------------------------------------------------------------------
 * Destroys the mutex so that the associate resources are freed
 */
void CsrMutexDestroy(CsrMutexHandle mutex)
{
    pthread_mutex_destroy(mutex);
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

    if (pthread_mutex_lock(mutex) != 0)
    {
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

    if (pthread_mutex_unlock(mutex) != 0)
    {
        return CSR_ERROR_INVALID_HANDLE;
    }
    return CSR_ERROR_SUCCESS;
}

void CsrPanic(CsrUint8 tech, CsrUint16 reason, const char *p)
{
    fprintf(stderr, "CsrPanic :: %s", p);
    abort();
}

CsrUint32 CsrRandom(void)
{
    static CSR_TIME s = 0;
    if(s == 0)
    {
        CsrSchedGetUtc(NULL, &s);
        srand((unsigned int)s);
    }
    return (CsrUint32)rand();
}

/*------------------------------------------------------------------*/
/* Base conversion */
/*------------------------------------------------------------------*/
CsrBool CsrHexStrToUint8(const char *string, CsrUint8 *returnValue)
{
    CsrUint16 currentIndex = 0;
    *returnValue = 0;
    if ((string[currentIndex] == '0') && (CSR_TOUPPER(string[currentIndex + 1]) == 'X'))
    {
        string += 2;
    }
    if (((string[currentIndex] >= '0') && (string[currentIndex] <= '9')) || ((CSR_TOUPPER(string[currentIndex]) >= 'A') && (CSR_TOUPPER(string[currentIndex]) <= 'F')))
    {
        while (((string[currentIndex] >= '0') && (string[currentIndex] <= '9')) || ((CSR_TOUPPER(string[currentIndex]) >= 'A') && (CSR_TOUPPER(string[currentIndex]) <= 'F')))
        {
            *returnValue = (CsrUint8) (*returnValue * 16 + (((string[currentIndex] >= '0') && (string[currentIndex] <= '9')) ? string[currentIndex] - '0' : CSR_TOUPPER(string[currentIndex]) - 'A' + 10));
            currentIndex++;
            if (currentIndex >= 2)
            {
                break;
            }
        }
        return TRUE;
    }
    return FALSE;
}
