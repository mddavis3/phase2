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
void disk_handler(int, void *);
void clock_handler2(int, void *);
void terminal_handler(int, void *);
void syscall_handler(int, void *);
void insert_mail_slot(int, int);       //added for MboxSend
void remove_mail_slot(int);
void insert_blocked_proc(int);         //added for MboxSend MboxReceive
void remove_blocked_proc(int);         //added for MboxSend MboxReceive
void mass_unbloxodus(int);             //added for MboxSend MboxRecieve
void unbloxodus(int);
void print_Mbox_Blocked_List(int);     //added for debug purposes
static void nullsys(sysargs *);

/* -------------------------- Globals ------------------------------------- */

int debugflag2 = 0;

/* the mail boxes */
mail_box MailBoxTable[MAXMBOX];
mail_slot MailSlotTable[MAXSLOTS];                          //added by Michael for start1 - REVIEW
mbox_proc MboxProcs[MAXPROC];                               //added by Michael for start1 - REVIEW

/* Mail box id used to keep track of unique mail box ids */ //added by Michael for MboxCreate
int global_mbox_id;

/* Dummy mailbox used for initialization */                 //added by Michael for start1
mail_box dummy_mbox = {0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/* Dummy mailslot used for initialization */                //added by Michael for start1
mail_slot dummy_slot = {0, NULL, NULL, NULL, NULL, NULL};

/* Dummy proc used for initialization */                    //added by Michael for start1
mbox_proc dummy_proc = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/* define the variable for the interrupt vector declared by USLOSS */
void(*int_vec[NUM_INTS])(int dev, void * unit);

/* define the variable for the system call vector declared by USLOSS */
void(*sys_vec[MAXSYSCALLS])(sysargs *args);

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
    * Create I/O mailboxes - completed
    * Initialize int_vec and sys_vec arrays 
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

   /* Allocate the 7 I/O Mailboxes */
   for ( int i = 0; i < 7; i++)
   {
      MboxCreate(0, MAX_MESSAGE);   
   }

   /* Initialize the interrupt handlers */
   int_vec[CLOCK_DEV] = clock_handler2;
   int_vec[DISK_DEV] = disk_handler;
   int_vec[TERM_DEV] = terminal_handler;

   /* Initialize the sys_vec array */
   int_vec[SYSCALL_INT] = syscall_handler;

   // Initialize the system call handlers to nullsys  - REVIEW
   for ( int i = 0; i < MAXSYSCALLS; i++)
   {
      sys_vec[i] = nullsys;
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
   MailBoxTable[i].num_blocked_procs = 0;

   //increment global int mbox_id
   //return >= 0 as the mailbox id number
   //global_mbox_id++;
   MailBoxTable[i].mbox_id = global_mbox_id++;
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

   int i = 0;
   int j = 0;

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

   //check if zero-slot mailbox
   if (MailBoxTable[i].num_slots == 0)
   {
      //check if mailbox has blocked receiver
      if (MailBoxTable[i].proc_ptr == NULL)
      {
         //no blocked receiver, block the sender
         //add to MboxProc Table
         //initialize the mbox_proc values
         MboxProcs[getpid()%MAXPROC].pid = getpid();
         MboxProcs[getpid()%MAXPROC].blocked = BLOCKED;
         MboxProcs[getpid()%MAXPROC].blocked_how = MBOXZEROSENDING;
         MailBoxTable[i].num_blocked_procs++;
         insert_blocked_proc(i); 

         //if process is zapped while it's blocked, return -3
         if ( block_me(MBOXZEROSENDING) == -1)
         {
            return -3;
         }

         //if the mailbox was released, return -3
         if (MailBoxTable[i].status == RELEASED)
         {
            //check if this is the last process blocked on the mailbox
            //if it is, unblock the releaser using unblock_proc
            if (MboxProcs[getpid()%MAXPROC].last_status == LASTPROC)
            {
               unblock_proc(MailBoxTable[i].releaser_pid);
            }
            return -3;
         }
         
         return 0;
      }
      
      if (MailBoxTable[i].proc_ptr != NULL && MailBoxTable[i].proc_ptr->blocked_how == MBOXZERORECEIVING)
      {
         //the receiver is blocked
         //check the size of receiver's buffer
         if (MailBoxTable[i].proc_ptr->message_size < msg_size)
         {
            //return -1 if the message is too big
            if (DEBUG2 && debugflag2)
            {
               console ("MboxSend(): message is too big for receiver's buffer. Return -1\n");
            }
            return (-1);
         }

         //assign message size to the recieving process
         //copy the message to the receiver assuming the message ptr is not NULL
         if(msg_ptr !=NULL)
         {
            MailBoxTable[i].proc_ptr->message_size = msg_size;
            memcpy(&MailBoxTable[i].proc_ptr->message, msg_ptr, msg_size);
         }
         //unblock receiver
         unbloxodus(i);

         //return 0 for success
         return 0;
      }
      else //the process blocked on the mailbox is a sender, so block this process on the mailbox
      {
         MboxProcs[getpid()%MAXPROC].pid = getpid();
         MboxProcs[getpid()%MAXPROC].blocked = BLOCKED;
         MboxProcs[getpid()%MAXPROC].blocked_how = MBOXZEROSENDING;
         MailBoxTable[i].num_blocked_procs++;
         insert_blocked_proc(i);

         //if process is zapped while it's blocked, return -3
         if ( block_me(MBOXZEROSENDING) == -1)
         {
            return -3;
         }

         //if the mailbox was released, return -3
         if (MailBoxTable[i].status == RELEASED)
         {
            //check if this is the last process blocked on the mailbox
            //if it is, unblock the releaser using unblock_proc
            if (MboxProcs[getpid()%MAXPROC].last_status == LASTPROC)
            {
               unblock_proc(MailBoxTable[i].releaser_pid);
            }
            return -3;
         }
      }

      //consider other cases here...
      return 0;
   }//end zero slot code


   //check to see if there are available mail slots
   //block the sender if there are no available slots
   //this applies only to nonzero-slot mailboxes
   if (MailBoxTable[i].num_used_slots >= MailBoxTable[i].num_slots && MailBoxTable[i].num_slots > 0)
   {

      //initialize the mbox_proc values
      MboxProcs[getpid()%MAXPROC].pid = getpid();
      MboxProcs[getpid()%MAXPROC].blocked = BLOCKED;
      MboxProcs[getpid()%MAXPROC].blocked_how = MBOXFULL;
      MailBoxTable[i].num_blocked_procs++;

      //throw the process into the blocked list on that mailbox
      insert_blocked_proc(i);

      //block the sender cus it's full if blockme returns -1 
      //then the process was zapped during its block_me call so return -3
      if ( block_me(MBOXFULL) == -1)
      {
         return -3;
      }

      //if the mailbox was released, return -3
      if (MailBoxTable[i].status == RELEASED)
      {
         //check if this is the last process blocked on the mailbox
         //if it is, unblock the releaser using unblock_proc
         if (MboxProcs[getpid()%MAXPROC].last_status == LASTPROC)
         {
            unblock_proc(MailBoxTable[i].releaser_pid);
         }
         return -3;
      }
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
   memcpy(&MailSlotTable[j].message, msg_ptr, msg_size);
   MailSlotTable[j].message_size = msg_size;

   //maintain linked list of mailslots
   insert_mail_slot(i, j);

   //make necessary changes to the mailbox struct
   MailBoxTable[i].num_used_slots++;

   //unblock one receiver blocked due to mailbox empty
   unbloxodus(i);

   return 0;

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

   int i = 0;
   int message_size;

   //check for the mailbox in the MailBoxTable
   while (MailBoxTable[i].mbox_id != mbox_id)
   {
      i++;
      //return -1 if mailbox is not in the mailbox table
      if (i == MAXMBOX)
      {
         if (DEBUG2 && debugflag2)
         {
            console ("MboxReceive(): mailbox not found in the mailbox table. Return -1\n");
         }
         return (-1);
      }
   }

   //check if the mail box has any messages available
   //this applies only for nonzero-slot mailboxes
   if (MailBoxTable[i].num_used_slots == 0 && MailBoxTable[i].num_slots > 0)
   {
      //block the receiver if there are no messages
      if (DEBUG2 && debugflag2)
      {
         console ("MboxReceive(): mailbox has no messages. Block the reciever.\n");
      }
      
      //Add to MboxProc Table
      //initialize the mbox_proc values
      MboxProcs[getpid()%MAXPROC].pid = getpid();
      MboxProcs[getpid()%MAXPROC].blocked = BLOCKED;
      MboxProcs[getpid()%MAXPROC].blocked_how = MBOXEMPTY;
      MailBoxTable[i].num_blocked_procs++;

      //throw the process into the blocked list of the mailbox and then block process
      //if process was zapped while blocked, return -3
      insert_blocked_proc(i);
      if (block_me(MBOXEMPTY) == -1)
      {
         return -3;
      }

      //if the mailbox was released, return -3
      if (MailBoxTable[i].status == RELEASED)
      {
         //check if this is the last process blocked on the mailbox
         //if it is, unblock the releaser using unblock_proc
         if (MboxProcs[getpid()%MAXPROC].last_status == LASTPROC)
         {
            unblock_proc(MailBoxTable[i].releaser_pid);
         }
         return -3;
      }
   }

   //check for zero-slot mailbox
   if (MailBoxTable[i].num_slots == 0)
   {
      if (MailBoxTable[i].proc_ptr == NULL)
      {
         //no blocked sender, block the receiver
         //add to MboxProc Table
         //initialize the mbox_proc values
         MboxProcs[getpid()%MAXPROC].pid = getpid();
         MboxProcs[getpid()%MAXPROC].blocked = BLOCKED;
         MboxProcs[getpid()%MAXPROC].blocked_how = MBOXZERORECEIVING;
         MboxProcs[getpid()%MAXPROC].message_size = msg_size;
         MailBoxTable[i].num_blocked_procs++;
         insert_blocked_proc(i);

         //if the process was zapped while blocked, return -3 
         if (block_me(MBOXZERORECEIVING) == -1)
         {
            return -3;
         }

         //if the mailbox was released, return -3
         if (MailBoxTable[i].status == RELEASED)
         {
            //check if this is the last process blocked on the mailbox
            //if it is, unblock the releaser using unblock_proc
            if (MboxProcs[getpid()%MAXPROC].last_status == LASTPROC)
            {
               unblock_proc(MailBoxTable[i].releaser_pid);
            }
            return -3;
         }

         //move message from proc struct to msg_ptr
         memcpy(msg_ptr, &MboxProcs[getpid()%MAXPROC].message, MboxProcs[getpid()%MAXPROC].message_size);
         return MboxProcs[getpid()%MAXPROC].message_size;
      }
      
      if (MailBoxTable[i].proc_ptr != NULL && MailBoxTable[i].proc_ptr->blocked_how == MBOXZEROSENDING)
      {
         //the sender is blocked
         //check the size
         if (MailBoxTable[i].proc_ptr->message_size > msg_size)
         {
            //return -1 if the message is too big
            if (DEBUG2 && debugflag2)
            {
               console ("MboxReceive(): message is too big for receiver's buffer. Return -1\n");
            }
            return (-1);
         }


         //copy the message from sender 
         message_size = MailBoxTable[i].proc_ptr->message_size;
         memcpy(msg_ptr, &MailBoxTable[i].proc_ptr->message, message_size);

         //unblock sender
         unbloxodus(i);

         //return the message_size
         return message_size;
      }
      else //the process blocked on the mailbox is a receiver, so block this process
      {
         MboxProcs[getpid()%MAXPROC].pid = getpid();
         MboxProcs[getpid()%MAXPROC].blocked = BLOCKED;
         MboxProcs[getpid()%MAXPROC].blocked_how = MBOXZERORECEIVING;
         MboxProcs[getpid()%MAXPROC].message_size = msg_size;
         MailBoxTable[i].num_blocked_procs++;
         insert_blocked_proc(i);

         //if the process was zapped while blocked, return -3 
         if (block_me(MBOXZERORECEIVING) == -1)
         {
            return -3;
         }

         //if the mailbox was released, return -3
         if (MailBoxTable[i].status == RELEASED)
         {
            //check if this is the last process blocked on the mailbox
            //if it is, unblock the releaser using unblock_proc
            if (MboxProcs[getpid()%MAXPROC].last_status == LASTPROC)
            {
               unblock_proc(MailBoxTable[i].releaser_pid);
            }
            return -3;
         }
      }
      
      // move message from proc struct to msg_ptr
      memcpy(msg_ptr, &MboxProcs[getpid()%MAXPROC].message, MboxProcs[getpid()%MAXPROC].message_size);
      return MboxProcs[getpid()%MAXPROC].message_size;
   } //end zero slot code
   

   //check the length of the message
   if (MailBoxTable[i].slot_ptr->message_size > msg_size)
   {
      //return -1 if the message is too big
      if (DEBUG2 && debugflag2)
      {
         console ("MboxReceive(): message is too big for receiver's buffer. Return -1\n");
      }
      return (-1);
   }
   message_size = MailBoxTable[i].slot_ptr->message_size;

   //copy the message from the mail slot to the receiver's message buffer
   memcpy(msg_ptr, &MailBoxTable[i].slot_ptr->message, message_size);

   //release the mailbox slot
   remove_mail_slot(i);

   //unblock one sender blocked due to full mailbox
   unbloxodus(i);
   
   return message_size;

} /* MboxReceive */

/* ------------------------------------------------------------------------
   Name - MboxRelease
   Purpose - releases a previously created mailbox
   Parameters - mailbox id.
   Returns - -3 if process was zapped while releasing the mailbox
             -1 the mailbox ID is not a mailbox that is in use
              0 successful completion
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxRelease(int mailboxID)
{
   int i = 0;

   //mark mailbox as being released
   //return -1 mailbox id is not a mailbox in use
   while ( MailBoxTable[i].mbox_id != mailboxID)
   {
      i++;
      if ( i >= MAXMBOX)
      {
         return -1;
      }
   }
   MailBoxTable[i].status = RELEASED;
   MailBoxTable[i].releaser_pid = getpid();

   //reclaim the mailslots from the mailslot table
   if (MailBoxTable[i].num_used_slots > 0)
   {
      while (MailBoxTable[i].num_used_slots > 0)
      {
         //release mailslots
         remove_mail_slot(i);
      }
   }

   //check for blocked procs and unblock them
   //block the releaser so that unblocked procs have a chance to finish
   if (MailBoxTable[i].num_blocked_procs > 0)
   {      
      mass_unbloxodus(i);
      block_me(MBOXRELEASING);
   }

   //return 0 if successful
   return 0;
} /* MboxRelease */

/* ------------------------------------------------------------------------
   Name - MboxCondSend
   Purpose - Put a message into a slot for the indicated mailbox.
   Parameters - mailbox id, pointer to data of msg, # of bytes in msg.
   Returns - -3 if process is zap'd
             -2 if mailbox is full, message not sent, or no mailbox slots
                available
             -1 if illegal values given as arguments
              0 if message sent successfully
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxCondSend(int mailboxID, void *message, int message_size)
{
   /* Check for kernel mode */
   check_kernel_mode();

   int i = 0;
   int j = 0;

   //check for the mailbox in the mailbox table
   while (MailBoxTable[i].mbox_id != mailboxID)
   {
      i++;
      //return -1 if mailbox is not in the mailbox table
      if (i == MAXMBOX)
      {
         if (DEBUG2 && debugflag2)
         {
            console ("MboxCondSend(): mailbox not found in the mailbox table. Return -1\n");
         }
         return (-1);
      }
   }

   //return -1 if message size is too large
   if (message_size > MailBoxTable[i].max_slot_size)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("MboxCondSend(): message size is too large. Return -1\n");
      }
      return (-1);
   }

   //return -1 if the mailbox was released
   if (MailBoxTable[i].status == RELEASED)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("MboxCondSend(): Mailbox was released. Return -1\n");
      }
      return (-1);
   }

   //check if zero-slot mailbox
   if (MailBoxTable[i].num_slots == 0)
   {
      //check if mailbox has blocked receiver
      if (MailBoxTable[i].proc_ptr == NULL)
      {
         //no blocked receiver, message not sent
         //return -2
         if (DEBUG2 && debugflag2)
         {
            console ("MboxCondSend(): no receiver waiting, message not sent. Return -2\n");
         }
         return (-2);
      }
      
      //if there is a blocked process on mailbox
      if (MailBoxTable[i].proc_ptr != NULL)
      {
         //the receiver is blocked
         //check the size of receiver's buffer
         if (MailBoxTable[i].proc_ptr->message_size < message_size)
         {
            //return -1 if the message is too big
            if (DEBUG2 && debugflag2)
            {
               console ("MboxCondSend(): message is too big for receiver's buffer. Return -1\n");
            }
            return (-1);
         }

         //copy the message to the receiver
         if (DEBUG2 && debugflag2)
         {
         console("MboxCondSend(): Address of message pointer is: %p\n", message);
         console("MboxCondSend(): contents of message pointer is: %d\n", *(int*)message);
         }
         
         //check if mailbox was released, return -1 if so
         if (MailBoxTable[i].status == RELEASED)
         {
            if (DEBUG2 && debugflag2)
            {
               console ("MboxCondSend(): Mailbox was released. Return -1\n");
            }
            return (-1);
         }

         memcpy(&MailBoxTable[i].proc_ptr->message, message, message_size);

         //unblock receiver
         unbloxodus(i);

         //return 0 for success
         return 0;
      }
      
      //consider other cases here...
      return 0;
   }

   //check if there are any unused slots, return -2 if not
   //for nonzero-slot mailboxes only
   if (MailBoxTable[i].num_used_slots >= MailBoxTable[i].num_slots && MailBoxTable[i].num_slots > 0)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("MboxCondSend(): There are no unused mailbox slots. Return -2\n");
      }
      return (-2);
   }

   //once slot is available, allocate slot from MailSlotTable
   while (MailSlotTable[j].is_free != 0)
   {
      j++;
      //return -2 if no slots available in MailSlotTable
      if (j >= MAXSLOTS)
      {
         if (DEBUG2 && debugflag2)
         {
            console ("MboxCondSend(): No slots available in MailSlotTable.\n");
         }
         return (-2);
      }
   }

   //return -1 if the mailbox was released
   if (MailBoxTable[i].status == RELEASED)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("MboxCondSend(): Mailbox was released. Return -1\n");
      }
      return (-1);
   }

   //initialize the mail_slot struct
   MailSlotTable[j].is_free = 1;
   MailSlotTable[j].mbox_id = MailBoxTable[i].mbox_id;
   memcpy(&MailSlotTable[j].message, message, message_size);
   MailSlotTable[j].message_size = message_size;

   //maintain linked list of mailslots
   insert_mail_slot(i, j);

   //make necessary changes to the mailbox struct
   MailBoxTable[i].num_used_slots++;

   //unblock one receiver blocked due to mailbox empty
   unbloxodus(i);

   //check if zap'd
   if (is_zapped() == 1)
   {
      //return -3 if zap'd
      return -3;
   }
   
   //successful return
   return 0;
} /* MboxCondSend */

/* ------------------------------------------------------------------------
   Name - MboxCondReceive
   Purpose - Put a message into a slot for the indicated mailbox.
   Parameters - mailbox id, pointer to data of msg, # of bytes in buffer.
   Returns - -3 if process is zap'd
             -2 if mailbox empty or no message to receive
             -1 if illegal values
              0 if successful
   Side Effects - none.
----------------------------------------------------------------------- */
int MboxCondReceive(int mailboxID, void *message, int max_message_size)
{
   /* Check for kernel mode */
   check_kernel_mode();

   int i = 0;
   int message_size;

   //check for the mailbox in the MailBoxTable
   while (MailBoxTable[i].mbox_id != mailboxID)
   {
      i++;
      //return -1 if mailbox is not in the mailbox table
      if (i == MAXMBOX)
      {
         if (DEBUG2 && debugflag2)
         {
            console ("MboxCondReceive(): mailbox not found in the mailbox table. Return -1\n");
         }
         return (-1);
      }
   }

   //return -1 if the mailbox was released
   if (MailBoxTable[i].status == RELEASED)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("MboxCondReceive(): Mailbox was released. Return -1\n");
      }
      return (-1);
   }

   //check if the mailbox has any messages
   if (MailBoxTable[i].num_used_slots == 0)
   {
      if (DEBUG2 && debugflag2)
         {
            console ("MboxCondReceive(): mailbox is empty. Return -2\n");
         }
      return (-2);
   }

   //check the length of the message
   if (MailBoxTable[i].slot_ptr->message_size > max_message_size)
   {
      //return -1 if the message is too big
      if (DEBUG2 && debugflag2)
      {
         console ("MboxCondReceive(): message is too big for receiver's buffer. Return -1\n");
      }
      return (-1);
   }
   message_size = MailBoxTable[i].slot_ptr->message_size;

   //return -1 if the mailbox was released
   if (MailBoxTable[i].status == RELEASED)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("MboxCondReceive(): Mailbox was released. Return -1\n");
      }
      return (-1);
   }
   
   //copy the message from the mail slot to the receiver's message buffer
   memcpy(message, &MailBoxTable[i].slot_ptr->message, message_size);

   //release the mailbox slot
   remove_mail_slot(i);

   //unblock one sender blocked due to full mailbox
   unbloxodus(i);

   //check if zap'd
   if (is_zapped() == 1)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("MboxCondReceive(): message is zap'd bruh. Return -3\n");
      }
      return (-3);
   }

   //successful return
   return 0;
} /* MboxCondReceive */

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
   mbox_proc_ptr walker;
   int flag = 0;
   int i = 0;

   while (i < 7)
   {
      walker = MailBoxTable[i].proc_ptr;
      if (walker != NULL)
      {
         flag++;
      }
      i++;
   }

   if (flag > 0)
   {
      return 1;
   }
   else return 0;
} /* check_io */

/* -------------------------------------------------------------------------
   Name - disk_handler()
   Purpose - Code to handle Disk innterupts.
   Parameters - int device type, pointer to which unit  
   Returns -
   --------------------------------------------------------------------------*/
void disk_handler(int dev, void *punit)
{
   if (dev != DISK_DEV)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("disk_handler(): wrong device. return.\n");
      }
      return;   
   }

   int status;
   int result;
   int unit = (int)punit;
   int table_offset = 1;

   if (unit < 0 && unit > 1)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("disk_handler(): unit is out of range. return.\n");
      }
      return;   
   }
   
   /*check the mailbox set-up for the disk unit. If not OK, return */
   device_input(DISK_DEV, unit, &status);
   result = MboxCondSend((unit+table_offset), &status, sizeof(status));

   //more checking on the return result
   return;
} /* disk_handler */


/* -------------------------------------------------------------------------
   Name - clock_handler2()
   Purpose - Code to handle clock innterupts.
   Parameters - int device type, pointer to which unit 
   Returns - 
   --------------------------------------------------------------------------*/
void clock_handler2(int dev, void *punit)
{
   if (dev != CLOCK_DEV)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("clock_handler2(): wrong device. return.\n");
      }
      return;   
   }

   if (DEBUG2 && debugflag2)
   {
      console ("clock_handler2(): confirmed entry. target acquired.\n");
   }

   //clock_ticker is static so it keeps accumulating every call of clock_handler2 while program is being ran.
   static int clock_ticker = 0;   
   int dummy_value = 0; //for dummy message to send to anybody receiving on the clock mailbox

   //add a 20ms tick to clock_ticker
   clock_ticker = clock_ticker + CLOCK_MS;

   //check if 5 interrupts have happened
   if (clock_ticker % 100 == 0)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("clock_handler2(): sending message.\n");
      }

      //Five interrupts have occured for a total of 100ms do a MboxCondSend to clock mailbox
      if (DEBUG2 && debugflag2)
      {
         console("clock_handler(): dummy_value %p\n", &dummy_value);
      }
      MboxCondSend(CLOCK_DEV, &dummy_value, sizeof(int)); //REVIEW THIS
      

      //Call timeslice to kick out current process if need be.
      time_slice();
   }

   return;
} /* clock_handler2*/


/* -------------------------------------------------------------------------
   Name - terminal_handler()
   Purpose - Code to handle terminal interupts.
   Parameters - int device type, pointer to which unit 
   Returns - 
   --------------------------------------------------------------------------*/
void terminal_handler(int dev, void *punit)
{
   if (dev != TERM_DEV)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("terminal_handler(): wrong device. return.\n");
      }
      return;   
   }

   int status;
   int result;
   int unit = (int)punit;
   int table_offset = 3;

   if (unit < 0 && unit > 3)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("terminal_handler(): unit is out of range. return.\n");
      }
      return;   
   }
   
   /*check the mailbox set-up for the disk unit. If not OK, return */
   device_input(TERM_DEV, unit, &status);
   result = MboxCondSend((unit+table_offset), &status, sizeof(status));

   //more checking on the return result
   return;
} /* terminal_handler*/

/* -------------------------------------------------------------------------
   Name - syscall_handler()
   Purpose - 
   Parameters - 
   Returns -
   --------------------------------------------------------------------------*/
void syscall_handler(int dev, void *unit)
{
   sysargs *sys_ptr;

   sys_ptr = (sysargs *) unit;
   // Sanity check: if the interrupt is not SYSCALL_INT, halt(1)
   if (dev != SYSCALL_INT)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("syscall_handler(): Interrupt is not SYSCALL_INT. halt(1).\n");
      }
      halt(1);
   }
   /* check what system: if the call is not in the range between 0 and MAXSYSCALLS, , halt(1) */ 
   if (sys_ptr->number < 1 || sys_ptr->number >= MAXSYSCALLS)
   {
      console ("syscall_handler(): sys_ptr->number not in range.  halt(1).\n");
      halt(1);
   }
   
	/* Now it is time to call the appropriate system call handler */ 
	sys_vec[sys_ptr->number](sys_ptr);

   return;
} /* syscall_handler */

/* -------------------------------------------------------------------------
   Name - waitdevice()
   Purpose - Does a MboxReceive operation on I/O mailboxes.
   Parameters - None
   Returns - -1 if the process was zap'd while waiting
              0 if successful
   --------------------------------------------------------------------------*/
int waitdevice(int type, int unit, int *status)
{

   int result = 0;
   int table_offset;	
   // Sanity checks
   if (type != CLOCK_DEV && type != DISK_DEV && type != TERM_DEV)
   {
      if (DEBUG2 && debugflag2)
      {
         console ("waitdevice(): type is not compatible. halt(1).\n");
      }
      halt(1);
   } 
	//more code logic could be inserted before the below checking 
	switch (type) 
   {		
      case CLOCK_DEV: 
         result = MboxReceive(unit, status, sizeof(int)); 
		   break;	
      case DISK_DEV:
         table_offset = 1;
         unit = unit + table_offset; 
		   result = MboxReceive(unit, status, sizeof(int)); 
		   break;
      case TERM_DEV: 
         table_offset = 3;
         unit = unit + table_offset;
         result = MboxReceive(unit, status, sizeof(int)); 
		   break; 
		default: 
		   printf("waitdevice(): bad type (%d). Halting...\n", type); 
		   halt(1); 
   } 

   //if the process was zap'd, return -1
   if (result == -3)
   { 
	    return -1;
   } 
   else return 0; 
} /* waitdevice */

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
   slot_ptr previous;
   walker = MailBoxTable[mailbox].slot_ptr;
   if (walker == NULL)
   {
      MailBoxTable[mailbox].slot_ptr = &MailSlotTable[mailslot];
   }
   else
   {
      while (walker != NULL)
      {
         previous = walker;
         walker = walker->next_slot_ptr;
      }
      previous->next_slot_ptr = &MailSlotTable[mailslot];
   }
   return;
} /* insert_mail_slot */

/* -------------------------------------------------------------------------
   Name - remove_mail_slot()
   Purpose - maintain the linked list of mailslots in the mbox and mailslot
             structs, release mailbox slots from mailslottable
   Parameters - int mbox_index
   Returns - Nothing
   --------------------------------------------------------------------------*/
void remove_mail_slot(int mbox_index)
{
   int j = 0;
   while (&MailSlotTable[j] != MailBoxTable[mbox_index].slot_ptr)
   {
      j++;
   }
   if (MailBoxTable[mbox_index].slot_ptr->next_slot_ptr == NULL)
   {
      MailBoxTable[mbox_index].slot_ptr = NULL;
   }
   else
   {
      MailBoxTable[mbox_index].slot_ptr = MailBoxTable[mbox_index].slot_ptr->next_slot_ptr;
   }
   MailSlotTable[j] = dummy_slot;

   //decrement used_slots on mailbox
   MailBoxTable[mbox_index].num_used_slots--;

   return;
} /* remove_mail_slot */

/* -------------------------------------------------------------------------
   Name - insert_blocked_proc()
   Purpose - insert proc to the linked list of blocked processes on a mailbox
   Parameters - int mailbox table slot
   Returns - Nothing
   --------------------------------------------------------------------------*/
void insert_blocked_proc(int mailbox)
{
  
   mbox_proc_ptr walker;
   mbox_proc_ptr previous;
   int i = getpid()%MAXPROC;
   walker = MailBoxTable[mailbox].proc_ptr;
   if (walker == NULL)
   {
     
      MailBoxTable[mailbox].proc_ptr = &MboxProcs[i];
    
   }
   else
   {
      while (walker != NULL)
      {
         previous = walker;
         walker = walker->next_proc_ptr;
      }
      previous->next_proc_ptr = &MboxProcs[i];
      
   }
  
   return;
} /* insert_blocked_proc */

/* -------------------------------------------------------------------------
   Name - remove_blocked_proc()
   Purpose - remove proc from the linked list of blocked processes on a mailbox
             updates the data fields of the mbox proc
   Parameters - int mailbox
   Returns - Nothing
   --------------------------------------------------------------------------*/
void remove_blocked_proc(int mailbox)
{
   if (MailBoxTable[mailbox].proc_ptr->next_proc_ptr == NULL)
   {
      MailBoxTable[mailbox].proc_ptr = NULL;
   }
   else
   {
      MailBoxTable[mailbox].proc_ptr = MailBoxTable[mailbox].proc_ptr->next_proc_ptr;
      MboxProcs[getpid()%MAXPROC].next_proc_ptr = NULL;
   }

   MboxProcs[getpid()%MAXPROC].blocked = UNBLOCKED;
   MboxProcs[getpid()%MAXPROC].blocked_how = UNBLOCKED;
   

   return;
} /* remove_blocked_proc */

/* -------------------------------------------------------------------------
   Name - mass_unbloxodus()
   Purpose - unblock all processes blocked on a mailbox
   Parameters - int mailbox
   Returns - Nothing
   --------------------------------------------------------------------------*/
void mass_unbloxodus(int mailbox)
{
   int pid;
   /*int zap_flag = 0;

   if (MailBoxTable[mailbox].status == RELEASED)
   {
      zap_flag = 1;
   }*/

   while(MailBoxTable[mailbox].proc_ptr != NULL)
   {   
      //if statement checks if the process is the last blocked process on the mailbox
      //marks it as the last process if it is (used for MboxRelease)    
      if (MailBoxTable[mailbox].proc_ptr->next_proc_ptr == NULL)
      {
         MailBoxTable[mailbox].proc_ptr->last_status = LASTPROC;
      }                                  
      pid = MailBoxTable[mailbox].proc_ptr->pid;

      /*if the zap_flag is 1, then zap the processes as they are being unblocked
      if (zap_flag == 1)
      {
         zap(pid);
      }*/

      MailBoxTable[mailbox].num_blocked_procs--;
      remove_blocked_proc(mailbox);
      unblock_proc(pid);
   }

   return;
} /* mass_unbloxodus */


/* -------------------------------------------------------------------------
   Name - unbloxodus()
   Purpose - unblock one process blocked on a mailbox
   Parameters - int mailbox
   Returns - Nothing
   --------------------------------------------------------------------------*/
void unbloxodus(int mailbox)
{
   int pid;
     
   if(MailBoxTable[mailbox].proc_ptr != NULL) 
   {                                            
      pid = MailBoxTable[mailbox].proc_ptr->pid;
      MailBoxTable[mailbox].num_blocked_procs--;
      remove_blocked_proc(mailbox);
      unblock_proc(pid);
   }
   return;
} /* unbloxodus */



/* -------------------------------------------------------------------------
   Name - print_Mbox_Blocked_Lists(int mailbox)
   Purpose - print all the blocked procs on a mailbox for debug prurposes 
   Parameters - int mailbox table slot
   Returns - Nothing
   --------------------------------------------------------------------------*/
void print_Mbox_Blocked_List(int mailbox_tbl_slot)
{
     
   mbox_proc_ptr walker;
      

   //Print all the Processes blocked on this mailbox.
      walker = MailBoxTable[mailbox_tbl_slot].proc_ptr;  //set walker to proc_ptr of mailbox found on mbox table earlier
      console("\n\n==================== Blocked List on Mailbox ===========================================\n\n");
      console ("Mbox Spot in Table: %d  MboxID: %d\n", mailbox_tbl_slot,  MailBoxTable[mailbox_tbl_slot].mbox_id);
      if (walker == NULL)
      {
         console("No Blocked Procs on this mailbox\n\n"); // no blocked list on this mail box 
      }

      while(walker != NULL)
      {
            
         console ("Blocked Proc Id: %d\n", walker->pid );
         walker = walker->next_proc_ptr;
      }
      console("\n========================================================================================\n\n");

}/*print_Mbox_Blocked_List*/

/* -------------------------------------------------------------------------
   Name - nullsys()
   Purpose - prints an error message
   Parameters - sysargs
   Returns - Nothing
   --------------------------------------------------------------------------*/
static void nullsys(sysargs *args)
{ 
	printf("nullsys(): Invalid syscall %d. Halting...\n", args->number); 
	halt(1);
} /* nullsys */