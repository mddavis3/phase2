/* ------------------------------------------------------------------------
   phase2.c

   University of Arizona South
   Computer Science 452

   ------------------------------------------------------------------------ */

#include <phase1.h>
#include <phase2.h>
#include <usloss.h>

#include "message.h"

#include <stdlib.h>                    //added by Michael for test00
#include <strings.h>                   //added by Michael for test00
#include <stdio.h>                     //added by Michael for test00
#include <string.h>                    //added by Michael for test00

/* ------------------------- Prototypes ----------------------------------- */
int start1 (char *);
extern int start2 (char *);
static void enableInterrupts(void);    //added by Michael for test00
void disableInterrupts(void);          //added by Michael for test00
int check_io(void);                    //added by Michael for test00


/* -------------------------- Globals ------------------------------------- */

int debugflag2 = 0;

/* the mail boxes */
mail_box MailBoxTable[MAXMBOX];


/* -------------------------- Functions ----------------------------------- */

/* ------------------------------------------------------------------------
   Name - start1
   Purpose - Initializes mailboxes and interrupt vector.
             Start the phase2 test process.
   Parameters - one, default arg passed by fork1, not used here.
   Returns - one to indicate normal quit.
   Side Effects - lots since it initializes the phase2 data structures.
   ----------------------------------------------------------------------- */
int start1(char *arg)
{
   int kid_pid, status;

   if (DEBUG2 && debugflag2)
      console("start1(): at beginning\n");

   //check_kernel_mode("start1");

   /* Disable interrupts */
   disableInterrupts();

   /* Initialize the mail box table, slots, & other data structures.
    * Initialize int_vec and sys_vec, allocate mailboxes for interrupt
    * handlers.  Etc... */

   enableInterrupts();

   /* Create a process for start2, then block on a join until start2 quits */
   if (DEBUG2 && debugflag2)
   {
      console("start1(): fork'ing start2 process\n");
   }

   kid_pid = fork1("start2", start2, NULL, 4 * USLOSS_MIN_STACK, 1);

   if ( join(&status) != kid_pid ) 
   {
      console("start2(): join returned something other than start2's pid\n");
   }
   
   
   return 0;
} /* start1 */


/* ------------------------------------------------------------------------
   Name - MboxCreate
   Purpose - gets a free mailbox from the table of mailboxes and initializes it 
   Parameters - maximum number of slots in the mailbox and the max size of a msg
                sent to the mailbox.
   Returns - -1 to indicate that no mailbox was created, or a value >= 0 as the
             mailbox id.
   Side Effects - initializes one element of the mail box array. 
   ----------------------------------------------------------------------- */
int MboxCreate(int slots, int slot_size)
{
} /* MboxCreate */


/* ------------------------------------------------------------------------
   Name - MboxSend
   Purpose - Put a message into a slot for the indicated mailbox.
             Block the sending process if no slot available.
   Parameters - mailbox id, pointer to data of msg, # of bytes in msg.
   Returns - zero if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxSend(int mbox_id, void *msg_ptr, int msg_size)
{
} /* MboxSend */


/* ------------------------------------------------------------------------
   Name - MboxReceive
   Purpose - Get a msg from a slot of the indicated mailbox.
             Block the receiving process if no msg available.
   Parameters - mailbox id, pointer to put data of msg, max # of bytes that
                can be received.
   Returns - actual size of msg if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxReceive(int mbox_id, void *msg_ptr, int msg_size)
{
} /* MboxReceive */

/* --------------------------------------------------------------------------------
   Name - enableInterrupts()
   Purpose - Enables the interrupts.
   Parameters - None
   Returns - Nothing
   ---------------------------------------------------------------------------------*/
static void enableInterrupts()
{
   psr_set((psr_get() | PSR_CURRENT_INT));
}  /*enableInterrupts*/

/* -------------------------------------------------------------------------
   Name - disableInterrupts()
   Purpose - Disables the interrupts.
   Parameters - None
   Returns - Nothing
   --------------------------------------------------------------------------*/
void disableInterrupts()
{
  /* turn the interrupts OFF iff we are in kernel mode */
  if((PSR_CURRENT_MODE & psr_get()) == 0) {
    //not in kernel mode
    console("Kernel Error: Not in kernel mode, may not disable interrupts\n");
    halt(1);
  } else
    /* We ARE in kernel mode */
    psr_set( psr_get() & ~PSR_CURRENT_INT );
} /* disableInterrupts */

/* -------------------------------------------------------------------------
   Name - check_io()
   Purpose - Checks for input/output.
   Parameters - None
   Returns - An integer.  Returns 0 for now, as a dummy function.
   --------------------------------------------------------------------------*/
int check_io()
{
   return 0;
} /* check_io */