#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "screen.h"
#include "string.h"
#include "lock.h"
#include "memory.h"
#include "timer.h"
#include "cpu.h"





//  /*
//  processes created by kernel, shell, user running programs, etc.
//  main mechanism is fork().
//  fork() shares the same code, file descriptors, all.
//  the data segment is copied, but not shared, different address space is created
//  This is separate than exec(), so that the shell child has the opportunity fo redirect stdin/out/err.
//  process termination is through either termination of program (compiler to call exit()),
//  premature voluntary exiting, being killed or signalled without handler, bugs etc.
//  any signal sent to a process, is also sent to all its children and so on.
//  seems like kernel will create init, the first process, for it to execute /etc/rc.
//  after /etc/rc, it looks at /etc/ttytab, and forks as many gettty processes.
//  they wait for user name, execute the login process on it, if successful, they run user's shell.

//  processes states can be: running, ready (runnable), blocked.
//  processes can block either by calling block(), by reading input from a pipe or terminal
//  when no data is available,
//  processes toggle between running and ready by the scheduler
//  they become unblocked when the event they're waiting on has happened.

//  it appears that an interrupt may cause the disk task to run, to handle the relevant disk interrupt
//  that's the pre-emptive nature of interrrupt driven systems.
//  the sleeping disk task is because it is blocked, waiting for this interrupt.
//  after being woken, it handles the event, then goes back to sleep, waiting another interrupt.

//  threads is another whole chapter we want to support. threads are not processes,
//  they all share the code and memory space with the other threads of the process.
//  threads are children of processes, many attributes come from them (e.g. PID),
//  but they have their own: instruction and stack pointer, regs, state.
//  we should push threads implementation later in the progress, because of complexity.

//  I think in the heart of switching is the logic of:
//  - push various things
//  - change DS
//  - pop the various things
//  - return (the return address is in the stack???)
//  Essentially, it prepares the return jump by changing stacks.
//  We may return to the original caller, if we set up their stack.
//  See swth.S and how the "init" process is triggered.
//  See https://github.com/tiqwab/xv6-x86_64/blob/master/kern/proc.c#L90-L98

//  We should differentiate between kernel processes and user processes.
//  We could allocate say, 1MB for kernel, have our pages, our stacks, etc.
//  This way, whenever an IP maps to physical > 2MB, it is user space, when < 2MB it's kernel.
 
//  The main idea is: we want each process to think that
//  - they have the CPU to themselves (we switch them without their knowledge)
//  - they have the memory to themselves (we give them a continuous virtual memory space)
//  - they have their own stack
//  */

// #define STATE_AVAILABLE  0  // available slot
// #define STATE_READY      1  // ready to run
// #define STATE_RUNNING    2  // running
// #define STATE_BLOCKED    3  // blocked or sleeping on something
// #define STATE_ZOMBIE     4  // exited, waiting for father to retrieve

// #define PRIORITY_HIGH   2
// #define PRIORITY_MID    1
// #define PRIORITY_LOW    0

// #define NUM_PRIORITIES               3  // 0=highest, n=lowest
// #define NUM_PROCESSES               64  // kernel will panic if we try to create and they're not enough
// #define CLOCK_TICKS_PER_RUN_SLOT    30  // roughly, milliseconds each time

// // user land process memory: code, original data + bss, fixed size stack, expandable heap
// // kernel land process memory: stack (code, data, heap are shared)
// struct process_struct {
//     uint16_t pid;
//     uint16_t parent_pid;
//     uint8_t priority;
//     uint8_t state: 2;
//     uint16_t running_ticks_left; // each tick a millisecond, usual values 20-50 msecs
//     uint16_t waiting_mask; // xv6 has pointer to channel we're sleeping on
//     char *command_line;
//     struct process_struct *next;
//     // saved registers and context
//     // char * mem_page_director;
//     // int mem_size;
//     // struct file_descriptor open_handles[FILES];
//     // inode_t *current_working_directory
// } __attribute__((packed));
// typedef struct process_struct process_t;


// struct process_list_struct {
//     process_t *head;
//     process_t *tail;
//     uint16_t count;
// };
// typedef struct process_list_struct process_list_t;


// process_t processes[NUM_PROCESSES];
// process_t *running_proc;
// process_list_t ready_procs[NUM_PRIORITIES];
// process_list_t waiting_procs; // most probably per... wait type?
// process_list_t available_procs;
// uint16_t last_pid;
// volatile lock_t process_lock;




// void init_processes();
// void enqueue_process(process_list_t *list, process_t *proc);
// process_t *dequeue_process(process_list_t *list);




// // setup tables, nubmers and pointers
// void init_processes() {
//     memset(processes, 0, sizeof(processes));
//     running_proc = NULL;
//     memset(ready_procs, 0, sizeof(ready_procs));
//     memset(&waiting_procs, 0, sizeof(waiting_procs));
//     memset(&available_procs, 0, sizeof(available_procs));
//     for (int i = 0; i < NUM_PROCESSES; i++) {
//         enqueue_process(&available_procs, &processes[i]);
//     }
// }

// // append a process at the end of the list, works in O(1) time
// void enqueue_process(process_list_t *list, process_t *proc) {
//     if (list->tail == NULL) {
//         list->tail = proc;
//         list->head = proc;
//     } else {
//         list->tail->next = proc;
//         list = proc;
//     }
//     list->count++;
//     proc->next = NULL;
// }

// // remove a process from the head of the list, works in O(1) time
// process_t *dequeue_process(process_list_t *list) {
//     if (list->head == NULL)
//         panic("Cannot dequeue process from empty queue");
    
//     process_t *proc;
//     proc = list->head;
//     list->head = proc->next;
//     if (list->head == NULL)
//         list->tail = NULL;
    
//     proc->next = NULL;
//     list->count--;
//     return proc;
// }

// process_t *allocate_new_process(uint16_t parent_pid, uint8_t priority) {
//     if (available_procs.count == 0)
//         panic("Cannot allocate new process, please increase the array size!");
    
//     acquire(&process_lock);
//     process_t *proc = dequeue_process(&available_procs);
//     last_pid += 1;
//     proc->pid = last_pid;
//     proc->parent_pid = parent_pid;
//     proc->priority = priority;
//     release(&process_lock);

//     // if this process runs in kernel,
//     // it needs not a code or data segment, 
//     // but it needs a stack segment.
//     // so, in theory we allocate memory and set SP to the stack
//     // then we prepare the initial stack frame, to call the entry point
//     // and the EIP is set to the fork() place to return 
//     // (that's why forking returns at the same place for parent and child!!!! :-)

// }












// void schedule_next_process() {
//     // the previously running process either exhausted the ticks or was Blocked
//     // find another candidate and run it.
//     // don't update current process, caller should have updated it
//     // higher priority lists must have precedence

//     // useful times to reschedule:
//     // - when a process has exited
//     // - when a process is blocked on IO
//     // - when a process has been created (re-evaluate priorities)
//     // - when an IO interrupt happens (maybe unblocks a process?)
//     // - when a clock interrupt happens (time sharing)

//     // if no process exists (everything waits on something) run the idle process


//     // find next ready process to run
//     process_t *target = NULL;
//     for (int priority = 0; priority < NUM_PRIORITIES; priority++) {
//         if (ready_procs[priority].head != NULL) {
//             target = dequeue_process(&ready_procs[priority]);
//             break;
//         }
//     }
//     if (target == NULL) {
//         // if the original task just fell into runnable mode,
//         // we would have found it in the runnable procs array.
//         // therefore, everything is waiting. we should run the idle task.
//     }

//     if (target == NULL)
//         panic("Proc: Nothing to schedule :-(");

//     running_proc = target;
//     running_proc->state = STATE_RUNNING;
//     running_proc->running_ticks_left = CLOCK_TICKS_PER_RUN_SLOT;
//     // what else?
// }

// void timer_ticked() {
//     if (running_proc == NULL)
//         return;

//     running_proc->running_ticks_left--;
//     if (running_proc->running_ticks_left > 0)
//         return;

//     // this will get paused, put it back to ready queue
//     running_proc->state = STATE_READY;
//     enqueue_process(&ready_procs[running_proc->priority], running_proc);

//     schedule_next_process();
// }


// // give up CPU control for this process, voluntarily
// void yield() {
//     // acquire lock
//     // mark this process as ready
// }

// // main function that return from a fork()
// int child_fork_return() {

// }

// // put current process to sleep on something
// void sleep_on(void *sleep_on) {
//     // acquire lock
//     // it does some magic with some lock, we'll see when we get there
//     // https://github.com/mit-pdos/xv6-public/blob/master/proc.c#L418

// }

// // any processes sleeping on this, mark them ready to run
// void wakeup(void *sleeping_on) {
//     // acquire lock
//     // loop, any process that is blocked where sleeping_on is target, 
//     // make it runnable again
// }

// // kill the target process
// int kill(uint32_t pid) {
//     // acquire lock
//     // loop, find, mark KILLED, READY, 
// }

// void dump_processes_table() {
//     char *states = {
//         "Available",
//         "Ready",
//         "Running",
//         "Blocked",
//         "Zombie"
//     };
//     //      12345 12345 High Running    12345678901234567890
//     printf("  PID  PPID Prio State      Name or cmd line\n");
//     for (int i = 0; i < NUM_PROCESSES; i++) {
//         if (!processes[i].pid)
//             continue;

//         printf("%5d %5d %4d %-10s  0x%02x %20s",
//             processes[i].pid,
//             processes[i].parent_pid,
//             processes[i].priority,
//             states[processes[i].state],
//             processes[i].command_line
//         );
//     }
// }










// /////////////////////////////////////////////////////////////////////
 /**
 * this method, written in assembly, performs a task switch
 * it takes two pointers to a uint32_t value
 * - first, it pushes a lot of registers on the stack, 
 * - then saves the ESP into the location pointed by the first argument.
 * - then it takes the value pointed by the second argument and sets ESP
 * - then it pops registers in the reverse order.
 * it will return to the caller whose ESP was saved as the second argument
 * 
 * if both pointers point to the same address, no apparent change will happen
 *
 * After the call, the old_esp value will point to the bottom of the saved stack
 * The stack_snapshot structure maps fields to what should be there in memory
 * if we make a stack_snapshot pointer to point to that value, we can see what's pushed
 * If we prepare such a structure, we can create a new task to switch to.
 * The way things are pushed and the stack_snapshot struct must be kept in sync
 */
extern void low_level_context_switch(uint32_t *old_esp_ptr, uint32_t *new_esp_ptr);

/**
 * this is what's pushed when switching and is used to prepare the target return
 * first entries in the structure are what has been pushed last,
 * or first entries is what will be popped first
 * the structure allows us to prepare new stack snapshot for starting new processes
 * see relevant assembly function
 */
struct switched_stack_snapshot {
    // these are explicitly pushed by us
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eflags;
    // this one is not pushed by our code, but by whoever calls our assembly method
    uint32_t return_address; 
} __attribute__((packed));
typedef struct switched_stack_snapshot switched_stack_snapshot_t;


// tiniest kernel process that can work
enum proc_state { READY, RUNNING, BLOCKED };
struct kernel_proc {
    char *name;
    struct kernel_proc *next;
    void *stack_page;
    union { // two views of the same piece of information
        uint32_t esp;
        switched_stack_snapshot_t *stack_snapshot;
    };
    uint64_t cpu_ticks_total;
    uint64_t cpu_ticks_last;
    enum proc_state state;
    int block_reason;
};
typedef struct kernel_proc kernel_proc_t;
struct proc_list {
    kernel_proc_t *head;
    kernel_proc_t *tail;
};
typedef struct proc_list proc_list_t;
kernel_proc_t kernel_procs[4];
kernel_proc_t *running_proc;
proc_list_t ready_list;
proc_list_t blocked_list;
typedef void (* func_ptr)();
volatile lock_t scheduling_lock = 0;

void schedule();
void dump_process_table();

void append(proc_list_t *list, kernel_proc_t *proc) {
    if (list->head == NULL) {
        list->head = proc;
        list->tail = proc;
        proc->next = NULL;
    } else {
        list->tail->next = proc;
        list->tail = proc;
        proc->next = NULL;
    }
}
void prepend(proc_list_t *list, kernel_proc_t *proc) {
    if (list->head == NULL) {
        list->head = proc;
        list->tail = proc;
        proc->next = NULL;
    } else {
        proc->next = list->head;
        list->head = proc;
    }
}
kernel_proc_t *dequeue(proc_list_t *list) {
    if (list->head == NULL)
        return NULL;
    kernel_proc_t *proc = list->head;
    list->head = proc->next;
    if (list->head == NULL)
        list->tail = NULL;
    proc->next = NULL;
    return proc;
}
void unlist(proc_list_t *list, kernel_proc_t *proc) {
    if (list->head == proc) {
        dequeue(list);
        return;
    }
    kernel_proc_t *trailing = list->head;
    while (trailing != NULL && trailing->next != proc)
        trailing = trailing->next;
    
    if (trailing == NULL)
        return; // we could not find it
    
    trailing->next = proc->next;
    if (list->tail == proc)
        list->tail = trailing;
    proc->next = NULL;
}

// this is how the running task can block itself
void block_me(int reason) {
    pushcli();
    running_proc->state = BLOCKED;
    running_proc->block_reason = reason;
    append(&blocked_list, running_proc);
    schedule();
    popcli();
}
void unblock_task(kernel_proc_t *proc) {
    pushcli();
    unlist(&blocked_list, proc);
    proc->state = READY;
    proc->block_reason = 0;
    prepend(&ready_list, proc);
    popcli();
}

void create_kernel_process(kernel_proc_t *proc, func_ptr entry_point, char *name) {
    char *stack_ptr = allocate_kernel_page();
    proc->stack_page = stack_ptr;
    proc->esp = (uint32_t)(stack_ptr + kernel_page_size() - sizeof(switched_stack_snapshot_t) - 64);
    proc->stack_snapshot->return_address = (uint32_t)entry_point;
    proc->name = name;
    proc->cpu_ticks_total = 0;
    proc->cpu_ticks_last = 0;
    proc->state = READY;
}
void process_a_main() {
    popcli(); // unlock the scheduler in our first execution

    int i = 0;
    while (true) {
        printf("A");
        pushcli();
        schedule();
        popcli();
        block_me(i);
        i++;
    }
    for (;;) asm("hlt");
}
void process_b_main() {
    popcli(); // unlock the scheduler in our first execution

    int i = 1000;
    while (true) {
        printf("B");
        pushcli();
        schedule();
        popcli();
    }
    for (;;) asm("hlt");
}
void idle_main() {
    popcli(); // unlock the scheduler in our first execution

    while (true) {
        printf("i");
        pushcli();
        schedule();
        popcli();
        // asm("hlt");
    }
}

// caller is responsible for locking interrupts before calling us
void schedule() {
    // acquire(&scheduling_lock);

    kernel_proc_t *previous = running_proc;
    kernel_proc_t *next = dequeue(&ready_list);
    if (next == NULL)
        return; // nothing to switch to
    
    // if it was running, put back to the ready list
    if (previous->state == RUNNING) {
        previous->state = READY;
        append(&ready_list, previous);
    }

    // before switching, some house keeping
    previous->cpu_ticks_total += (timer_get_uptime_msecs() - previous->cpu_ticks_last);

    // mark the new running proc, "next" variable will have a different value afterwards
    running_proc = next;
    running_proc->state = RUNNING;

    /**
     * -------------------------------------------------------------------
     * completely unintiutive, but immensely important:
     * before and after this call, we are in a different stack frame.
     * the values of all arguments and local variables are different!!!!!
     * for example, after the switch, the "old" becomes whatever was used 
     * to switch out the thing we are going to switch in!!!!
     * so, be careful what the expectations are before and after calling this method.
     * -------------------------------------------------------------------
     */
    low_level_context_switch(
        &previous->esp,
        &running_proc->esp
    );

    // current pointer now has the current step.
    running_proc->cpu_ticks_last = timer_get_uptime_msecs();

    // release(&scheduling_lock);
}
void test_switching_start() {
    // we should not neglect the original task that has been running since boot
    // this is what we will switch "from" into whatever other task we want to spawn.
    // this way we always have a "from" to switch from...
    memset((char *)kernel_procs, 0, sizeof(kernel_procs));
    memset((char *)&ready_list, 0, sizeof(ready_list));
    memset((char *)&blocked_list, 0, sizeof(blocked_list));
    create_kernel_process(&kernel_procs[0], 0x00, "Booted");
    create_kernel_process(&kernel_procs[1], process_a_main, "Proc_A");
    create_kernel_process(&kernel_procs[2], process_b_main, "Proc_B");
    create_kernel_process(&kernel_procs[3], idle_main, "Idle");

    running_proc = &kernel_procs[0];
    running_proc->state = RUNNING;

    append(&ready_list, &kernel_procs[1]);
    append(&ready_list, &kernel_procs[2]);
    append(&ready_list, &kernel_procs[3]);

    // assuming we work as a task, we will be switched in multiple times
    int delay = 0;
    while (true) {
        printf("R");

        pushcli();
        schedule();
        popcli();

        if (++delay > 30) {
            printf("\n");
            dump_process_table();
            timer_pause_blocking(3000);
            delay = 0;

            if (kernel_procs[1].state == BLOCKED)
                unblock_task(&kernel_procs[1]);
        }
    }
}

static int switching_ticks = 0;
void test_switching_tick() {
    if (++switching_ticks > 1000) {
        switching_ticks = 0;
        printf("(tick)");
        
        // schedule_another_process();

        // the task is kicking in,
        // after the schedule_another_process() is called,
        // we never return here, therefore the IRQ is never acknowledged,
        // therefore it never fires a second time!
        // some discussion here: https://www.reddit.com/r/osdev/comments/i69bv4/problems_with_preempting_from_timer_interrupt/
        // we could have similar issue when holding a lock, as we "return" into the task,
        // therefore we never release the lock.
        // what to do, what to do!!!
        // what idea is that, the first time, instead of returning into the task,
        // we return to a piece of code of ours that can run the task.
        // maybe this has good info: https://wiki.osdev.org/Brendan%27s_Multi-tasking_Tutorial
    }
}


void dump_process_table() {
    char *states[] = {
        "READY",
        "RUNNING",
        "BLOCKED"
    };
    printf("Process list:\n");
    printf("* i Name       ESP      Entry    State     Blk    CPU  \n");
    for (int i = 0; i < sizeof(kernel_procs) / sizeof(kernel_procs[0]); i++) {
        kernel_proc_t proc = kernel_procs[i];
        printf("%c %d %-10s %08x %08x %-7s %3d %6us\n", 
            (running_proc == &proc) ? '*' : ' ',
            i, 
            proc.name, 
            proc.esp, 
            proc.stack_snapshot->return_address,
            (char *)states[(int)proc.state],
            proc.block_reason,
            (proc.cpu_ticks_total / 1000)
        );
    }
}

void test_switching_context_functionality() {
    switched_stack_snapshot_t snapshot;
    uint32_t esp = 0;
    printf("Before context switch ESP=%08x\n", esp);
    low_level_context_switch(&esp, &esp);
    // save things so we can print them
    // don't call any methods, or we'll overwrite the stack!!!!
    char *p = (char *)&snapshot;
    for (uint32_t i = 0; i < sizeof(snapshot); i++) {
        *p++ = *(((char *)esp) + i);
    }
    printf("After  context switch ESP=%08x\n", esp);
    printf("Saved switch stack snapshot:\n"
        "edi            0x%08x\n"
        "esi            0x%08x\n"
        "ebp            0x%08x\n"
        "ebx            0x%08x\n"
        "edx            0x%08x\n"
        "ecx            0x%08x\n"
        "eax            0x%08x\n"
        "eflags         0x%08x\n"
        "return_address 0x%08x\n",
        snapshot.edi,
        snapshot.esi,
        snapshot.ebp,
        snapshot.ebx,
        snapshot.edx,
        snapshot.ecx,
        snapshot.eax,
        snapshot.eflags,
        snapshot.return_address
    );
}