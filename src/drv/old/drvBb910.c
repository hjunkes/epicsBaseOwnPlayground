/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* bb910_driver.c */
/* base/src/drv $Id$ */
/*
 * subroutines that are used to interface to the binary input cards
 *
 * 	Author:      Bob Dalesio
 * 	Date:        6-13-88
 */

/*
 * Code Portions:
 *
 * bi_driver_init  Finds and initializes all binary input cards present
 * bi_driver       Interfaces to the binary input cards present
 */


#include <vxWorks.h>
#include <stdlib.h>
#include <stdio.h>
#include <vxLib.h>
#include <sysLib.h>
#include <vme.h>
#include <module_types.h>
#include <drvSup.h>

static long report();
static long init();

struct {
        long    number;
        DRVSUPFUN       report;
        DRVSUPFUN       init;
} drvBb910={
        2,
        report,
        init};

static long report(level)
    int level;
{

    bb910_io_report(level);
    return(0);
}
static long init()
{

    bb910_driver_init();
    return(0);
}

static char SccsId[] = "@(#)drvBb910.c	1.6\t6/3/93";

#define MAX_BB_BI_CARDS	(bi_num_cards[BB910]) 

/* Burr-Brown 910 binary input memory structure */
/* Note: the high and low order words are switched from the io card */
struct bi_bb910{
        unsigned short  csr;            /* control status register */
        unsigned short  low_value;      /* low order 16 bits value */
        unsigned short  high_value;     /* high order 16 bits value */
        char    end_pad[0x100-6];       /* pad until next card */
};

/* pointers to the binary input cards */
struct bi_bb910 **pbi_bb910s;      /* Burr-Brown 910s */

/* test word for forcing bi_driver */
int	bi_test;

static char *bb910_shortaddr;


/*
 * BI_DRIVER_INIT
 *
 * intialization for the binary input cards
 */

int bb910_driver_init(){
	int bimode;
        int status;
        register short  i;
        struct bi_bb910        *pbi_bb910;

	pbi_bb910s = (struct bi_bb910 **) 
		calloc(MAX_BB_BI_CARDS, sizeof(*pbi_bb910s));
	if(!pbi_bb910s){
		return ERROR;
	}

        /* intialize the Burr-Brown 910 binary input cards */
        /* base address of the burr-brown 910 binary input cards */

        status=sysBusToLocalAdrs(VME_AM_SUP_SHORT_IO,bi_addrs[BB910],&bb910_shortaddr);
        if (status != OK){ 
           printf("Addressing error in bb910 driver\n");
           return ERROR;
        }
        pbi_bb910 = (struct bi_bb910 *)bb910_shortaddr;

        /* determine which cards are present */
        for (i = 0; i < bi_num_cards[BB910]; i++,pbi_bb910++){
            if (vxMemProbe(pbi_bb910,READ,sizeof(short),&bimode) == OK){
                pbi_bb910s[i] = pbi_bb910;
            }
            else {
                pbi_bb910s[i] = 0;
            }
        }
        
        return (0);

}


int bb910_driver(card,mask,prval)
       
	register unsigned short card;
	unsigned int            mask;
	register unsigned int	*prval;
{
	register unsigned int	work;

	if (!pbi_bb910s[card])
       		return (-1);

        /* read */

        work = (pbi_bb910s[card]->high_value << 16)    /* high */
        	+ pbi_bb910s[card]->low_value;             /* low */
	/* apply mask */
	*prval = work & mask;

	return (0);             
   }

#define masks(K) ((1<<K))
void bb910_io_report(level)
  short int level;
 { 
   register short i,j,k,l,m,num_chans;
   unsigned int jval,kval,lval,mval;

   for (i = 0; i < bi_num_cards[BB910]; i++){
	if (pbi_bb910s[i]){
           printf("BI: BB910:      card %d\n",i);
           if (level > 0){
               num_chans = bi_num_channels[BB910];
               for(j=0,k=1,l=2,m=3;j < num_chans,k < num_chans, l < num_chans,m < num_chans;
                   j+=IOR_MAX_COLS,k+= IOR_MAX_COLS,l+= IOR_MAX_COLS,m += IOR_MAX_COLS){
        	if(j < num_chans){
                        bb910_driver(i,masks(j),BB910,&jval);
                 	if (jval != 0) 
                  		 jval = 1;
                         printf("Chan %d = %x\t ",j,jval);
                }  
         	if(k < num_chans){
                        bb910_driver(i,masks(k),BB910,&kval);
                        if (kval != 0) 
                        	kval = 1;
                        printf("Chan %d = %x\t ",k,kval);
                }
                if(l < num_chans){
                        bb910_driver(i,masks(l),BB910,&lval);
                	if (lval != 0) 
                        	lval = 1;
                	printf("Chan %d = %x \t",l,lval);
                 }
                  if(m < num_chans){
                        bb910_driver(i,masks(m),BB910,&mval);
                 	if (mval != 0) 
                        	mval = 1;
                 	printf("Chan %d = %x \n",m,mval);
                   }
             }
           }
        }
   }     
 }
