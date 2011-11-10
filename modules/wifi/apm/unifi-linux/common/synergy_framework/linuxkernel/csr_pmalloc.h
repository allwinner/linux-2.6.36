#ifndef CSR_PMALLOC_H__
#define CSR_PMALLOC_H__
/****************************************************************************

               (c) Cambridge Silicon Radio Limited 2009

               All rights reserved and confidential information of CSR

REVISION:      $Revision: #1 $
****************************************************************************/

#include "csr_types.h"
#include "csr_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrPmalloc
 *
 *  DESCRIPTION
 *      Returns a pointer to a block of memory of length "size" bytes obtained
 *      from the pools.
 *
 *      Panic()s on failure.
 *
 *  RETURNS
 *      void * - pointer to allocated block
 *
 *----------------------------------------------------------------------------*/
extern void *CsrPmalloc(size_t size);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrPfree
 *
 *  DESCRIPTION
 *      Return a memory block previously obtained via CsrPmalloc to the pools.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
extern void CsrPfree(void *ptr);

/* Allocate memory and set it to all zeroes */
#define CsrZpmalloc(s) (CsrMemSet(CsrPmalloc(s), 0x00, (s)))


#ifdef __cplusplus
}
#endif

#endif

