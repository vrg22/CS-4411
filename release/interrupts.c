#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <ucontext.h>
#include <semaphore.h>
#include "defs.h"
#include "interrupts.h"
#include "interrupts_private.h"
#include "minithread.h"
#include "assert.h"
#include "machineprimitives.h"

#define MAXEVENTS 64
#define DISK_INTERRUPT_TYPE 4
#define READ_INTERRUPT_TYPE 3
#define NETWORK_INTERRUPT_TYPE 2
#define CLOCK_INTERRUPT_TYPE 1
#define MAXBUF 1000
#define ENABLED 1
#define DISABLED 0

interrupt_level_t interrupt_level;
long ticks;
extern int start();
extern int end();

/*
 * Virtual processor interrupt level (spl).
 * Are interrupts enabled? A new interrupt will only be taken when interrupts
 * are enabled.
 */
interrupt_level_t interrupt_level;

typedef struct interrupt_t interrupt_t;
struct interrupt_t {
  interrupt_handler_t handler;
  void *arg;
};

static pthread_mutex_t signal_mutex;

#define R8 0
#define R9 1
#define R10 2
#define R11 3
#define R12 4
#define R13 5
#define R14 6
#define R15 7
#define RDI 8
#define RSI 9
#define RBP 10
#define RBX 11
#define RDX 12
#define RAX 13
#define RCX 14
#define RSP 15
#define RIP 16
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
       } while (0)


interrupt_handler_t mini_clock_handler;
interrupt_handler_t mini_network_handler;
interrupt_handler_t mini_read_handler;
interrupt_handler_t mini_disk_handler;

static volatile int signal_handled = 0;

sem_t interrupt_received_sema;

/*
 * atomically sets interrupt level and returns the original
 * interrupt level
 */
interrupt_level_t set_interrupt_level(interrupt_level_t newlevel) {
    return swap(&interrupt_level, newlevel);
}


/*
 * Register the minithread clock handler by making
 * mini_clock_handler point to it.
 *
 * Then set the signal handler for SIGRTMAX-1 to
 * handle_interrupt.  This signal handler will either
 * interrupt the minithreads, or drop the interrupt,
 * depending on safety conditions.
 *
 * The signals are handled on their own stack to reduce
 * chances of an overrun.
 */
void
minithread_clock_init(int period, interrupt_handler_t clock_handler){
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    struct sigaction sa;
    stack_t ss;
    mini_clock_handler = clock_handler;

    sem_init(&interrupt_received_sema,0,0);

    ss.ss_sp = malloc(SIGSTKSZ);
    if (ss.ss_sp == NULL){
        perror("malloc.");
        abort();
    }
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) == -1){
        perror("signal stack");
        abort();
    }

    if(DEBUG)
        printf("SIGRTMAX = %d\n",SIGRTMAX);

    /* Establish handler for timer signal */
    sa.sa_handler = (void*)handle_interrupt;
    sa.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
    sa.sa_sigaction= (void*)handle_interrupt;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask,SIGRTMAX-1);
    sigaddset(&sa.sa_mask,SIGRTMAX-2);
    if (sigaction(SIGRTMAX-1, &sa, NULL) == -1)
        errExit("sigaction");

    /* Create the timer */
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMAX-1;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCK_THREAD_CPUTIME_ID, &sev, &timerid) == -1)
        errExit("timer_create");

    /* Start the timer */
    its.it_value.tv_sec = (period) / 1000000000;
    its.it_value.tv_nsec = (period) % 1000000000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if (timer_settime(timerid, 0, &its, NULL) == -1)
        errExit("timer_settime");
}


/*
 * This function handles a signal and invokes the specified interrupt
 * handler, ensuring that signals are unmasked first.
 */
void
handle_interrupt(int sig, siginfo_t *si, ucontext_t *ucontext)
{
    uint64_t eip = ucontext->uc_mcontext.gregs[RIP];
    /*
     * This allows us to check the interrupt level
     * and effectively block other signals.
     *
     * We can check the interrupt level and disable
     * them, which will prevent other signals from
     * interfering with our other check for library
     * calls.
     */
    if(interrupt_level==ENABLED &&
            eip > (uint64_t)start &&
            eip < (uint64_t)end){

        unsigned long *newsp;
        /*
         * push the return address
         */
        newsp = (unsigned long *) ucontext->uc_mcontext.gregs[RSP];
        *--newsp =(unsigned long)ucontext->uc_mcontext.gregs[RIP];

        /*
         * make room for saved state and align stack.
         */
#define ROUND(X,Y)   (((unsigned long)X) & ~(Y-1)) /* Y must be a power of 2 */
        newsp = (unsigned long *) ROUND(newsp, 16);
        if(ucontext->uc_mcontext.fpregs!=0){
            newsp -= sizeof(struct _fpstate)/sizeof(long);
            memcpy(newsp,ucontext->uc_mcontext.fpregs,sizeof(struct _fpstate));
            ucontext->uc_mcontext.fpregs = (void *)newsp;
        }

        *--newsp = (unsigned long)ucontext->uc_mcontext.gregs[RSP] - sizeof(unsigned long); /*address of RIP*/
        newsp -= sizeof(struct sigcontext)/sizeof(long);
        memcpy(newsp,&ucontext->uc_mcontext,sizeof(struct sigcontext));
        *--newsp = (unsigned long)ucontext->uc_mcontext.fpregs;
        *--newsp = (unsigned long)minithread_trampoline; /*return address*/

        /*
         * set the context so that we end up in the student's clock handler
         * and our stack pointer is at the return address we just pushed onto
         * the stack.
         */
        if(sig==SIGRTMAX-2){
            ucontext->uc_mcontext.gregs[RSP]=(unsigned long)newsp;
            ucontext->uc_mcontext.gregs[RIP]=(unsigned long)((interrupt_t*)si->si_value.sival_ptr)->handler;
            ucontext->uc_mcontext.gregs[RDI]=(unsigned long)((interrupt_t*)si->si_value.sival_ptr)->arg;
            set_interrupt_level(DISABLED);
        }
        else if(sig==SIGRTMAX-1){
            ucontext->uc_mcontext.gregs[RSP]=(unsigned long)newsp;
            ucontext->uc_mcontext.gregs[RIP]=(unsigned long)mini_clock_handler;
            ucontext->uc_mcontext.gregs[RDI]=(unsigned long)0;
            if(DEBUG)
                printf("SP=%p\n",newsp);
        }
        else {
            printf("UNKNOWN SIGNAL\n");
            fflush(stdout);
            abort();
        }
        if(sig==SIGRTMAX-2)
            signal_handled = 1;
    }

    if(sig==SIGRTMAX-2){
        if(DEBUG)
            printf("Signal received\n");
        sem_post(&interrupt_received_sema);
    }
}

void send_interrupt(int interrupt_type, interrupt_handler_t handler, void* arg){

    interrupt_t interrupt;
    pthread_mutex_lock(&signal_mutex);
    for (;;){
        signal_handled = 0;

        interrupt.arg = arg;
        if(interrupt_type==NETWORK_INTERRUPT_TYPE)
            interrupt.handler = mini_network_handler;
        else if(interrupt_type==READ_INTERRUPT_TYPE)
            interrupt.handler = mini_read_handler;
        else if(interrupt_type==DISK_INTERRUPT_TYPE)
            interrupt.handler = mini_disk_handler;
        else
            abort();

        /* Repeat if signal is not delivered. */
        while(sigqueue(getpid(),SIGRTMAX-2, (union sigval)(void*)&interrupt)==-1);

        /* semaphore_P to wait for main thread signal */
        sem_wait(&interrupt_received_sema);

        /* Check if interrupt was handled */
        if(signal_handled)
            break;

        sleep(0);
        /* resend if necessary */
    }
    pthread_mutex_unlock(&signal_mutex);
}
