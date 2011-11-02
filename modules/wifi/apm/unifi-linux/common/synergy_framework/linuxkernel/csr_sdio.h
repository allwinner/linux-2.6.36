#ifndef CSR_SDIO_H__
#define CSR_SDIO_H__
/****************************************************************************

               (c) Cambridge Silicon Radio Limited 2009

               All rights reserved and confidential information of CSR

REVISION:      $Revision: #1 $
****************************************************************************/

#include <csr_types.h>
#include <csr_error.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CSR_SDIO_RESULT_DEVICE_WAS_RESET    0
#define CSR_SDIO_RESULT_DEVICE_NOT_RESET    1

#define CSR_SDIO_FEATURE_BYTE_MODE  0x00000001  /* Byte mode CMD53 supported */

#define CSR_SDIO_ANY_MANF_ID        0xFFFF
#define CSR_SDIO_ANY_CARD_ID        0xFFFF
#define CSR_SDIO_ANY_SDIO_FUNCTION  0xFF
#define CSR_SDIO_ANY_SDIO_INTERFACE 0xFF

typedef struct
{
    CsrUint16 manfId;       /* Vendor ID to match or CSR_SDIO_ANY_MANF_ID */
    CsrUint16 cardId;       /* Device ID to match or CSR_SDIO_ANY_CARD_ID */
    CsrUint8 sdioFunction;  /* SDIO Function number to match or CSR_SDIO_ANY_SDIO_FUNCTION */
    CsrUint8 sdioInterface; /* SDIO Standard Interface Code to match or CSR_SDIO_ANY_SDIO_INTERFACE */
} CsrSdioFunctionId;

typedef struct
{
    CsrSdioFunctionId sdioId;
    CsrUint16 blockSize;
    CsrUint32 features;
    void *driverData;   /* For use by the Function Driver */
    void *priv;         /* For use by the SDIO Driver */
} CsrSdioFunction;

typedef void (*CsrSdioDsrCallback)(CsrSdioFunction *function, CsrInt32 result);
typedef CsrSdioDsrCallback (*CsrSdioCallback)(CsrSdioFunction *function, CsrInt32 result);

typedef struct
{
    CsrInt32 (*inserted)(CsrSdioFunction *function);
    void (*removed)(CsrSdioFunction *function);
    CsrSdioDsrCallback (*interrupt)(CsrSdioFunction *function);
    CsrInt32 (*suspend)(CsrSdioFunction *function);
    CsrInt32 (*resume)(CsrSdioFunction *function);
    CsrSdioFunctionId *ids;
    CsrUint8 idsCount;
} CsrSdioFunctionDriver;

CsrInt32 CsrSdioFunctionDriverRegister(CsrSdioFunctionDriver *functionDriver);
void CsrSdioFunctionDriverUnregister(CsrSdioFunctionDriver *functionDriver);
CsrInt32 CsrSdioFunctionEnable(CsrSdioFunction *function);
CsrInt32 CsrSdioFunctionDisable(CsrSdioFunction *function);
void CsrSdioFunctionActive(CsrSdioFunction *function);
void CsrSdioFunctionIdle(CsrSdioFunction *function);
CsrInt32 CsrSdioRead8(CsrSdioFunction *function, CsrUint32 address, CsrUint8 *data);
CsrInt32 CsrSdioWrite8(CsrSdioFunction *function, CsrUint32 address, CsrUint8 data);
CsrInt32 CsrSdioRead16(CsrSdioFunction *function, CsrUint32 address, CsrUint16 *data);
CsrInt32 CsrSdioWrite16(CsrSdioFunction *function, CsrUint32 address, CsrUint16 data);
CsrInt32 CsrSdioF0Read8(CsrSdioFunction *function, CsrUint32 address, CsrUint8 *data);
CsrInt32 CsrSdioF0Write8(CsrSdioFunction *function, CsrUint32 address, CsrUint8 data);
CsrInt32 CsrSdioRead(CsrSdioFunction *function, CsrUint32 address, void *data, CsrUint32 length);
CsrInt32 CsrSdioWrite(CsrSdioFunction *function, CsrUint32 address, const void *data, CsrUint32 length);
CsrInt32 CsrSdioInterruptEnable(CsrSdioFunction *function);
CsrInt32 CsrSdioInterruptDisable(CsrSdioFunction *function);
void CsrSdioInterruptAcknowledge(CsrSdioFunction *function);
CsrInt32 CsrSdioPowerOn(CsrSdioFunction *function);
void CsrSdioPowerOff(CsrSdioFunction *function);
CsrInt32 CsrSdioSuspend(CsrSdioFunction *function);
CsrInt32 CsrSdioResume(CsrSdioFunction *function);
CsrInt32 CsrSdioHardReset(CsrSdioFunction *function);
CsrInt32 CsrSdioBlockSizeSet(CsrSdioFunction *function, CsrUint16 blockSize);
void CsrSdioRead8Async(CsrSdioFunction *function, CsrUint32 address, CsrUint8 *data, CsrSdioCallback callback);
void CsrSdioWrite8Async(CsrSdioFunction *function, CsrUint32 address, CsrUint8 data, CsrSdioCallback callback);
void CsrSdioRead16Async(CsrSdioFunction *function, CsrUint32 address, CsrUint16 *data, CsrSdioCallback callback);
void CsrSdioWrite16Async(CsrSdioFunction *function, CsrUint32 address, CsrUint16 data, CsrSdioCallback callback);
void CsrSdioF0Read8Async(CsrSdioFunction *function, CsrUint32 address, CsrUint8 *data, CsrSdioCallback callback);
void CsrSdioF0Write8Async(CsrSdioFunction *function, CsrUint32 address, CsrUint8 data, CsrSdioCallback callback);
void CsrSdioReadAsync(CsrSdioFunction *function, CsrUint32 address, void *data, CsrUint32 length, CsrSdioCallback callback);
void CsrSdioWriteAsync(CsrSdioFunction *function, CsrUint32 address, const void *data, CsrUint32 length, CsrSdioCallback callback);

#ifdef __cplusplus
}
#endif

#endif
