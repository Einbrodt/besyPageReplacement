/* Include-file defining global defines and constants  	 		*/
/* used by the operating system */

/* Changed Variables by Tamara Boerner and Nicole Einbrodt */
#ifndef __GLOBAL__
#define __GLOBAL__

#define	_CRT_SECURE_NO_WARNINGS		// suppress legacy warnings 

#include <stdlib.h>
#include <time.h>
#include "bs_types.h"
#include "core.h"
#include "memoryManagement.h"
#include "processcontrol.h"
#include "log.h"
#include "simruntime.h"
#include "timer.h"


// Number of possible concurrent processes, i.e. size of the process table 
#define MAX_PROCESSES (unsigned)100

// Size of the physical memory available to user processes in frames
// This value shall be changed to a suitable number for more realistic testing. 
// The system must run for an arbitrary (but reasonable) memory size
#define MEMORYSIZE 8		//physical Memory Size set to 8 [TB, NE]

// Period of the timer. on all multiples of this value the timer ISR ist called by the simulation
#define TIMER_INTERVAL 50			// *** This value must not be changed! ***

#define FILENAME_LENGTH 128
// name of the file with process definitions
#define PROCESS_FILENAME "processes.txt"
// name of the file with the simulation run an empty file name switches to random event stimulus
#define RUN_FILENAME "run_many_intervals.txt"
//#define RUN_FILENAME ""

/* ----------------------------------------------------------------	*/
/* Define global variables that will be visible in all sourcefiles	*/
extern unsigned int	maxPID;				// largest valid PID
extern PCB_t processTable[MAX_PROCESSES + 1]; 	// the process table
extern unsigned systemTime; 			// the current system time (up time)
int rBits[MEMORYSIZE];			// the reference bits for the clock algorithm]

/* ----------------------------------------------------------------	*/
/* Define global constants that will be visible in all sourcefiles	*/
#define NONE -1			// indicating that no frame was found
#define NOPROCESS 0		// Indicating that no process is used/allocated

#endif  /* __GLOBAL__ */ 