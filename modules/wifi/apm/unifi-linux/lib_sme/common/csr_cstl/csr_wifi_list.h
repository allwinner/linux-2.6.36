/** @file csr_list.h
 *
 * Abstract singly linked list implementation
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   Abstract singly linked list implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/csr_cstl/csr_wifi_list.h#1 $
 *
 ****************************************************************************/
#ifndef __CSR_LIST_H__
#define __CSR_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup csr_list
 */

/* STANDARD INCLUDES ********************************************************/
#include "csr_types.h"

/* PROJECT INCLUDES *********************************************************/

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/
/** list memory ownership enum declaration */
typedef enum csr_list_memory_ownership
{
    list_owns_both,
    list_owns_node,
    list_owns_value,
    list_owns_neither
} csr_list_memory_ownership;

/** list result enum declaration */
typedef enum csr_list_result
{
    csr_list_success,
    csr_list_error,
    csr_list_not_found
} csr_list_result;

/** list node declaration */
typedef struct csr_list_node
{
    /** pointer to the next node */
    struct csr_list_node* next;
    /** memory ownership flag */
    csr_list_memory_ownership memoryownership;
    /** pointer to the data */
    void* value;
} csr_list_node;

/** list anchor declaration */
typedef struct csr_list
{
    /** Number of entries in the list */
    CsrUint16      count;
    /** pointer to head node */
    csr_list_node* head;
    /** pointer to tail node */
    csr_list_node* tail;
} csr_list;

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/
/**
 * @brief initializes a list
 *
 * @par Description
 * initializes a list
 *
 * @param[in] list : The list to be acted on.
 *
 * @return
 *        void
 */
extern void csr_list_init(csr_list* list);


/**
 * @brief clears the list of all elements
 *
 * @par Description
 * This function clears the list of all elements. The node and data
 * is deleted as defined by the ownership rights.
 *
 * @param[in] list : The list to be acted on.
 *
 * @return
 *        void
 */
extern void csr_list_clear(csr_list* list);


/**
 * @brief inserts a node into a list at a the head
 *
 * @par Description
 * This function inserts a node into a list at a the head
 *
 * @param[in] list         : The list to be acted on.
 * @param[in] ownership    : who owns the node and data.
 * @param[in] node         : The new node.
 * @param[in] value        : The data to be added.
 *
 * @return
 *        void
 */
extern void csr_list_insert_head (csr_list* list, csr_list_memory_ownership ownership, csr_list_node* node, void* value);


/**
 * @brief inserts a node into a list at a the tail
 *
 * @par Description
 * This function inserts a node into a list at a the tail
 *
 * @param[in] list         : The list to be acted on.
 * @param[in] ownership    : who owns the node and data.
 * @param[in] node         : The new node.
 * @param[in] value        : The data to be added.
 *
 * @return
 *        void
 */
extern void csr_list_insert_tail (csr_list* list, csr_list_memory_ownership ownership, csr_list_node* node, void* value);


/**
 * @brief inserts a node into a list at a given point
 *
 * @par Description
 * This function inserts a node into a list at a given point
 *
 * @param[in] list         : The list to be acted on.
 * @param[in] ownership    : who owns the node and data.
 * @param[in] existingnode : The marker Node .
 * @param[in] node         : The new node.
 * @param[in] value        : The data to be added.
 *
 * @return
 *        void
 */
extern void csr_list_insert_after(csr_list* list, csr_list_memory_ownership ownership, csr_list_node* existingnode, csr_list_node* node, void* value);


/**
 * @brief Removed a given node
 *
 * @par Description
 * This function removed a specified node from the given list.
 *
 * @param[in] list : The list to be acted on.
 * @param[in] node : The node to be removed.
 *
 * @return
 *        void
 */
extern csr_list_result csr_list_remove(csr_list* list, csr_list_node* node);


/**
 * @brief removed head node
 *
 * @par Description
 * This function removed a the head node.
 *
 * @param[in] list : The list to be acted on.
 *
 * @return
 *        void
 */
extern void csr_list_removehead(csr_list* list);


/**
 * @brief removed tail node
 *
 * @par Description
 * This function removed a the tail node.
 *
 * @param[in] list : The list to be acted on.
 *
 * @return
 *        void
 */
extern void csr_list_remove_tail(csr_list* list);

#ifdef CSR_LIST_REFERENCE

/**
 * @brief calulates the number of elements in the list
 *
 * @par Description
 * This function calulates the number of elements in the list
 *
 * @param[in] list         : The list to be acted on.
 *
 * @return
 *        int : the number of elements in the list
 */
extern int csr_list_size(csr_list* list);

/**
 * @brief checks if a list is empty
 *
 * @par Description
 * This function checks if a list is empty.
 *
 * @param[in] list : The list to be acted on.
 *
 * @return
 *        void
 */
extern int csr_list_isempty(csr_list* list);

/**
 * @brief return the head of the list
 *
 * @par Description
 * return the head of the list in the native csr_list_node form
 *
 * @param[in] list : The list to be acted on.
 *
 * @return
 *        void
 */
extern csr_list_node* csr_list_gethead_void(csr_list* list);


/**
 * @brief return the tail of the list
 *
 * @par Description
 * return the tail of the list in the native csr_list_node form
 *
 * @param[in] list : The list to be acted on.
 *
 * @return
 *        void
 */
extern csr_list_node* csr_list_gettail_void(csr_list* list);


/**
 * @brief checks if a list is empty
 *
 * @par Description
 * This function checks if a list is empty.
 *
 * @param[in] list : The list to be acted on.
 *
 * @return
 *        void
 */
extern csr_list_node* csr_list_getnext_void(csr_list* list, csr_list_node* node);

#else /* CSR_LIST_REFERENCE */

/** optimised macro replacement for function call */
#define csr_list_isempty(list) ((list)->head == NULL)
/** optimised macro replacement for function call */
#define csr_list_gethead_void(list) ((list)->head)
/** optimised macro replacement for function call */
#define csr_list_gettail_void(list) ((list)->tail)
/** optimised macro replacement for function call */
#define csr_list_getnext_void(list,node) ((node)->next)
/** optimised macro replacement for function call */
#define csr_list_size(list) ((list)->count)
#endif /* CSR_LIST_REFERENCE */

/* ----------------------------------------------------------- */

/* ----------------------------------------------------------- */
/* Type casting Macro API*/
/* ----------------------------------------------------------- */
/** returns the head with the type specified */
#define csr_list_gethead_t(type,list) ((csr_list_gethead_void(list)==NULL)?NULL:(type)csr_list_gethead_void(list)->value)
/** returns the tail with the type specified */
#define csr_list_gettail_t(type,list) ((csr_list_gettail_void(list)==NULL)?NULL:(type)csr_list_gettail_void(list)->value)
/** returns the next value with the type specified from the given node*/
#define csr_list_getnext_t(type,list,node) ((csr_list_getnext_void(list,node)==NULL)?NULL:(type)csr_list_getnext_void(list,node)->value)
/** returns the value in a node with the type specified */
#define csr_list_getvalue_t(type,node) ((node==NULL)?NULL:(type)node->value)
/* ----------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __CSR_LIST_H__ */
