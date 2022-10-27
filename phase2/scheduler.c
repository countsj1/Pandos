#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../Phase2/initial.c"
#include "/usr/include/umps3/umps/libumps.h"

/* global variables maintaining time usage*/
cpu_t TODStarted;
cpu_t currentTOD;

/* global variables from initial.c */
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;

/*We need to implement a clock...*/

void scheduler() {

    state_t iniState;
    pcb_PTR p;

    if(emptyProcQ(readyQue))
    {
        if(procssCnt == 0)
        {
            HALT();
        }
        else
        {
            if(softBlockCnt != 0)
            {
                
                currentProc = NULL;

                setTimer(DISABLE);

                iniState = ALLOFF | IEPON | IMON | TEBITON;
                Load_State(iniState);
                finalMSG("", FALSE);
            }
            else
            {
                finalMSG("", TRUE);
            }
        }
    }

    Move_Process(p);

    /*Load 5ms on PLT*/
    setTimer(TIMESLICE); /* Time slice is 5ms */
    myLDST(currentProc) /*Load Processor State*/
}

switch_State(state_PTR currentProccess)
{
    STCK(intervaltimer);
    setTimer(pseudoClockSema4);


    currentProc.p_s = currentProccess;
    myLDST(currentProc);
}

myLDST(pcb_t *currProc){
    proc = currProc;
    LDST(&(currProc->p_s));

/* Stealing this idea from Mikey. It seemed cool */
finalMSG(char msg[], bool Bstatus)
{
    if(Bstatus)
    {
        PANIC();
    }
    if(Bstatus == FALSE)
    {
        WAIT();
    }
    printf(char);
}

Move_Process(pcb_PTR p)
{
    removeProcQ(readyQue);
    insertChild(currentProc, p);
}
