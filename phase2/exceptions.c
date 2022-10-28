#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
#include "/usr/include/umps3/umps/libumps.h"

/********************************** Exception Handling ****************************
 *
 *   WIP!
 * 
 *   Authors:
 *      Ronnie Cole
 *      Joe Pinkerton
 *      Joseph Counts
*/

/* global variables from initial.c */
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;
extern int deviceSema4s[MAXDEVICECNT];

void SYSCALL(SYSNUM) 
{
    switch(SYSNUM) 
    {
        case CREATEPROCESS:
            Create_ProcessP();
        case TERMINATEPROCESS:
            Terminate_Process();
        case PASSEREN:
            wait(sema4);
        case VERHOGEN:
            signal(sema4);
        case WAITIO:
            Wait_for_IO_Device();
        case GETCPUTIME:
            Get_CPU_Time(p);
        case WAITCLOCK:
            Wait_For_Clock();
        case GETSUPPORTPRT:
            void Get_SUPPORT_Data();
    }
    if(SYSNUM > GETSUPPORTPRT) {
        passUpOrDie(currentProc, GENERALEXCEPT);
    }
}

/* System Call 1: When called, a newly populated pcb is placed on the Ready Queue and made a child
of the Current Process. pcb_t p is the name for this new process, and its fields obtained from registers a1 and a2.
Its cpu time is initialized to 0, and its semaphore address is set to NULL since it is in the ready
state. */
void Create_ProcessP()
{
    /* Initialize fields of p */
    pcb_t *p;
    p->p_s = s_a1;
    p->p_supportStruct = s_a2;
    /* Make p a child of currentProc and also place it on the ReadyQueue */
    insertProcQ(readyQue, p);
    insertChild(currentProc, p);
    processCnt++;
    /* Set cpu time to 0 and semAdd to NULL */
    p->p_time = 0;
    p->p_semAdd = NULL;
}

/* System Call 2: When called, the executing process and all its progeny are terminated. */
void Terminate_Process()
{
    pcb_PTR temp = currentProc;
    /* If currentProc has a child... */
    if(emptyChild(temp) == FALSE){
        /* Go to that child... */
        temp = temp->p_child;
        /* and then go to its sibling */
        if(temp->p_sibn != NULL) {
            temp = temp->p_sibn;
        }
        /* Call Terminate Process again until currentProc has no children */
        Terminate_Process();
    } else {
        /* If the CurrentProc has no children, we can remove it */
        freePcb(temp);
        temp = NULL;
        --processCnt;
        scheduler();
    }
}

/* System Call 3: Preforms a "P" operation or a wait operation. The semaphore is decremented
and then blocked.*/
pcb_t *wait(sema4)
{
    sema4--;
    if(sema4 < 0){
        pcb_t *p = removeProcQ(&(sema4));
        insertBlocked(&sema4, currentProc);
    }
    BlockedSYS(p);
}

/* System Call 3: Preforms a "V" operation or a signal operation. The semaphore is incremented
and is unblocked/placed into the ReadyQue.*/
pcb_t *signal(sema4)
{
    sema4++;
    if(sema4 <= 0){
        pcb_PTR temp = removeBlocked(&sema4);
        insertProcQ(&readyQue, temp);
    }
}

/* Sys5 */
void Wait_for_IO_Device()
{
    BlockedSYS(currentProc);
    /*SYSCALL(PASSEREN, iosema4, 0, 0); Need Helper Function here*/
    /*Need to handle subdevices*/
}

/* Sys6 */
int Get_CPU_Time(pcb_t *p)
{
    accumulatedTime = currentProc.p_time;
}

/* Sys7 */
void Wait_For_Clock()
{
    /* Define pseudoClockSema4 */
    pseudoClockSema4--;
    /* Handle this in a helper function */
    if(pseudoClockSema4 < 0)
    {
        pcb_t *p = removeProcQ(&(pseudoClockSema4));
        insertBlocked(&(pseudoClockSema4),p);
    }
    BlockedSYS(currentProc);
}

/* Sys8 */
void Get_SUPPORT_Data()
{
    return currentProc->p_supportStruct;
}

/*Used for syscalls that block*/
void BlockedSYS(pcb_t *p)
{
    p.p_s.s_pc = p.p_s.s_pc + 4;
    p->p_s.s_status = ALLOFF | IEPON | IMON | TEBITON;
    p->p_time = p->p_time + intervaltimer;
    insertBlocked(currentProc);
    scheduler();
}

void programTRPHNDLR() {
    passUpOrDie(currentProc, GENERALEXCEPT);
}

void uTLB_RefillHandler() {
    passUpOrDie(currentProc, PGFAULTEXCEPT);
}

/* Passup Or Die */

void passUpOrDie(pcb_t currProc, int ExeptInt) {
    if(currProc->p_supportStruct == NULL) {
        Terminate_Process();
    }
    if(currProc->p_supportStruct != NULL) {
        passUp(currProc, ExeptInt);
    }
}

void passUp(pcb_t currProc, int ExeptInt) {
    state_PTR tempstate = (state_t *) BIOSDATAPAGE->s_cause & GETEXECCODE;
    currProc->p_supportStruct->sup_exceptState[ExeptInt] = tempstate;
    context_t exceptContext;
    exceptContext.c_stackPtr = currProc->p_supportStruct->sup_exceptState[ExeptInt].c_stackPtr;
    exceptContext.c_status = currProc->p_supportStruct->sup_exceptState[ExeptInt].c_status;
    exceptContext.c_pc = currProc->p_supportStruct->sup_exceptState[ExeptInt].c_pc;
    LDCXT(exceptContext);
}

void uTLB_RefillHandler () {
    setENTRYHI(0x80000000);
    setENTRYLO(0x00000000);
    TLBWR();
    LDST ((state_PTR) 0x0FFFF000);
}