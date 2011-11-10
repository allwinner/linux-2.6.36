/** @file csr_pmalloc.c
 *
 * Linux Kernal pMalloc implementation
 * It is part of the porting exercise.
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2007-2009. All rights reserved.
 *
 * @section DESCRIPTION
 *   Provides abstraction malloc and free implementation
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 ****************************************************************************/

#include "csr_pmalloc.h"

void *CsrPmalloc(unsigned int sz)
{
    CsrUint8 *p_alloc;

    p_alloc = (CsrUint8*)kmalloc(sz, GFP_KERNEL);
    if (p_alloc == NULL) {
        return NULL;
    }

    return (p_alloc);
}

void CsrPfree(void *ptr)
{
    if (!ptr) {
        return;
    }

    kfree(ptr);
}

