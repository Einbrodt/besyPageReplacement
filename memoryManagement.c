/* Tamara Boerner and Nicole Einbrodt							*/

// memorymanagement.c : Implementation of the memory mananager 
/* for comments on the global functions see the associated .h-file	*/
//

#include "memoryManagement.h"
#include "processcontrol.h"
#include <limits.h>

Boolean memoryManagerInitialised = FALSE;
unsigned emptyFrameCounter = 0;		// number of empty Frames 
frameList_t emptyFrameList = NULL;
frameListEntry_t* emptyFrameListTail = NULL;

/* ------------------------------------------------------------------------ */
/*		               Declarations of local helper functions				*/


Boolean isPagePresent(unsigned pid, unsigned page);
/* Predicate returning the present/absent status of the page in memory		*/

Boolean storeEmptyFrame(int frame);
/* Store the frame number in the data structure of empty frames				*/
/* and update emptyFrameCounter												*/

int getEmptyFrame(void);
/* Returns the frame number of an empty frame.								*/
/* A return value of -1 indicates that no empty frame exists. In this case	*/
/* a page replacement algorithm must be called to free evict a page and		*/
/* thus clear one frame */

Boolean movePageOut(unsigned pid, unsigned page, int frame);
/* Creates an empty frame at the given location.							*/
/* Copies the content of the frame occupid by the given page to secondary	*/
/* storage.																	*/
/* The exact location in seocondary storage is found and alocated by		*/
/* this function.															*/
/* The page table entries are updated to reflect that the page is no longer */
/* present in RAM, including its location in seondary storage				*/
/* Returns TRUE on success and FALSE on any error							*/

Boolean movePageIn(unsigned pid, unsigned page, unsigned frame);
/* Returns TRUE on success and FALSE on any error							*/

Boolean updatePageEntry(unsigned pid, action_t action);
/* updates the data relevant for page replacement in the page table entry,	*/
/* e.g. set reference and modyfy bit.										*/
/* In this simulation this function has to cover also the required actions	*/
/* nornally done by hardware, i.e. it summarises the actions of MMu and OS  */
/* when accessing physical memory.											*/
/* Returns TRUE on success and FALSE on any error							*/

Boolean pageReplacement(unsigned* pid, unsigned* page, int* frame);
/* ===== The page replacement algorithm								======	*/
/* In the initial implementation the frame to be cleared is chosen			*/
/* globaly and randomly, i.e. a frame is chosen at random regardless of the	*/
/* process that is currently usigt it.										*/
/* The values of pid and page number passed to the function may be used by  */
/* local replacement strategies */
/* OUTPUT: */
/* The frame number, the process ID and the page currently assigned to the	*/
/* frame that was chosen by this function as the candidate that is to be	*/
/* moved out and replaced by another page is returned via the call-by-		*/
/* reference parameters.													*/
/* Returns TRUE on success and FALSE on any error							*/


/* ------------------------------------------------------------------------ */
/*                Start of public Implementations							*/


Boolean initMemoryManager(void)
{
	// mark all frames of the physical memory as empty 
	for (int i = 0; i < MEMORYSIZE; i++) {
		storeEmptyFrame(i);
		rBits[i] = 0;
	}
	memoryManagerInitialised = TRUE;		// flag successfull initialisation

	return TRUE;
}

Boolean shutdownMemoryManager(void)
{
	// iterate the list of empty frames, destroy all entries and free the memory
	frameListEntry_t* toBeDeleted = NULL;
#pragma warning(push)
#pragma warning(disable : 6001)		// Avoid warning for uninitialised variable: emptyFrameList is known
	while (emptyFrameCounter > 0) {
		if (emptyFrameList == NULL) return FALSE;	// no empty frame exists, but there should 
		// remove entry first frame from the list
		toBeDeleted = emptyFrameList;
		emptyFrameList = emptyFrameList->next;
		free(toBeDeleted);
		emptyFrameCounter--;		// one empty frame less
	}
	memoryManagerInitialised = FALSE;		// memoryManager is no longer initialised
	return TRUE;
#pragma warning(pop)
}

int accessPage(unsigned pid, action_t action)
/* handles the mapping from logical to physical address, i.e. performs the	*/
/* task of the MMU and parts of the OS in a computer system					*/
/* Returns the number of the frame on success, also in case of a page fault */
/* Returns a negative value on error										*/
{
	int frame = INT_MAX;		// the frame the page resides in on return of the function
	unsigned outPid = pid;
	unsigned outPage = action.page;
	// check if page is present
	if (isPagePresent(pid, action.page))
	{// yes: page is present
		// look up frame in page table and we are done
		frame = processTable[pid].pageTable[action.page].frame;
	}
	else
	{// no: page is not present
		logPid(pid, "Pagefault");
		// check for an empty frame
		frame = getEmptyFrame();
		if (frame < 0)
		{	// no empty frame available: start replacement algorithm to find candidate frame
			logPid(pid, "No empty frame found, running replacement algorithm");
			pageReplacement(&outPid, &outPage, &frame);
			// move candidate frame out to secondary storage
			movePageOut(outPid, outPage, frame);
			frame = getEmptyFrame();
		} // now we have an empty frame to move the page into
		// move page in to empty frame
		movePageIn(pid, action.page, frame);
	}
	// update page table for replacement algorithm
	updatePageEntry(pid, action);
    if (processTable[pid].valid && processTable[pid].pageTable != NULL) {
        pageTableEntry_t* pageEntry = &processTable[pid].pageTable[action.page];
        pageEntry->referenced = TRUE;
    }
	return frame;
}

Boolean createPageTable(unsigned pid)
/* Create and initialise the page table	for the given process									*/
/* Information on max. process size must be already stored in the PCB		*/
/* Returns TRUE on success, FALSE otherwise									*/
#pragma warning( push )				// store current settings
#pragma warning( disable : 6386 )	// disable buffer overflow warning, which is thrown without actual threat
{
	pageTableEntry_t* pTable = NULL;
	// create and initialise the page table of the process
	pTable = malloc(processTable[pid].size * sizeof(pageTableEntry_t));
	if (pTable == NULL) return FALSE;
	// initialise the page table
	for (unsigned i = 0; i < processTable[pid].size; i++)
	{
		pTable[i].present = FALSE;
		pTable[i].frame = NONE;
		pTable[i].swapLocation = NONE;
	}
	processTable[pid].pageTable = pTable;
	return TRUE;
#pragma warning( pop )				// restore unaltered settings
}

Boolean deAllocateProcess(unsigned pid)
/* free the physical memory used by a process, destroy the page table		*/
/* returns TRUE on success, FALSE on error									*/
{
	// iterate the page table and mark all currently used frames as free
	pageTableEntry_t* pTable = processTable[pid].pageTable;
	for (unsigned i = 0; i < processTable[pid].size; i++)
	{
		if (pTable[i].present == TRUE)
		{	// page is in memory, so free the allocated frame
			storeEmptyFrame(pTable[i].frame);	// add to pool of empty frames
			// update the simulation accordingly !! DO NOT REMOVE !!
			sim_UpdateMemoryMapping(pid, (action_t) { deallocate, i }, pTable[i].frame);
		}
	}
	free(processTable[pid].pageTable);	// free the memory of the page table
	processTable[pid].pageTable = NULL;
	return TRUE;
}

int getEmptyFrameCount(void)
/* Returns the current number of empty frames.								*/
/* A return value of -1 indicates an unitialised memoryManager				*/
{
	if (memoryManagerInitialised)
		return emptyFrameCounter;
	else
		return -1;
}


/* ---------------------------------------------------------------- */
/*                Implementation of local helper functions          */

Boolean isPagePresent(unsigned pid, unsigned page)
/* Predicate returning the present/absent status of the page in memory		*/
{
	return processTable[pid].pageTable[page].present;
}

Boolean storeEmptyFrame(int frame)
/* Store the frame number in the data structure of empty frames				*/
/* and update emptyFrameCounter												*/
{
	frameListEntry_t* newEntry = NULL;
	newEntry = malloc(sizeof(frameListEntry_t));
	if (newEntry != NULL)
	{
		// create new entry for the frame passed
		newEntry->next = NULL;
		newEntry->frame = frame;
		if (emptyFrameList == NULL)			// first entry in the list
		{
			emptyFrameList = newEntry;
		}
		else
			// appent do the list
			emptyFrameListTail->next = newEntry;
		emptyFrameListTail = newEntry;
		emptyFrameCounter++;				// one more free frame	
	}
	return (newEntry != NULL);
}

int getEmptyFrame(void)
/* Returns the frame number of an empty frame.								*/
/* A return value of -1 indicates that no empty frame exists. In this case	*/
/* a page replacement algorithm must be called to evict a page and thus 	*/
/* clear one frame															*/
{
	frameListEntry_t* toBeDeleted = NULL;
	int emptyFrameNo = NONE;
	if (emptyFrameList == NULL) return NONE;	// no empty frame exists
	emptyFrameNo = emptyFrameList->frame;	// get number of empty frame
	// remove entry of that frame from the list
	toBeDeleted = emptyFrameList;
	emptyFrameList = emptyFrameList->next;
	free(toBeDeleted);
	emptyFrameCounter--;					// one empty frame less
	return emptyFrameNo;
}

Boolean movePageIn(unsigned pid, unsigned page, unsigned frame)
/* Returns TRUE on success ans FALSE on any error							*/
{
	// copy of the content of the page from secondary memory to RAM not simulated
	// update the page table: mark present, store frame number, clear statistics
	// *** This must not be removed. The statistics is used by other components of the OS ***
	processTable[pid].pageTable[page].frame = frame;	// list in the pageTabele
	processTable[pid].pageTable[page].present = TRUE;	// mark as present 
	// page was just moved in, i.e. is used and not modified: set R-bit, reset M-bit. 
	processTable[pid].pageTable[page].modified = FALSE;
	processTable[pid].pageTable[page].referenced = TRUE;
	// Statistics for advanced replacement algorithms need to be reset here also
	// *** This must be extended for advences page replacement algorithms ***
	
	// NEW: Update aging-specific logic: set the MSB in the age field
	processTable[pid].pageTable[page].age |= 0x80;

	// update the simulation accordingly !! DO NOT REMOVE !!
	sim_UpdateMemoryMapping(pid, (action_t) { allocate, page }, frame);
	return TRUE;
}

Boolean movePageOut(unsigned pid, unsigned page, int frame)
/* Creates an empty frame at the given location.							*/
/* Copies the content of the frame occupid by the given page to secondary	*/
/* storage.																	*/
/* The exact location in seocondary storage is found and alocated by		*/
/* this function.															*/
/* The page table entries are updated to reflect that the page is no longer */
/* present in RAM, including its location in seondary storage				*/
/* Returns TRUE on success and FALSE on any error							*/
{
	// allocation of secondary memory storage location and copy of page are ommitted for this simulation
	// no distinction between clean and dirty pages made at this point
	// update the page table: mark absent, add frame to pool of empty frames
	// *** This must be extended for advences page replacement algorithms ***
	processTable[pid].pageTable[page].present = FALSE;

	// NEW: Update aging-specific logic: reset the age field
	processTable[pid].pageTable[page].age = 0;

	storeEmptyFrame(frame);	// add to pool of empty frames
	// update the simulation accordingly !! DO NOT REMOVE !!
	sim_UpdateMemoryMapping(pid, (action_t) { deallocate, page }, frame);
	return TRUE;
}

Boolean updatePageEntry(unsigned pid, action_t action)
/* updates the data relevant for page replacement in the page table entry,	*/
/* e.g. set reference and modify bit.										*/
/* In this simulation this function has to cover also the required actions	*/
/* nornally done by hardware, i.e. it summarises the actions of MMU and OS  */
/* when accessing physical memory.											*/
/* Returns TRUE on success ans FALSE on any error							*/
// *** This must be extended for advences page replacement algorithms ***
{
	pageTableEntry_t* pageEntry = &processTable[pid].pageTable[action.page];

	// NEW: set age according to reference and modification bit
	pageEntry->age >>= 1;
	pageEntry->age |= (pageEntry->referenced << 7); // NEW: Assuming 8-bit age field
	// NEW: reset age related bits
	pageEntry->referenced = FALSE;



	return TRUE;
}


Boolean pageReplacement(unsigned* outPid, unsigned* outPage, int* outFrame)
/* ===== The page replacement algorithm								======	*/
/* In the initial implementation the frame to be cleared is chosen			*/
/* globaly and randomly, i.e. a frame is chosen at random regardless of the	*/
/* process that is currently usigt it.										*/
/* The values of pid and page number passed to the function may be used by  */
/* local replacement strategies */
/* OUTPUT: */
/* The frame number, the process ID and the page currently assigned to the	*/
/* frame that was chosen by this function as the candidate that is to be	*/
/* moved out and replaced by another page is returned via the call-by-		*/
/* reference parameters.													*/
/* Returns TRUE on success and FALSE on any error							*/
{
	Boolean found = FALSE;
	unsigned minPid, minPage;
	int minFrame = -1;
	unsigned minAge = UINT_MAX;

	// NEW: iterate over all processes
	for (unsigned pid = 1; pid <= MAX_PROCESSES; pid++)
	{
		if (processTable[pid].valid && processTable[pid].pageTable != NULL)
		{
			// NEW: iterate over all pages
			for (unsigned page = 0; page < processTable[pid].size; page++)
			{
				pageTableEntry_t* pageEntry = &processTable[pid].pageTable[page];

				// NEW: find 'oldest' page
				if (!pageEntry->referenced && pageEntry->age < minAge)
				{
					// NEW: update parameters
					minAge = pageEntry->age;
					minPid = pid;
					minPage = page;
					minFrame = pageEntry->frame;
					found = TRUE;
				}
			}
		}
	}

	// Update the output parameters
	*outPid = minPid;
	*outPage = minPage;
	*outFrame = minFrame;

	return found;
}

//TODO: 
// - find out why frame outputs as 429496295 
//				-> SOLVED
// - find more suitable size for updatePageEntry 
//				-> SOLVED? a�ging wurde auf 8-Bitreduziert in table
// - add output that respresents the aging table from the lectures		
//				-> YES, need to do!
// - maybe also track pagefaults as a counter? does that make sense?
//				-> idk
// - timereventhandler has to be adjusted
//				-> SOLVED
// - hopefully, making it work! c: 
//				-> it does work :D we just need to make it pretty ;)