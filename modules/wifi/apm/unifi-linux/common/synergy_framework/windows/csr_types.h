#ifndef CSR_TYPES_H__
#define CSR_TYPES_H__
/****************************************************************************

               (c) Cambridge Silicon Radio Limited 2009

               All rights reserved and confidential information of CSR

REVISION:      $Revision: #1 $
****************************************************************************/

#undef  CSRMAX
#define CSRMAX(a,b)    (((a) > (b)) ? (a) : (b))

#undef  CSRMIN
#define CSRMIN(a,b)    (((a) < (b)) ? (a) : (b))

/* To shut lint up. */
#undef  CSR_UNUSED
#define CSR_UNUSED(x)           (void)(x)
#define CSR_PARAM_UNUSED(x)     ((void)(x))

#define CSR_INVALID_PHANDLE     0xFFFF /* Invalid protocol handle setting */
#define CSR_INVALID_TIMERID     (0)

/* Data types */

typedef unsigned        char    CsrUint8;
typedef unsigned short  int     CsrUint16;
typedef unsigned long   int     CsrUint32;

typedef signed   char           CsrInt8;
typedef signed   short  int     CsrInt16;
typedef signed   long   int     CsrInt32;

typedef unsigned char           CsrChar;

/* Synergy types */
typedef int                     delimiter_for_start_of_csr_types_t;

typedef unsigned char           CsrBool;

typedef unsigned char           CsrString;
typedef unsigned char           CsrUcs2String; /* UCS2 oriented byte-pairs ordered (MSB,LSB) strings */
typedef CsrUint16               CsrUtf16String; /* 16-bit UTF16 strings */
typedef CsrUint32               CsrUint24;

#ifndef NULL
#   define NULL (0)
#endif

#endif
