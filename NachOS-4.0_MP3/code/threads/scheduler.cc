// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
//*****************************************************************//
//	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//*****************************************************************//
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
//    readyList = new List<Thread *>; 
    L1 = new SortedList<Thread *>(SJFSort); 
    L2 = new SortedList<Thread *>(PrioritySort); 
    L3 = new List<Thread *>; 
    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
//    delete readyList; 
	delete L1; 
	delete L2; 
	delete L3; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
	//How do I print Ticks if I don't have kernel object??? Actually, we do have the object.
    thread->setStatus(READY);
//    readyList->Append(thread);
	if(thread->getLevel()==3){
		L3->Append(thread);
		insertLevel = 3;
		DEBUG(dbgPri, "Tick " << kernel->stats->totalTicks << ": Thread " << thread->getID() << " is inserted into queue L3");
	}
	else if(thread->getLevel()==2){
		L2->Insert(thread);
		insertLevel = 2;
		DEBUG(dbgPri, "Tick " << kernel->stats->totalTicks << ": Thread " << thread->getID() << " is inserted into queue L2");
	}
	else{
		if(thread->getPriority()>=150) std::cout << "Priority out of range!!!" << std::endl;
		L1->Insert(thread);
		insertLevel = 1;
		DEBUG(dbgPri, "Tick " << kernel->stats->totalTicks << ": Thread " << thread->getID() << " is inserted into queue L1");
	}
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------
//TO DO
Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
//    kernel->scheduler->Print();
	/*
    if (readyList->IsEmpty()) {
		return NULL;
    } else {
    	return readyList->RemoveFront();
    }
	*/
	Thread * Tar;
    if(!L1->IsEmpty()){
	Tar = L1->RemoveFront();
	DEBUG(dbgPri, "Tick " << kernel->stats->totalTicks << ": Thread " << Tar->getID() << " is removed from queue L1");
	return Tar;
    }
    else if(!L2->IsEmpty()){
	Tar = L2->RemoveFront();
	DEBUG(dbgPri, "Tick " << kernel->stats->totalTicks << ": Thread " << Tar->getID() << " is removed from queue L2");
	return Tar;
    }
    else if(!L3->IsEmpty()){
	Tar = L3->RemoveFront();
	DEBUG(dbgPri, "Tick " << kernel->stats->totalTicks << ": Thread " << Tar->getID() << " is removed from queue L3");
	return Tar;
    }
    else{
	return NULL;
    }
}
Thread *
Scheduler::GetNextToRun()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    if(!L1->IsEmpty()){
	return L1->Front();
    }
    else if(!L2->IsEmpty()){
	return L2->Front();
    }
    else if(!L3->IsEmpty()){
	return L3->Front();
    }
    else return NULL;
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    nextThread->Stime = kernel->stats->totalTicks; //Set Stime
 
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
 
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    DEBUG(dbgPri, "Tick " << kernel->stats->totalTicks << ": Thread " << nextThread->getID() << " is now selected for execution, thread " << oldThread->getID() << " is replaced, and it has executed " << oldThread->Bursttime << " ticks");
    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
 
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
    currentLevel = kernel->currentThread->getLevel();
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
//    readyList->Apply(ThreadPrint);
    cout << "L1:";
	L1->Apply(ThreadPrint);
    cout << "\nL2:";
	L2->Apply(ThreadPrint);
    cout << "\nL3:";
	L3->Apply(ThreadPrint);
    cout << "\n";
}
void
Scheduler::UpdateWtime(){
	//CurrentThread is not in ReadyQueue
	/*
	auto& it = L1.begin();
	for(it = L1.begin();it!= L1.end();it++){it->Aging();}
	for(it = L2.begin();it!= L2.end();it++){it->Aging();}
	for(it = L3.begin();it!= L3.end();it++){it->Aging();}
	*/
	L1->Apply(ThreadAging);
	L2->Apply(ThreadAging);
	L3->Apply(ThreadAging);
}
