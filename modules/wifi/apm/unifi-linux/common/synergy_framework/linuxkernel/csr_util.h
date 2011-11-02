#ifndef _CSR_UTIL_H__
#define _CSR_UTIL_H__
/****************************************************************************

               (c) Cambridge Silicon Radio Limited 2009

               All rights reserved and confidential information of CSR

REVISION:      $Revision: #1 $
****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
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

#include "csr_types.h"

/*------------------------------------------------------------------*/
/* Bits - intended to operate on CsrUint32 values */
/*------------------------------------------------------------------*/
#define COAL_MASK_IS_SET(val, mask) (((val) & (mask)) == (mask))
#define COAL_MASK_IS_UNSET(val, mask) ((((val) & (mask)) ^ mask) == (mask))
#define COAL_MASK_SET(val, mask)    ((val) |= (mask))
#define COAL_MASK_UNSET(val, mask)  ((val) = ((val) ^ (mask)) & (val)) /* Unsets the bits in val that are set in mask */
#define COAL_BIT_IS_SET(val, bit)   ((CsrBool) ((((val) & (1UL << (bit))) != 0)))
#define COAL_BIT_SET(val, bit)      ((val) |= (1UL << (bit)))
#define COAL_BIT_UNSET(val, bit)    ((val) &=~(1UL << (bit)))
#define COAL_BIT_TOGGLE(val, bit)   ((val) ^= (1UL << (bit)))
extern CsrUint8 CoalSparseBitCount(CsrUint32 n);
extern CsrUint8 CoalDenseBitCount(CsrUint32 n);

/*------------------------------------------------------------------*/
/* Endian conversion */
/*------------------------------------------------------------------*/
#define COAL_GET_UINT16_FROM_LITTLE_ENDIAN(ptr)        (((CsrUint16)((CsrUint8 *)(ptr))[0]) | \
                                                        ((CsrUint16)((CsrUint8 *)(ptr))[1]) << 8)
#define COAL_GET_UINT32_FROM_LITTLE_ENDIAN(ptr)        (((CsrUint32)((CsrUint8 *)(ptr))[0]) | \
                                                        ((CsrUint32)((CsrUint8 *)(ptr))[1]) << 8 | \
                                                        ((CsrUint32)((CsrUint8 *)(ptr))[2]) << 16 | \
                                                        ((CsrUint32)((CsrUint8 *)(ptr))[3]) << 24)

#define COAL_COPY_UINT16_TO_LITTLE_ENDIAN(uint, ptr)    ((CsrUint8 *) (ptr))[0] = ((CsrUint8) ((uint) & 0x00FF)); \
                                                        ((CsrUint8 *) (ptr))[1] = ((CsrUint8) (((uint) >> 8) & 0x00FF))
#define COAL_COPY_UINT32_TO_LITTLE_ENDIAN(uint, ptr)    ((CsrUint8 *) (ptr))[0] = ((CsrUint8) ((uint) & 0x000000FF)); \
                                                        ((CsrUint8 *) (ptr))[1] = ((CsrUint8) (((uint) >> 8) & 0x000000FF)); \
                                                        ((CsrUint8 *) (ptr))[2] = ((CsrUint8) (((uint) >> 16) & 0x000000FF)); \
                                                        ((CsrUint8 *) (ptr))[3] = ((CsrUint8) (((uint) >> 24) & 0x000000FF))

#define COAL_GET_UINT16_FROM_BIG_ENDIAN(ptr) (((CsrUint16)((CsrUint8 *)(ptr))[1]) | ((CsrUint16)((CsrUint8 *)(ptr))[0]) << 8)
#define COAL_GET_UINT24_FROM_BIG_ENDIAN(ptr) (((CsrUint24)((CsrUint8 *)(ptr))[2]) | \
                                             ((CsrUint24)((CsrUint8 *)(ptr))[1]) << 8 | ((CsrUint24)((CsrUint8 *)(ptr))[0]) << 16)
#define COAL_GET_UINT32_FROM_BIG_ENDIAN(ptr) (((CsrUint32)((CsrUint8 *)(ptr))[3]) | ((CsrUint32)((CsrUint8 *)(ptr))[2]) << 8 |  \
                                             ((CsrUint32)((CsrUint8 *)(ptr))[1]) << 16 | ((CsrUint32)((CsrUint8 *)(ptr))[0]) << 24)
#define COAL_COPY_UINT16_TO_BIG_ENDIAN(uint, ptr)    ((CsrUint8 *) (ptr))[1] = ((CsrUint8) ((uint) & 0x00FF)); \
                                             ((CsrUint8 *) (ptr))[0] = ((CsrUint8) ((uint) >> 8))
#define COAL_COPY_UINT24_TO_BIG_ENDIAN(uint, ptr)    ((CsrUint8 *) (ptr))[2] = ((CsrUint8) ((uint) & 0x000000FF)); \
                                             ((CsrUint8 *) (ptr))[1] = ((CsrUint8) (((uint) >> 8) & 0x000000FF)); \
                                             ((CsrUint8 *) (ptr))[0] = ((CsrUint8) (((uint) >> 16) & 0x000000FF))
#define COAL_COPY_UINT32_TO_BIG_ENDIAN(uint, ptr)    ((CsrUint8 *) (ptr))[3] = ((CsrUint8) ((uint) & 0x000000FF)); \
                                             ((CsrUint8 *) (ptr))[2] = ((CsrUint8) (((uint) >> 8) & 0x000000FF)); \
                                             ((CsrUint8 *) (ptr))[1] = ((CsrUint8) (((uint) >> 16) & 0x000000FF)); \
                                             ((CsrUint8 *) (ptr))[0] = ((CsrUint8) (((uint) >> 24) & 0x000000FF))

/*------------------------------------------------------------------*/
/* Base conversion */
/*------------------------------------------------------------------*/
extern CsrBool CsrHexStrToUint8(const char * string, CsrUint8 * returnValue);
extern CsrBool CsrHexStrToUint16(const char * string, CsrUint16 * returnValue);
extern CsrBool CsrHexStrToUint32(const char * string, CsrUint32 * returnValue);
extern CsrUint32 CsrPow(CsrUint32 base, CsrUint32 exponent);
extern void CsrIntToBase10(CsrInt32 number, char *str);
extern void CsrUInt16ToHex(CsrUint16 number, char * str);
extern void CsrUInt32ToHex(CsrUint32 number, char * str);

/*------------------------------------------------------------------*/
/*  String */
/*------------------------------------------------------------------*/
#define CsrMemCpy memcpy
#define CsrMemSet memset
#define CsrMemMove memmove
#define CsrMemCmp memcmp
extern void *CsrMemDup(const void *buf1, CsrInt32 count);
#define CsrStrCpy strcpy
#define CsrStrNCpy strncpy
extern int CsrStrNICmp(const char *string1, const char *string2, CsrUint32 count);
#define CsrStrCat strcat
#define CsrStrNCat strncat
#define CsrStrStr strstr
#define CsrStrLen strlen
#define CsrStrCmp strcmp
#define CsrStrNCmp strncmp
extern char *CsrStrDup(const char *string);
#define CsrStrChr strchr
extern CsrUint32 CsrStrToInt(const char *string);
#define CsrSprintf sprintf
extern char *CsrStrNCpyZero(char * dest, char * src, CsrUint32 count);

/*------------------------------------------------------------------*/
/*  Wcs String */
/*------------------------------------------------------------------*/
extern CsrUint32 CsrWcsLen( const CsrUint16 *string );
extern CsrUint16 *CsrWcsCpy( CsrUint16 *dest, const CsrUint16 *src );
extern CsrUint16 *CsrWcsCat( CsrUint16 *dest, const CsrUint16 *src );
extern CsrInt32 CsrRevIWcsNMatchC( const CsrUint16 *src, const CsrUint16 pattern, CsrInt32 cnt );
extern CsrInt32 CsrRevIWcsNMatch( const CsrUint16 *src, const CsrUint16 *pattern, CsrInt32 cnt );

/*------------------------------------------------------------------*/
/* Filename */
/*------------------------------------------------------------------*/
const char *CsrGetBaseName(const char *file);

/*------------------------------------------------------------------*/
/* Misc */
/*------------------------------------------------------------------*/
#define COAL_TOUPPER(character)    (((character) >= 'a') && ((character) <= 'z') ? ((character) - 0x20) : (character))
#define COAL_TOLOWER(character)    (((character) >= 'A') && ((character) <= 'Z') ? ((character) + 0x20) : (character))
extern CsrBool CoalIsSpace(CsrUint8 c);
#define COAL_ARRAY_SIZE(x) (sizeof(x)/sizeof(*(x)))

void CsrLog(CsrUint16 logLevel, CsrCharString * prefixString,
            const CsrCharString * format, ...);
void CsrLogBuffer(CsrUint16 logLevel, CsrCharString* prefixString,
                  CsrUint8* buffer, CsrUint16 BufferLength,
                  const CsrCharString* format, ...);


#ifdef __cplusplus
}
#endif

#endif

