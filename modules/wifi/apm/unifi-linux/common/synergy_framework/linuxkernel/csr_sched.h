#ifndef COAL_SCHED_H__
#define COAL_SCHED_H__
/****************************************************************************

               (c) Cambridge Silicon Radio Limited 2009

               All rights reserved and confidential information of CSR

REVISION:      $Revision: #1 $
****************************************************************************/

#include "csr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* System time, in microseconds */
typedef CsrUint32                CSR_TIME;

/* Time of day in UTC, for CsrSchedGetUtc() */
typedef struct {
    CsrUint32 sec;
    CsrUint16 msec;
} csr_utctime;

/* An identifier issued by the scheduler. */
typedef CsrUint32                CsrSchedulerIdentifier;

/* A task identifier */
typedef CsrUint16                CsrTaskId;

/* A queue identifier */
typedef CsrUint16                CsrQid;

/* A message identifier */
typedef CsrSchedulerIdentifier    CsrMsgId;

/* A timer event identifier */
typedef CsrSchedulerIdentifier    CsrTid;

/* Scheduler entry functions share this structure */
typedef void (*schedEntryFunction_t)(void **inst);

/* Time constants. */
#define COAL_TIME_MAX                ((CSR_TIME) 0xFFFFFFFF)
#define COAL_MILLISECOND             ((CSR_TIME)(1000))
#define COAL_SECOND                  ((CSR_TIME)(1000 * COAL_MILLISECOND))
#define COAL_MINUTE                  ((CSR_TIME)(60 * COAL_SECOND))

/* Queue and primitive that identifies the environment */
#define COAL_TASK_ID                0xFFFF
#define COAL_PRIM                   (COAL_TASK_ID)
#define COAL_EXCLUDED_MODULE_QUEUE  0xFFFF

/* Queue number bit that should never leave the BlueCore */
#define COAL_QUEUE_ON_CHIP        0x8000

/* Part of the queue-id that is used to identify a segment
 * and the number of bits to shift to obtain it */
#define COAL_QUEUE_SEGMENT        0x7000
#define COAL_QUEUE_SEGMENT_SHIFT  12

/* Maximum number of supported segments */
#define COAL_MAX_SEGMENTS         4

/* Define a global task queue id number */
#define CSR_DECLARE_TASK(task) \
    CsrUint16 task = COAL_TASK_ID

#define INIT_TASK(queue, init, deinit, handler, task_prim, task_prim_ver, data, id) \
    CsrSchedRegisterTask(&queue, init, deinit, handler, #handler, task_prim, task_prim_ver, data, id)

/*
 * Background interrupt definitions
 */
#define CSR_BGINT_INVALID   0
typedef CsrUint16 CsrBgint;

typedef void (*CsrBgintHandler)(void *);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBgintRegister
 *
 *  DESCRIPTION
 *      Register a background interrupt handler function with the scheduler.
 *        When CsrBgint() is called from the foreground (e.g. an interrupt
 *        routine) the registered function is called.
 *
 *        If "cb" is null then the interrupt is effectively disabled. If a
 *        no bgints are available, CSR_BGINT_INVALID is returned, otherwise
 *        a CsrBgint value is returned to be used in subsequent calls to
 *        CsrBgint().  id is a possibly NULL identifier used for logging
 *        purposes only.
 *
 *  RETURNS
 *      CsrBgint -- CSR_BGINT_INVALID denotes failure to obtain a CsrBgintSet.
 *
 *----------------------------------------------------------------------------*/
CsrBgint CsrBgintRegister(CsrBgintHandler cb, void *context, const char *id_str, CsrUint16 id, CsrUint8 id_version);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBgintUnregister
 *
 *  DESCRIPTION
 *      Unregister a background interrupt handler function.
 *
 *      ``irq'' is a background interrupt handle previously obtained
 *      from a call to CsrBgintRegister().
 *
 *  RETURNS
 *      void.
 *
 *----------------------------------------------------------------------------*/
void CsrBgintUnregister(CsrBgint irq);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBgintSet
 *
 *  DESCRIPTION
 *      Set background interrupt.
 *
 *  RETURNS
 *      void.
 *
 *----------------------------------------------------------------------------*/
extern void CsrBgintSet(CsrBgint);

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
extern CSR_TIME CsrGetTime(void);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedGetUtc
 *
 *  DESCRIPTION
 *      Get the current system wallclock timestamp in UTC.
 *      Specifically, if tod is non-NULL, the contents will be set to the
 *      number of seconds (plus any fraction of a second in milliseconds)
 *      since January 1st 1970.  If tm is non-NULL, the contents will be
 *      set to the current system time as would have been returned by
 *      CsrGetTime().
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
extern void CsrSchedGetUtc(csr_utctime *tod, CSR_TIME *tm);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeAdd
 *
 *  DESCRIPTION
 *      Add two time values. Adding the numbers can overflow the range of a
 *      CSR_TIME, so the user must be cautious.
 *
 *  RETURNS
 *      CSR_TIME - the sum of "t1" and "t2".
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeAdd(t1, t2) ((t1) + (t2))

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeSub
 *
 *  DESCRIPTION
 *      Subtract two time values. Subtracting the numbers can provoke an
 *      underflow, so the user must be cautious.
 *
 *  RETURNS
 *      CSR_TIME - "t1" - "t2".
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeSub(t1, t2)    ((CsrInt32) (t1) - (CsrInt32) (t2))

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeEq
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" equal "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeEq(t1, t2) ((t1) == (t2))

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeGt
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" is greater than "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeGt(t1, t2) (CsrTimeSub((t1), (t2)) > 0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeGe
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" is greater than, or equal to "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeGe(t1, t2) (CsrTimeSub((t1), (t2)) >= 0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeLt
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" is less than "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeLt(t1, t2) (CsrTimeSub((t1), (t2)) < 0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeLe
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" is less than, or equal to "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeLe(t1, t2) (CsrTimeSub((t1), (t2)) <= 0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrPutMessage
 *
 *  DESCRIPTION
 *      Sends a message consisting of the integer "mi" and the void * pointer
 *      "mv" to the message queue "q".
 *
 *      "mi" and "mv" are neither inspected nor changed by the scheduler - the
 *      task that owns "q" is expected to make sense of the values. "mv" may
 *      be null.
 *
 *  NOTE
 *      If "mv" is not null then it will typically be a chunk of CsrPmalloc()ed
 *      memory, though there is no need for it to be so. Tasks should normally
 *      obey the convention that when a message built with CsrPmalloc()ed memory
 *      is given to CsrPutMessage() then ownership of the memory is ceded to the
 *      scheduler - and eventually to the recipient task. I.e., the receiver of
 *      the message will be expected to CsrPfree() the message storage.
 *
 *  RETURNS
 *      CsrMsgId - message identifier.
 *
 *----------------------------------------------------------------------------*/
#ifdef STRING_LOG__
extern CsrMsgId CsrPutMessageStringLog(CsrQid q, CsrUint16 mi, void *mv,
                                       CsrUint32 line, CsrCharString *file);
#define CsrPutMessage(q, mi, mv) CsrPutMessageStringLog((q), (mi), (mv), __LINE__, (CsrCharString *) __FILE__)
#else
extern CsrMsgId CsrPutMessage(CsrQid q, CsrUint16 mi, void *mv);
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBroadcastMessage
 *
 *  DESCRIPTION
 *      Sends a message to all tasks.
 *
 *      The user must supply a "factory function" that is called once
 *      for every task that exists. The "factory function", msg_build_func,
 *      must allocate and initialise the message and set the msg_build_ptr
 *      to point to the message when done.
 *
 *  NOTE
 *      N/A
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
#ifdef STRING_LOG__
extern void CsrBroadcastMessageStringLog(CsrUint16 mi,
                                         void *(*msg_build_func)(void*),
                                         void *msg_build_ptr,
                                         CsrUint32 line,
                                         CsrCharString *file);
#define CsrBroadcastMessage(mi, fn, ptr) CsrBroadcastMessageStringLog((mi), (fn), (ptr), __LINE__, (CsrCharString *) __FILE__)
#else
extern void CsrBroadcastMessage(CsrUint16 mi, void *(*msg_build_func)(void*),
                                void *msg_build_ptr);
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrGetMessage
 *
 *  DESCRIPTION
 *      Obtains a message from the message queue "q" if one is available. The
 *      calling task must own "q". The message consists of one or both of a
 *      CsrUint16 and a void *.
 *
 *      If the calling task does not own "q" then the scheduler calls panic().
 *
 *  RETURNS
 *      CsrBool - TRUE if a message has been obtained from the queue, else FALSE.
 *      If a message is taken from the queue, then "*pmi" and "*pmv" are set to
 *      the "mi" and "mv" passed to CsrPutMessage() respectively.
 *
 *      "pmi" and "pmv" can be null, in which case the corresponding value from
 *      them message is discarded.
 *
 *----------------------------------------------------------------------------*/
extern CsrBool CsrGetMessage(CsrQid q,
                          CsrUint16 *pmi,
                          void **pmv);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimedEventIn
 *
 *  DESCRIPTION
 *      Causes the void function "fn" to be called with the arguments
 *      "fniarg" and "fnvarg" after "delay" has elapsed.
 *
 *      "delay" must be less than half the range of a CSR_TIME.
 *
 *      CsrTimedEventIn() does nothing with "fniarg" and "fnvarg" except
 *      deliver them via a call to "fn()".   (Unless CsrCancelTimedEvent()
 *      is used to prevent delivery.)
 *
 *  NOTE
 *      The function will be called at or after "delay"; the actual delay will
 *      depend on the timing behaviour of the scheduler's tasks.
 *
 *  RETURNS
 *      CsrTid - A timed event identifier, can be used in CsrCancelTimedEvent().
 *
 *----------------------------------------------------------------------------*/
#ifdef STRING_LOG__
extern CsrTid CsrTimedEventInStringLog(CSR_TIME delay,
                                       void (*fn)(CsrUint16 mi, void *mv),
                                       CsrUint16 fniarg, void *fnvarg,
                                       CsrUint32 line, CsrCharString *file);
#define CsrTimedEventIn(d, fn, fni, fnv) CsrTimedEventInStringLog((d), (fn), (fni), (fnv), __LINE__, (CsrCharString *) __FILE__)
#else
extern CsrTid CsrTimedEventIn(CSR_TIME delay,
                              void (*fn)(CsrUint16 mi, void *mv),
                              CsrUint16 fniarg, void *fnvarg);
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrCancelTimedEvent
 *
 *  DESCRIPTION
 *      Attempts to prevent the timed event with identifier "eventid" from
 *      occurring.
 *
 *  RETURNS
 *      CsrBool - TRUE if cancelled, FALSE if the event has already occurred.
 *
 *----------------------------------------------------------------------------*/
#ifdef STRING_LOG__
extern CsrBool CsrCancelTimedEventStringLog(CsrTid eventid, CsrUint16 *pmi,
                                            void **pmv,
                                            CsrUint32 line,
                                            CsrCharString *file);
#define CsrCancelTimedEvent(e, pmi, pmv) CsrCancelTimedEventStringLog((e), (pmi), (pmv), __LINE__, (CsrCharString *) __FILE__)
#else
extern CsrBool CsrCancelTimedEvent(CsrTid eventid, CsrUint16 *pmi, void **pmv);
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedGetTaskQueue
 *
 *  DESCRIPTION
 *      Return the queue identifier for the currently running queue
 *
 *  RETURNS
 *      CsrQid - The current task queue identifier, or 0xFFFF if not available.
 *
 *----------------------------------------------------------------------------*/
extern CsrQid CsrSchedGetTaskQueue(void);

/* Register a task */
void CsrSchedRegisterTask(CsrQid *queue, schedEntryFunction_t init,
                          schedEntryFunction_t deinit,
                          schedEntryFunction_t handler,
                          const CsrCharString *name, CsrUint16 task_id,
                          const CsrCharString *task_id_ver,
                          void *data, CsrUint16 id);

/* Scheduler log interface */
#define CSR_SCHED_CUR_CTX_TASK      ((CsrUint8) 0x00)
#define CSR_SCHED_CUR_CTX_BGINT     ((CsrUint8) 0x01)
#define CSR_SCHED_CUR_CTX_EXT       ((CsrUint8) 0x02)

typedef struct {
    CsrUint8     type;           /* <! Type */
    CsrUint8     thread_id;      /* <! Thread ID */
    CsrUint16    task_id;        /* <! Task ID */
} CsrSchedTaskCtx;

typedef struct {
    CsrUint8     type;           /* <! Type */
    CsrUint8     thread_id;      /* <! Thread ID */
    CsrUint8     bgint;          /* <! BG Int */
} CsrSchedBgIntCtx;

typedef struct {
    CsrUint8     type;           /* <! Type */
} CsrSchedExtCtx;

typedef union {
    CsrUint8 type;
    CsrSchedTaskCtx task;
    CsrSchedBgIntCtx bgint;
    CsrSchedExtCtx ext;
} CsrSchedCurCtx;

#ifdef STRING_LOG__
/* Get current log context */
void CsrSchedGetCurCtx(CsrSchedCurCtx *ctx);
#endif

#ifdef __cplusplus
}
#endif

#endif
