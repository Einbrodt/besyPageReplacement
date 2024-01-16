/* Implementation of the log functions */
/* for comments on the global functions see the associated .h-file	*/

/* ---------------------------------------------------------------- */
/* Include required external definitions */
#include <math.h>
#include "bs_types.h"
#include "global.h"
#include "log.h"


extern sim_frame_t sim_memoryMap[MEMORYSIZE];    // Array storing the use of physical memory. For simulation use ONLY!


/* ---------------------------------------------------------------- */
/*                Declarations of local helper functions            */

/* ---------------------------------------------------------------- */
/* Declarations of global variables visible only in this file 		*/
// array with strings associated to scheduling events for log outputs
char eventString[3][12] = {"completed", "io", "quantumOver"};

/* ---------------------------------------------------------------- */
/*                Externally available functions					*/
/* ---------------------------------------------------------------- */

void logGeneric(char *message) {
    printf("%6u : %s\n", systemTime, message);
}

void logPid(unsigned pid, char *message) {
    printf("%6u : PID %3u : %s\n", systemTime, pid, message);
}

void logPidMemAccess(unsigned pid, action_t action) {
    printf("%6u : PID %3u : ", systemTime, pid);
    if (action.op == write) printf("Write");
    if (action.op == read) printf(" Read");
    printf("-Access to Page: %3u\n", action.page);
}

void logPidMemPhysical(unsigned pid, unsigned page, unsigned frame) {
    printf("%6u : PID %3u : Resolving page %2u in frame %2u\n",
           systemTime, pid, page, frame);
}

void logMemoryMapping(void)
/* prints out a memory map showing the use of all frames of the physical mem*/

{
    int frame;
    printf("%6u : Current allocation of physical memory: [PID, page, R-Bit] per frame\n",
        systemTime);
    printf("\t00          01          02          03          04          05          06          07    \n");
    for (int row = 0; row <= (MEMORYSIZE / 8); row++)   // loop for rows
    {
        printf("%6u\t", row);
        for (int column = 0; column < 8; column++)
        {
            frame = (row * 8 + column);
            if (frame >= MEMORYSIZE) break;
            printf("[%2u,", sim_memoryMap[frame].pid);

            if (sim_memoryMap[frame].pid == 0)
                printf(" -,");
            else {
                printf("%2x,", sim_memoryMap[frame].page);
            }
            printf(" %d]  ", getReferencedBit(frame));

        }
        printf("\n");
    }
}
    
    
    
    /*
    int frame, intervals, pid;
    intervals = systemTime / TIMER_INTERVAL;
    pid = 0;
    for (int row = 0; row <= (MEMORYSIZE / 8); row++)   // loop for rows
    {
        printf("%6u\t", row);
        for (int column = 0; column < 8; column++)
        {
            printf("SystemTime %6u\n", systemTime);
            /* for (int interval = 0; interval < intervals; interval++) {

                printf("Interval \t%d\n", interval);
            }
            frame = (row * 8 + column);
            if (frame >= MEMORYSIZE) break;
            printf("PID: %d\n", sim_memoryMap[frame].pid);
            printf("Page: ");
            if (sim_memoryMap[frame].pid == 0)
                printf("- | ");
            else {
                printf("%d | ", sim_memoryMap[frame].page);
                printf("%d | ", getReferencedBit(frame));
            }
           
            printf("\n\n");
            

        }
    }
}
*/




int getReferencedBit(int frame) {
    unsigned pid = sim_memoryMap[frame].pid;
    unsigned page = sim_memoryMap[frame].page;

    if (processTable[pid].valid && processTable[pid].pageTable != NULL) {
        pageTableEntry_t* pageEntry = &processTable[pid].pageTable[page];
        return pageEntry->referenced;
    }
    return 0;
}

    
	



/* ----------------------------------------------------------------- */
/*                       Local helper functions                      */
/* ----------------------------------------------------------------- */



