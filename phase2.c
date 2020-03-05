/* ------------------------------------------------------------------------
   phase2.c

   University of Arizona South
   Computer Science 452

   for debug console printouts:
   if (DEBUG2 && debugflag2)
         {
            console ("\n");
         }

   ------------------------------------------------------------------------ */

#include <stdlib.h>                    //added by Michael for test00
#include <strings.h>                   //added by Michael for test00
#include <stdio.h>                     //added by Michael for test00
#include <string.h>                    //added by Michael for test00

#include <phase1.h>
#include <phase2.h>
#include <usloss.h>

#include "message.h"


/* ------------------------- Prototypes ----------------------------------- */
int start1 (char *);
extern int start2 (char *);
static void enableInterrupts(void);    //added by Michael for test00
void disableInterrupts(void);          //added by Michael for test00
int check_io(void);                    //added by Michael for test00
void check_kernel_mode(void);          //added by Michael for start1
void insert_mail_slot(int, int);       //added for MboxSend


/* -------------------------- Globals ------------------------------------- */

int debugflag2 = 0;

/* the mail boxes */
mail_box MailBoxTable[MAXMBOX];
mail_slot MailSlotTable[MAXSLOTS];                          //added by Michael for start1 - REVIEW
mbox_proc MboxProcs[MAXPROC];                               //added by Michael for start1 - REVIEW

/* Mail box id used to keep track of unique mail box ids */ //added by Michael for MboxCreate
int global_mbox_id;

/* Dummy mailbox used for initialization */                 //added by Michael for start1
mail_box dummy_mbox = {0, NULL, NULL, NULL, NULL, NULL, NULL};

/* Dummy mailslot used for initialization */                //added by Michael for start1
mail_slot dummy_slot = {0, NULL, NULL, NULL, NULL};

/* Dummy proc used for initialization */                    //added by Michael for start1
mbox_proc dummy_proc = {NULL};

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
   {
      console("start1(): at beginning\n");
   }

   /* Check for kernel mode */
   check_kernel_mode();

   /* Disable interrupts */
   disableInterrupts();

   /* Initialize the mail box table, slots, & other data structures. - completed
    * Initialize the Process Table - completed
    * Create I/O mailboxes and Initialize int_vec and sys_vec arrays, 
    * allocate mailboxes for interrupt handlers.
    */

   /* Initialize global mbox_id to 0 */
   global_mbox_id = 0;

   /* Initialize the mail box table */
   for (int i = 0; i < MAXMBOX; i++)
   {
      MailBoxTable[i] = dummy_mbox;
   }

   /* Initialize the mail slot table */
   for (int i = 0; i <MAXSLOTS; i++)
   {
      MailSlotTable[i] = dummy_slot;
   }

   /* Initialize the phase2 proc table */
   for (int i = 0; i < MAXPROC; i++)
   {
      MboxProcs[i] = dummy_proc;
   }

   /* Enable interrupts */
   enableInterrupts();

   /* Create a process for start2, then block on a join until start2 quits */
   if (DEBUG2 && debugflag2)
   {
      console("start1(): fork'ing start2 process\n");
   }

   kid_pid = fork1("start2", start2, NULL, 4 * USLOSS_MIN_STACK, 1);

   if (join(&status) != kid_pid) 
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
   /* Check for kernel mode */
   check_kernel_mode();

   int i = 0;

   //check for a free mailbox from the table of mailboxes
   while (MailBoxTable[i].is_free != 0)
   {
      i++;
      //return -1 if no mailboxes are available in mailboxtable
      if (i >= MAXMBOX)
      {
         if (DEBUG2 && debugflag2)
         {
            console ("MboxCreate(): no mailboxes available. Return -1\n");
         }
         return (-1);
      }
   }

   //return -1 if slot_size is incorrect
   //doesn't check for negative slot_size...
   if (slot_size > MAX_MESSAGE)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("MboxCreate(): slot_size is incorrect. Return -1\n");
      }
      return (-1);
   }

   //initialize the mailbox struct
   MailBoxTable[i].is_free = 1;       //indicates that the mailboxtable slot is full
   MailBoxTable[i].status = ACTIVE;   //indicates that the mailbox is active
   MailBoxTable[i].num_slots = slots;
   MailBoxTable[i].max_slot_size = slot_size;
   MailBoxTable[i].num_used_slots = 0;

   //increment global int mbox_id
   //return >= 0 as the mailbox id number
   global_mbox_id++;
   MailBoxTable[i].mbox_id = global_mbox_id;
   return MailBoxTable[i].mbox_id;

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
   /* Check for kernel mode */
   check_kernel_mode();

   int i, j = 0;

   //check for the mailbox in the mailbox table
   while (MailBoxTable[i].mbox_id != mbox_id)
   {
      i++;
      //return -1 if mailbox is not in the mailbox table
      if (i == MAXMBOX)
      {
         if (DEBUG2 && debugflag2)
         {
            console ("MboxSend(): mailbox not found in the mailbox table. Return -1\n");
         }
         return (-1);
      }
   }

   //return -1 if mailbox id is inactive
   //this might be redundant, if it's inactive, it shouldn't be in the table anyway
   if (MailBoxTable[i].status == INACTIVE)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("MboxSend(): mailbox id is inactive. Return -1\n");
      }
      return (-1);
   }

   //return -1 if message size is too large
   if (msg_size > MailBoxTable[i].max_slot_size)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("MboxSend(): message size is too large. Return -1\n");
      }
      return (-1);
   }

   //check to see if there are available mail slots
   //block the sender if there are no available slots
   if (MailBoxTable[i].num_used_slots >= MailBoxTable[i].num_slots)
   {
      //block the sender cus it's full - REVIEW
      block_me(MAILBOXFULL);
   }
   
   //once slot is available, allocate slot from MailSlotTable
   while (MailSlotTable[j].is_free != 0)
   {
      j++;
      //halt(1) overflow of MailSlotTable, no free space
      if (j >= MAXSLOTS)
      {
         if (DEBUG2 && debugflag2)
            {
               console ("MboxSend(): Error. Overflow of MailSlotTable.\n");
            }
         halt(1);
      }
   }

   //initialize the mail_slot struct
   MailSlotTable[j].is_free = 1;
   MailSlotTable[j].mbox_id = MailBoxTable[i].mbox_id;
   memcpy(MailSlotTable[j].message, msg_ptr, msg_size);

   //maintain linked list of mailslots
   insert_mail_slot(i, j);

   //make necessary changes to the mailbox struct
   MailBoxTable[i].num_used_slots++;

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
   /* Check for kernel mode */
   check_kernel_mode();

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

/* -------------------------------------------------------------------------
   Name - check_kernel_mode()
   Purpose - Checks the PSR to see what the current mode is. Halts(1) if
             the current mode is user mode. Returns otherwise.
   Parameters - None
   Returns - Nothing
   --------------------------------------------------------------------------*/
void check_kernel_mode()
{
   // test if in kernel mode; halt if in user mode
   if ((PSR_CURRENT_MODE & psr_get()) == 0)
   {
      console("check_kernel_mode(): Error - current mode is user mode. Halt(1)\n");
      halt(1);
   }

   // return if in kernel mode
   return;
} /* check_kernel_mode */


/* -------------------------------------------------------------------------
   Name - insert_mail_slot()
   Purpose - maintain the linked list of mailslots in the mbox and mailslot
             structs
   Parameters - int mailbox, int mailslot
   Returns - Nothing
   --------------------------------------------------------------------------*/
void insert_mail_slot(int mailbox, int mailslot)
{
   slot_ptr walker;
   walker = MailBoxTable[mailbox].slot_ptr;
   if (walker == NULL)
   {
      MailBoxTable[mailbox].slot_ptr = &MailSlotTable[mailslot];
   }
   else
   {
      while (walker != NULL)
      {
         walker = walker->next_slot_ptr;
      }
      walker->next_slot_ptr = &MailSlotTable[mailslot];
   }
   return;
} /* insert_mail_slot */
