/* Implementation of the timer handler function								*/
/* for comments on the global functions see the associated .h-file			*/
#include "bs_types.h"
#include "global.h"
#include "timer.h"
#include "processcontrol.h"

void timerEventHandler(void)
/* The event Handler (aka ISR) of the timer event. 							*/
/* Updates the data structures used by the page replacement algorithm 		*/

/* xxxx This function is a stub with reduced functionality, it must be xxxx */
/* xxxx extended for advanced memory management function to enable     xxxx */
/* xxxx full functionality of the operating system					   xxxx */
{
    logGeneric("Processing Timer Event Handler: resetting R-Bits");
    // in absence of a data structure indexing the pages that are present, all 
    // running processes and all present pages must be checked. 
    // If the page is present, the R-bit is reset.
    for (unsigned pid = 1; pid <= MAX_PROCESSES; pid++)
    {
        if (processTable[pid].valid && processTable[pid].pageTable != NULL)
        {
            // Iterate over all pages of the process
            for (unsigned page = 0; page < processTable[pid].size; page++)
            {
                pageTableEntry_t* pageEntry = &processTable[pid].pageTable[page];

                // If the page is present, reset the referenced bit
                if (pageEntry->present)
                {
                    pageEntry->referenced = FALSE;
                }
            }
        }
    }
    // for a more sophisticated memory management systems with reasonable 
    // page replacement, this timer event endler must be improved
}