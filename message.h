#define DEBUG2 1
#define RELEASED 2            //added for MboxRelease
#define ACTIVE 1              //added by Michael for MboxSend
#define INACTIVE 0            //added by Michael for MboxSend
#define MBOXFULL 11           //added by Michael for MboxSend
#define MBOXEMPTY 12          //added for MboxReceive
#define MBOXZEROSENDING 13    //added for zero-slot mailboxes
#define MBOXZERORECEIVING 14
#define MBOXRELEASING 15      
#define BLOCKED 1
#define UNBLOCKED 0
#define LASTPROC 1

typedef struct mail_slot *slot_ptr;
typedef struct mailbox mail_box;
typedef struct mbox_proc *mbox_proc_ptr;
typedef struct mail_slot mail_slot;       //added by Michael for start1
typedef struct mbox_proc mbox_proc;       //added by Michael for start1

struct mailbox {
   int            is_free;             //added by Michael for MboxCreate
   int            mbox_id;
   int            status;              //added by Michael for MboxSend
   int            num_slots;           //added by Michael for MboxCreate
   int            max_slot_size;       //added by Michael for MboxCreate
   mbox_proc_ptr  proc_ptr;            //pointing to process table entry - first process blocked on the mailbox
   slot_ptr       slot_ptr;            //pointing to the first message sent to the mailbox 
   int            num_used_slots;      //added to keep track of how many slots are being used 
   int            num_blocked_procs;   //keep track of how many blocked processes for MboxRelease
   int            releaser_pid;      
   /* other items as needed... */
};

struct mail_slot {
   int       is_free;                  //added by Michael for MboxSend
   int       mbox_id;
   int       status;
   int       message_size;
   slot_ptr  next_slot_ptr;            //slot_ptr pointing to the next message
   char      message [MAX_MESSAGE];    //added by Michael for MboxSend
   /* other items as needed... */
};

struct psr_bits {
    unsigned int cur_mode:1;
    unsigned int cur_int_enable:1;
    unsigned int prev_mode:1;
    unsigned int prev_int_enable:1;
    unsigned int unused:28;
};

union psr_values {
   struct psr_bits bits;
   unsigned int integer_part;
};

struct mbox_proc {                           //added by Michael from developement_slides1
   int            pid;
   int            blocked_how;               //indication of whether process is blocked on a send or a receive
   int            blocked;                   //indication of whether process is blocked
   mbox_proc_ptr  next_proc_ptr;             //pointing to the next blocked process
   int            message_size;
   char           message [MAX_MESSAGE];
   int            last_status;
   /* other items as needed */
};