#ifndef COMMAND_DISPATCHER_H_
#define COMMAND_DISPATCHER_H_
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// include massage types for Command_t structs payload
#include "irmp/irmp.h"
#include "tasks/solidStateTask.h"
#include "tasks/irmpTask.h"

#ifdef __cplusplus
extern "C" {
#endif

// Common data structure to forward received commands (via ethernet) to the other components
typedef struct __attribute__ ( ( __packed__ ) ) Command_t {
     size_t len;
     uint8_t component; // adress of target component (indirectly identifies also the contained type)
     // payload must be stored on the heap!
     union __attribute__ ( ( __packed__ ) ) {
          IRMP_DATA* irsndData;
          IRMP_Command_t* irmpCommand;
          SolidStateCommand_t* solidStateCommands;
          uint8_t* raw;
     }payload;
} Command_t;


// number of components
#define MAX_COMPONENTS  (3)

// Identifiers for the components, must be common knowledge between client and controller
typedef enum Components_t {
     IRSND = 0x00,
     IRMP = 0x01,
     SOLIDSTATE = 0x02
} Components_t;



// Init dispatcher
void Dispatcher_init();

// forward command to
void Dispatcher_dispatch ( Command_t* command );

#ifdef __cplusplus
}
#endif
#endif
