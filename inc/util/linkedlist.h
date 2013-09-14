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
static inline linkedlist_node_t* linkedlist_createNode ( void* data )
{
     linkedlist_node_t* node = malloc ( sizeof ( linkedlist_node_t ) );
     node->data = data;
     node->next = NULL;
     return node;
}

/**
 * Frees a node and returns the contained data (must be freed seperately)
 * \param node - to be freed
 * \return pointer to stored data
 */
static inline void* linkedlist_freeNode ( linkedlist_node_t* node )
{
     void * data = node->data;
     free ( node );
     return data;
}

/**
 * Comparator and processor
 */
typedef bool ( *LinkedListComparator_t ) ( void* compareValue, void* data );
typedef void ( *LinkedListProcessor_t ) ( linkedlist_node_t* data, void* parameter );

/**
 * Search an element. This method returns a pointer to the first linkedlist_node_t d1 of lst with comp(d1.data)==true.
 * If such an element is not found null is returned.
 *
 * \param list - linkedlist
 * \param comp - comparator
 * \param compareValue - 1 st parameter of comp
 * \return   pointer to the first linkedlist_node_t d1 of lst with comp(d1.data)==true or null if none exists.
 */
static inline linkedlist_node_t* linkedlist_searchNode ( linkedlist_t* list, LinkedListComparator_t comp, void* compareValue )
{
     for ( volatile linkedlist_node_t** it = &list->next; *it != NULL; it = ( volatile linkedlist_node_t** ) & ( ( *it )->next ) ) {
          if ( comp ( compareValue, ( void* ) ( *it )->data ) ) {
               return ( linkedlist_node_t* ) *it;
          }
     }
     return NULL;
}


/**
 * Execute method process for every element contained.
 *
 * \param list - linkedlist
 * \param process - handle data
 * \param parameter - 2nd parameter for process
 * \return   pointer to the first linkedlist_node_t d1 of lst with comp(d1.data)==true or null if none exists.
 */
static inline void linkedlist_processList ( linkedlist_t* list, LinkedListProcessor_t process, void* parameter )
{
     for ( volatile linkedlist_node_t** it = &list->next; *it != NULL; it = ( volatile linkedlist_node_t** ) & ( ( *it )->next ) ) {
          process ( ( linkedlist_node_t* ) ( *it ), parameter ) ;
     }
}

/**
 * Remove all element with a data null pointer
 */
static inline void linkedlist_cleanup ( linkedlist_t* list )
{
     for ( volatile linkedlist_node_t** it = &list->next; *it != NULL; it = ( volatile linkedlist_node_t** ) & ( ( *it )->next ) ) {
          if ( ( *it )->data == NULL ) {
               linkedlist_node_t* oldElement = ( linkedlist_node_t* ) *it;
               *it = ( ( *it )->next );
               linkedlist_freeNode ( oldElement );
               --list->elements;
          }
          // end of list reached
          if ( *it == NULL )
               return;
     }
}

/**
 * Add a new element to the front of the list
 */
static void inline linkedlist_pushToFront ( linkedlist_t* list, linkedlist_node_t* node )
{
     volatile linkedlist_node_t* oldFirst = list->next;
     list->next = node;
     node->next = oldFirst;
     ++list->elements;
}
#endif
