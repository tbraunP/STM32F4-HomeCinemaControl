#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "FreeRTOS.h"

/**
 * Ringbuffer structure
 *
 */
typedef struct linkedlist_node_t {
     void* data;
     volatile struct linkedlist_node_t* next;
} linkedlist_node_t;

typedef struct linkedlist_t {
     size_t elements;
     volatile linkedlist_node_t* next;
} linkedlist_t;

/**
 * Create an empty linked list
 */
static inline linkedlist_t createLinkedList()
{
     return ( ( linkedlist_t ) {
          .next = NULL, .elements = 0
     } );
}

/**
 * Create a new linkedNode on the heap, be sure that data is static or stored on the heap
 * \param data - data to be added
 * \return linkedlist_node with data
 */
static inline linkedlist_node_t* createNode ( void* data )
{
     linkedlist_node_t* node = malloc ( sizeof ( linkedlist_node_t ) );
     node->data = data;
     return node;
}

/**
 * Frees a node and returns the contained data (must be freed seperately)
 * \param node - to be freed
 * \return pointer to stored data
 */
static inline void* freeNode ( linkedlist_node_t* node )
{
     void * data = node->data;
     free ( node );
     return data;
}

/**
 * Comparator
 */
typedef bool ( *LinkedListComparator_t ) ( void* data );

/**
 * Search an element. This method returns a pointer to the first linkedlist_node_t d1 of lst with comp(d1.data)==true.
 * If such an element is not found null is returned.
 * \param list - linkedlist
 * \param comp - comparator
 * \return   pointer to the first linkedlist_node_t d1 of lst with comp(d1.data)==true or null if none exists.
 */
static inline linkedlist_node_t* searchNode ( linkedlist_t* list, LinkedListComparator_t comp )
{
     for ( volatile linkedlist_node_t** it = &list->next; *it != NULL; it = ( volatile linkedlist_node_t** ) & ( ( *it )->next ) ) {
          if ( comp ( ( void* ) ( *it )->data ) ) {
               return ( linkedlist_node_t* ) *it;
          }
     }
     return NULL;
}

#endif
