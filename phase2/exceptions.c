#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/exceptions.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "/usr/include/umps3/umps/libumps.h"

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

     Load_State(p);
     PANIC();
}

/*SYS1*/
void Create_ProcessP()
{
    /*Initialize fields of p*/
    pcb_t *p;
    p->p_s = p->p_s.s_a1;
    p->p_supportStruct = s_a2;
    insertProcQ(readyQue, p);
    insertChild(currentProc, p);
    p->p_time = 0;
    p->p_semAdd = NULL;
}

/* Sys2 */
void Terminate_Process()
{
    if(emptyChild(currentProc)){
        /* current process has no children */
        outChild(currentProc);
        freePcb(currentProc);
        --processCnt;
    } else{

    }

    /* no current process anymore */
    currentProc = NULL;
    scheduler();
}

/* Sys3 Passeren*/
pcb_t* wait(sema4)
{
    sema4--;
    if(sema4 < 0){
        pcb_t *p = removeProcQ(&(sema4));
        insertBlocked(&sema4, currentProc);
    }
    BlockedSYS(p);
    return currentProc;
}

/* Sys4 Verhogen */
pcb_t* signal(sema4)
{
    sema4++;
    if(sema4 <= 0){
        pcb_PTR temp = removeBlocked(&sema4);
        insertProcQ(&readyQue, temp);
    }
    return currentProc;
}

/* Sys5 */
void Wait_for_IO_Device()
{
    BlockedSYS(currentProc);
    SYSCALL(PASSEREN, iosema4, 0, 0);
    /*Need to handle subdevices*/
}

/* Sys6 */
int Get_CPU_Time(pcb_t p)
{
    accumulatedTime = currentProc.p_time;
}

/* Sys7 */
void Wait_For_Clock()
{
    /* Define pseudoClockSema4 */
    wait(pseudoClockSema4);
    /*Do this every 100ms*/
    signal(pseudoClockSema4);
    /********************/
    BlockedSYS(currentProc);
}

/* Sys8 */
void Get_SUPPORT_Data()
{
    return currentProc->p_supportStruct;
}

/*Used for syscalls that block*/
void BlockedSYS(pcb_t p)
{
    /*Refer to 3.5.11 to complete this code */
    p.p_s.s_pc = p.p_s.s_pc + 4;
    p->p_s.s_status = ALLOFF | IEPON | IMON | TEBITON;
    p->p_time = p->p_time + intervaltimer;
    insertBlocked(currentProc);
    scheduler();
    /****************************************/
}