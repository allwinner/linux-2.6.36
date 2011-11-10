/** @file csr_list.c
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
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/csr_cstl/csr_list.c#3 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/
#include "abstractions/osa.h"

/* PROJECT INCLUDES *********************************************************/
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "csr_cstl/csr_wifi_list.h"

/* MACROS *******************************************************************/

/** node initialization macro */
#define INIT_NODE(node,nextnode,mem,assignvalue) \
    node->next = nextnode; \
    node->memoryownership = mem; \
    node->value = assignvalue

/** node deletion macro */
#define DELETE_NODE(node) \
    { \
    csr_list_memory_ownership memory_ownership = node->memoryownership;\
    if(memory_ownership == list_owns_both || memory_ownership == list_owns_value) { CsrPfree(node->value); } \
    if(memory_ownership == list_owns_both || memory_ownership == list_owns_node)  { CsrPfree(node); } \
    }

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/*---------------------------------------------------------------------------*/
/**
 * @brief finds a previous node
 *
 * @par Description:
 * find a previous node in a list
 *
 * @ingroup csr_list
 *
 * @param[in] list : The list to be checked
 * @param[in] node : The node to seach infront of
 *
 * @return
 *      csr_list_node* : the located node
 */
static csr_list_node* findPrevNode(csr_list* list, csr_list_node* node)
{
    csr_list_node* curr = list->head;

    require(TR_CSR_LIST, list != NULL);
    require(TR_CSR_LIST, node != NULL);

    while (curr != NULL)
    {
        if (curr->next == node)
        {
            return curr;
        }
        curr = curr->next;
    }
    return curr;
}

/**
 * @brief checks is a node is in a list
 *
 * @par Description:
 * checks is a node is in a list
 *
 * @ingroup csr_list
 *
 * @param[in] list : The list to be checked
 * @param[in] node : The node to search for
 *
 * @return
 *      csr_list_node* : the located node
 */
csr_list_node* isInList(csr_list* list, csr_list_node* node)
{
    csr_list_node* curr = list->head;

    require(TR_CSR_LIST, list != NULL);
    require(TR_CSR_LIST, node != NULL);

    while (curr != NULL && curr != node)
    {
        curr = curr->next;
    }
    return curr;
}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
void csr_list_init(csr_list* list)
{
    require(TR_CSR_LIST, list != NULL);

    list->count = 0;
    list->head = NULL;
    list->tail = NULL;
}

/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
void csr_list_clear(csr_list* list)
{
    require(TR_CSR_LIST, list != NULL);

    while(list->head != NULL)
    {
        csr_list_node* node = list->head;
        list->head = list->head->next;
        DELETE_NODE(node);
    }

    csr_list_init(list);
}

/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
void csr_list_insert_head(csr_list* list, csr_list_memory_ownership ownership, csr_list_node* node, void* value)
{
    require(TR_CSR_LIST, list != NULL);
    require(TR_CSR_LIST, node != NULL);
    require(TR_CSR_LIST, value != NULL);
    require(TR_CSR_LIST, isInList(list, node) == NULL); /*lint !e666 */

    INIT_NODE(node, list->head, ownership, value);

    if (list->head == NULL)
    {
        list->tail = node;
    }
    list->head = node;
    list->count++;
}

/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
void csr_list_insert_tail(csr_list* list, csr_list_memory_ownership ownership, csr_list_node* node, void* value)
{
    require(TR_CSR_LIST, list != NULL);
    require(TR_CSR_LIST, node != NULL);
    require(TR_CSR_LIST, value != NULL);

    INIT_NODE(node, NULL, ownership, value);

    if ( list->head == NULL)
    {
        list->head = node;
    }
    else
    {
        list->tail->next = node;
    }

    list->tail = node;
    list->count++;
}

/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
void csr_list_insert_after(csr_list* list, csr_list_memory_ownership ownership, csr_list_node* existingnode, csr_list_node* node, void* value)
{
    require(TR_CSR_LIST, list != NULL);
    require(TR_CSR_LIST, existingnode != NULL);
    require(TR_CSR_LIST, node != NULL);
    require(TR_CSR_LIST, value != NULL);
    require(TR_CSR_LIST, isInList(list, existingnode) != NULL); /*lint !e666 */

    INIT_NODE(node, existingnode->next, ownership, value);

    existingnode->next = node;

    if (list->tail == existingnode)
    {
        list->tail = node;
    }
    list->count++;
}

#ifdef CSR_LIST_REFERENCE
/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
int csr_list_size(csr_list* list)
{
    require(TR_CSR_LIST, list != NULL);
    return list->count;
}

/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
int csr_list_isempty(csr_list* list)
{
    require(TR_CSR_LIST, list != NULL);
    return list->head == NULL;
}

/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
csr_list_node* csr_list_gethead_void(csr_list* list)
{
    require(TR_CSR_LIST, list != NULL);
    return list->head;
}

/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
csr_list_node* csr_list_gettail_void(csr_list* list)
{
    require(TR_CSR_LIST, list != NULL);
    return list->tail;
}

/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
csr_list_node* csr_list_getnext_void(csr_list* list, csr_list_node* node)
{
    require(TR_CSR_LIST, list != NULL);
    require(TR_CSR_LIST, node != NULL);
    require(TR_CSR_LIST, isInList(list, node) != NULL); /*lint !e666 */

    return node->next;
}
#endif

/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
csr_list_result csr_list_remove(csr_list* list, csr_list_node* node)
{
    require(TR_CSR_LIST, list != NULL);
    require(TR_CSR_LIST, node != NULL);
    require(TR_CSR_LIST, isInList(list, node) != NULL); /*lint !e666 */

    if (list->head == node)
    {
        list->head = node->next;
        if (list->head == NULL)
        {
            list->tail = NULL;
        }
        DELETE_NODE(node);
    }
    else
    {
        csr_list_node* prevnode = findPrevNode(list, node);
        if (prevnode == NULL)
        {
            return csr_list_not_found;
        }
        prevnode->next = node->next;
        if (prevnode->next == NULL)
        {
            list->tail = prevnode;
        }
        DELETE_NODE(node);
    }
    list->count--;
    return csr_list_success;
}

/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
void csr_list_removehead(csr_list* list)
{
    require(TR_CSR_LIST, list != NULL);

    if (list->head != NULL)
    {
        (void)csr_list_remove(list, list->head);
    }
}

/*
 * Description:
 * See description in csr_cstl/csr_wifi_list.h
 */
/*---------------------------------------------------------------------------*/
void csr_list_remove_tail(csr_list* list)
{
    csr_list_node* curr = list->head;
    csr_list_node* prev = NULL;

    require(TR_CSR_LIST, list != NULL);

    if (list->head != NULL)
    {
        while (curr->next != NULL)
        {
            prev = curr;
            curr = curr->next;
        }

        /* delete the node */
        DELETE_NODE(curr);

        /* and update the previous node to be tail */
        if (prev)
        {
            prev->next = NULL;
        }

        /* update the list tail */
        list->tail = prev;
        list->count--;
    }
}
