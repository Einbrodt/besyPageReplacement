/* Include-file defining functions of the OS-core 	 		*/
#ifndef __CORE__
#define __CORE__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>



void initOS(void);
/* all initialisation steps are started in this function					*/

void shutdownOS(void);
/* clear up and de-allocate memory used by the OS							*/
/* must be extended, if the function initOS() is extended						*/

Boolean coreLoop(void);
/* the core scheduling loop of the OS										*/
/* returns TRUE if the stimulus was completed without error					*/			
/* returns FALSE when an error occurres that prevents completion of the sim */

#endif /* __CORE__ */