/****************************************************************************

               (c) Cambridge Silicon Radio Limited 2009

               All rights reserved and confidential information of CSR

REVISION:      $Revision: #1 $
****************************************************************************/

#include <sys/timeb.h>
#include <sys/time.h>

#include "csr_sched.h"
#include "csr_types.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrGetTime
 *
 *  DESCRIPTION
 *      Get the current system time.
 *
 *  RETURNS
 *      CSR_TIME - the current system time.
 *
 *----------------------------------------------------------------------------*/
CSR_TIME CsrGetTime(void)
{
    struct timeb tb;
    CSR_TIME realNow;

    ( void ) ftime( &tb );

    realNow = ( CSR_TIME )(( tb.time * 1000000 ) + (tb.millitm * 1000));

    return(realNow);
}

void CsrSchedGetUtc(csr_utctime *tod, CSR_TIME *tm)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    if (tm != NULL)
    {
        *tm = (tv.tv_sec % 4290) * 1000000 + tv.tv_usec * 1000;
    }

    if (tod != NULL)
    {
        tod->sec = tv.tv_sec;
        tod->msec = tv.tv_usec / 1000;
    }
}
