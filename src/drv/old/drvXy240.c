/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* xy240_driver.c */
/* base/src/drv $Id$ */
/*
 *	routines used to test and interface with Xycom240
 *	digital i/o module
 *
 * 	Author:      Bob Dalesio
 * 	Date:        11/20/91
 */

#include "vxWorks.h"
#include "taskLib.h"
#include "vme.h"
#include "module_types.h"
#include "task_params.h"
#include <drvSup.h>
#include <dbDefs.h>
#include <dbScan.h>
#include <taskwd.h>

#define XY240_ADDR0	(bi_addrs[XY240_BI])
#define XY240_MAX_CARDS	(bi_num_cards[XY240_BI])
#define XY240_MAX_CHANS	(bi_num_channels[XY240_BI])

#define masks(K) ((1<<K))

/*xy240 memory structure*/
struct dio_xy240{
        char            begin_pad[0x80];        /*go to interface block*/
        unsigned short  csr;                    /*control status register*/
        unsigned short  isr;                    /*interrupt service routine*/
        unsigned short  iclr_vec;               /*interrupt clear/vector*/
        unsigned short  flg_dir;                /*flag&port direction*/
        unsigned short  port0_1;                /*port0&1 16 bits value*/
        unsigned short  port2_3;                /*por2&3 16 bits value*/
        unsigned short  port4_5;                /*port4&5 16 bits value*/
        unsigned short  port6_7;                /*port6&7 16 bits value*/
        char            end_pad[0x400-0x80-16]; /*pad til next card*/
};

/*create dio control structure record*/
struct dio_rec
        {
        struct dio_xy240 *dptr;                 /*pointer to registers*/
        short num;                              /*device number*/
        short mode;                             /*operating mode*/
        unsigned short sport0_1;                /*saved inputs*/
        unsigned short sport2_3;                /*saved inputs*/
        IOSCANPVT ioscanpvt;
        /*short dio_vec;*/                      /*interrupt vector*/
        /*unsigned int intr_num;*/              /*interrupt count*/
        };


LOCAL
struct dio_rec *dio;	/*define array of control structures*/

static long report();
static long init();

struct {
        long    number;
        DRVSUPFUN       report;
        DRVSUPFUN       init;
} drvXy240={
        2,
        report,
        init};
static long report(level)
    int level;
{
    xy240_io_report(level);
    return(0);
}

void xy240_bi_io_report(int card);
void xy240_bo_io_report(int card);

static long init()
{

    xy240_init();
    return(0);
}

/*dio_int
 *
 *interrupt service routine
 *
 */
#if 0
dio_int(ptr)
 register struct dio_rec *ptr;
{
 register struct dio_xy240 *regptr;

 regptr = ptr->dptr;

}
#endif

/*
 *
 *dio_scan
 *
 *task to check for change of state
 *
 */
int dio_scan()
 
{	
	int i;
	int	first_scan,first_scan_complete;

 for(;;){
    if(interruptAccept) break;
    taskDelay(vxTicksPerSecond/30);
 }
 first_scan_complete = FALSE;
 first_scan = TRUE;
 for (;;) 
 {
   for (i = 0; i < XY240_MAX_CARDS; i++)
    {
    if (dio[i].dptr)
     if (((dio[i].dptr->port0_1) ^ (dio[i].sport0_1)) || 
		((dio[i].dptr->port2_3) ^ (dio[i].sport2_3))
		|| first_scan)
      {
	 /* printf("io_scanner_wakeup for card no %d\n",i); */
          scanIoRequest(dio[i].ioscanpvt);
	  dio[i].sport0_1 = dio[i].dptr->port0_1;
	  dio[i].sport2_3 = dio[i].dptr->port2_3;	  
	  }
    }
    if (first_scan){
	first_scan = 0;
	first_scan_complete = 1;
    }
    taskDelay(vxTicksPerSecond/30);	
  }
}



/*DIO DRIVER INIT
 *
 *initialize xy240 dig i/o card
 */
int xy240_init()
{
	short 			junk;
	register short 		i;
	struct dio_xy240	*pdio_xy240;
	int 			tid;
	int			status;
	int			at_least_one_present = FALSE;

	/*
	 * allow for runtime reconfiguration of the
	 * addr map
	 */
	dio = (struct dio_rec *) calloc(XY240_MAX_CARDS, sizeof(*dio));
	if(!dio){
		return ERROR;
	}

	status = sysBusToLocalAdrs(VME_AM_SUP_SHORT_IO,XY240_ADDR0,&pdio_xy240);
        if (status != OK){
           	printf("%s: Unable to map the XY240 A16 base addr\n", __FILE__);
           	return ERROR;
        }

        for (i = 0; i < XY240_MAX_CARDS; i++, pdio_xy240++){
	  
		if (vxMemProbe(pdio_xy240,READ,2,&junk) != OK){
	   		dio[i].dptr = 0;
    			continue;
		} 

		/*
		 * 	register initialization
		 */
		pdio_xy240->csr = 0x3;
		pdio_xy240->iclr_vec = 0x00;	/*clear interrupt input register latch*/
		pdio_xy240->flg_dir = 0xf0;	/*ports 0-3,input;ports 4-7,output*/
		dio[i].sport2_3 = pdio_xy240->port2_3;	/*read and save high values*/
                dio[i].dptr = pdio_xy240;
		at_least_one_present = TRUE;
                scanIoInit(&dio[i].ioscanpvt);
	}
				
 	if (at_least_one_present) 
  	{
   		if ((tid = taskNameToId(XY240_NAME)) != ERROR){
			taskwdRemove(tid);
   			taskDelete(tid);
		}

		status = taskSpawn(
			XY240_NAME,
			XY240_PRI,
			XY_240_OPT,
			XY_240_STACK,
			dio_scan);
   		if (status == ERROR){
  			printf("Unable to create XY240 scan task\n");
		}
		else taskwdInsert(status,NULL,NULL);
  	}



	return OK;

} 	

int xy240_getioscanpvt(card,scanpvt)
short 		card;
IOSCANPVT 	*scanpvt;
{
        if ((card >= XY240_MAX_CARDS) || (!dio[card].dptr)) return(0);
        *scanpvt = dio[card].ioscanpvt;
        return(0);
}


/*
 * XY240_BI_DRIVER
 *
 *interface to binary inputs
 */

int xy240_bi_driver(card,mask,prval)
register short 		card;
unsigned int		mask;
register unsigned int	*prval;
{
	register unsigned int	work;

	if ((card >= XY240_MAX_CARDS) || (!dio[card].dptr)) 
	 return -1;
	work = (dio[card].dptr->port0_1 << 16)
	 + dio[card].dptr->port2_3;
	*prval = work & mask;

	return(0);
}

/*
 *
 *XY240_BO_READ
 *
 *interface to binary outputs
 */

int xy240_bo_read(card,mask,prval)
register short 		card;
unsigned int		mask;
register unsigned int	*prval;
{
	register unsigned int	work;
 
	if ((card >= XY240_MAX_CARDS) || (!dio[card].dptr)){ 
		 return -1;
        }
              
	/* printf("%d\n",dio[card].num); */
	work = (dio[card].dptr->port4_5 << 16)
	 + dio[card].dptr->port6_7;
                 		
	*prval = work &= mask;

        return(0);
 }

/* XY240_DRIVER
 *
 *interface to binary outputs
 */

int xy240_bo_driver(card,val,mask)
register short 		card;
unsigned int		mask;
register unsigned int	val;
{
	register unsigned int	work;

 	if ((card >= XY240_MAX_CARDS) || (!dio[card].dptr)) 
		 return ERROR;

	/* use structure to handle high and low short swap */
	/* get current output */

	work = (dio[card].dptr->port4_5 << 16)
			 + dio[card].dptr->port6_7;

	work = (work & ~mask) | (val & mask);

	dio[card].dptr->port4_5 = (unsigned short)(work >> 16);
	dio[card].dptr->port6_7 = (unsigned short)work;

	return OK;
 }


/*dio_out
 *
 *test routine for xy240 output 
 */
int dio_out(card,port,val)
short	card,port,val;
{

 if ((card > XY240_MAX_CARDS-1)) 		/*test to see if card# is allowable*/
  {
  printf("card # out of range\n");
  return -1;
  }
 else if (!dio[card].dptr)				/*see if card exists*/
  {
  printf("can't find card %d\n", card);
  return -2;
  }
 else if ((port >7) || (port <4))			/*make sure they're output ports*/
  {
  printf("use ports 4-7\n");
  return -3;
  }
 else if (port == 4)
  {
  dio[card].dptr->port4_5 = val<< 8;
  return -4;
  }
 else if (port == 5)
  {
  dio[card].dptr->port4_5 = val;
  return -5;
  }
 else if (port == 6)
  {
  dio[card].dptr->port6_7 = val<< 8;
  return -6;
  }
else if (port == 7)
  {
  dio[card].dptr->port6_7 = val;
  return -7;
  }
else{
  printf("use ports 4-7\n");
  return -8;
  }
}
 
/*XY240_WRITE
 *
 *command line interface to test bo driver
 *
 */
int xy240_write(card,val)
        short card;
        unsigned int val;
 {
  return xy240_bo_driver(card,val,0xffffffff);
 }
 


long
xy240_io_report(level)
short int level;
{
  	int card;

        for (card = 0; card < XY240_MAX_CARDS; card++){
        
                if(dio[card].dptr){
                   printf("B*: XY240:\tcard %d\n",card);
                   if (level >= 1){
			xy240_bi_io_report(card);
			xy240_bo_io_report(card);
                   }
                }
        }
        return(0); 
}

                          
void xy240_bi_io_report(int card)
{
        short int num_chans,j,k,l,m;
        int jval,kval,lval,mval;

        num_chans = XY240_MAX_CHANS;

        if(!dio[card].dptr){
		return;
	}

	printf("\tXY240 BINARY IN CHANNELS:\n");
	for(	j=0,k=1,l=2,m=3;
		j < num_chans,k < num_chans, l< num_chans, m < num_chans; 
		j+=IOR_MAX_COLS,k+= IOR_MAX_COLS,l+= IOR_MAX_COLS, m += IOR_MAX_COLS){
                         

		if(j < num_chans){
			xy240_bi_driver(card,masks(j),&jval);
			if (jval != 0) 
				jval = 1;
			printf("\tChan %d = %x\t ",j,jval);
		}
		if(k < num_chans){
			xy240_bi_driver(card,masks(k),&kval);
			if (kval != 0) 
				kval = 1;
			printf("Chan %d = %x\t ",k,kval);
		}
		if(l < num_chans){
			xy240_bi_driver(card,masks(l),&lval);
			if (lval != 0) 
				lval = 1;
			printf("Chan %d = %x \t",l,lval);
		}
		if(m < num_chans){
			xy240_bi_driver(card,masks(m),&mval);
			if (mval != 0) 
				mval = 1;
			printf("Chan %d = %x \n",m,mval);
		}
	}
}


void xy240_bo_io_report(int card)
{
        short int num_chans,j,k,l,m;
        int jval,kval,lval,mval;

        num_chans = XY240_MAX_CHANS;

	if(!dio[card].dptr){
		return;
	}

	printf("\tXY240 BINARY OUT CHANNELS:\n");

	for(	j=0,k=1,l=2,m=3;
		j < num_chans,k < num_chans, l < num_chans,m < num_chans; 
		j+=IOR_MAX_COLS,k+= IOR_MAX_COLS,l+= IOR_MAX_COLS, m += IOR_MAX_COLS){

		if(j < num_chans){
			xy240_bo_read(card,masks(j),&jval);
			if (jval != 0) 
				jval = 1; 
			printf("\tChan %d = %x\t ",j,jval);
		}
		if(k < num_chans){
			xy240_bo_read(card,masks(k),&kval);
			if (kval != 0) 
				kval = 1; 
 			printf("Chan %d = %x\t ",k,kval);
		}
		if(l < num_chans){
			xy240_bo_read(card,masks(l),&lval);
			if (lval != 0) 
				lval = 1; 
			printf("Chan %d = %x \t",l,lval);
		}
		if(m < num_chans){
			xy240_bo_read(card,masks(m),&mval);
			if (mval != 0) 
				mval = 1; 
			printf("Chan %d = %x \n",m,mval);
		}
	}
}

