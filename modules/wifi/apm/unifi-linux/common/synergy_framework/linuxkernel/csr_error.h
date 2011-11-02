#ifndef CSR_ERROR_H__
#define CSR_ERROR_H__
/****************************************************************************

               (c) Cambridge Silicon Radio Limited 2009

               All rights reserved and confidential information of CSR

REVISION:      $Revision: #1 $
****************************************************************************/
#include <linux/errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CSR_ENOENT      ENOENT
#define CSR_EIO         EIO
#define CSR_ENOMEM      ENOMEM
#define CSR_EFAULT      EFAULT
#define CSR_EEXIST      EIO
#define CSR_ENODEV      ENODEV
#define CSR_EINVAL      EINVAL
#define CSR_ENOSPC      ENOSPC
#define CSR_ERANGE      ERANGE
#define CSR_ENODATA     ENODATA
#define CSR_ETIMEDOUT   ETIMEDOUT
#define CSR_ENOTSUP     EIO

#ifdef __cplusplus
}
#endif

#endif
