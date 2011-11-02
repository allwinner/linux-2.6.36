/****************************************************************************

               (c) Cambridge Silicon Radio Limited 2009

               All rights reserved and confidential information of CSR

REVISION:      $Revision: #1 $
****************************************************************************/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/netdevice.h>

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
    CsrUint32 timeMilliseconds;
    timeMilliseconds = jiffies_to_msecs(jiffies);
    return timeMilliseconds;
}

void CsrSchedGetUtc(csr_utctime *tod, CSR_TIME *tm)
{
    CsrUint32 timeMilliseconds = jiffies_to_msecs(jiffies);
    timeMilliseconds = jiffies_to_msecs(jiffies);

    if (tm != NULL)
    {
        *tm = timeMilliseconds * COAL_MILLISECOND;
    }

    if (tod != NULL)
    {
        tod->sec  = timeMilliseconds / COAL_MILLISECOND;
        tod->msec = timeMilliseconds % COAL_MILLISECOND;
    }
}
