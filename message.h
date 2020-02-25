#define DEBUG2 1
#define ACTIVE 1              //added by Michael for MboxSend
#define INACTIVE 0            //added by Michael for MboxSend

typedef struct mail_slot *slot_ptr;
typedef struct mailbox mail_box;
typedef struct mbox_proc *mbox_proc_ptr;

struct mailbox {
   int      is_free;          //added by Michael for MboxCreate
   int      mbox_id;
   int      status;           //added by Michael for MboxSend
   int      num_slots;        //added by Michael for MboxCreate
   int      max_slot_size;    //added by Michael for MboxCreate           
   /* other items as needed... */
};

struct mail_slot {
   int       mbox_id;
   int       status;
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
