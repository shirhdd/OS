// thread.h

#ifndef EX2_THREAD_H
#define EX2_THREAD_H

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7
/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address (address_t addr);



#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr) {
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}
#endif



#include <setjmp.h>
#include <csignal>
#define FAIL (-1)
#define SUCCESS 0
#define STACK_SIZE 4096
#pragma once
typedef void (*thread_entry_point)(void);
enum state {
    READY, BLOCKED, RUNNING, BLOCKED_AND_SLEEP, SLEEP
};

class Thread {
private:
    state t_state;
    thread_entry_point entry_point;
    void *stack;
    const int t_id;
    int q_time;
    int s_time;
    sigjmp_buf _env;

public:

    Thread(state tState, void (*entryPoint)(void) , void *stack,
           const int tId, int qTime,
           int sTime) : t_state(tState),
                                                  entry_point(entryPoint),
                                                  stack(stack), t_id(tId),
                                                  q_time(qTime),
                                                  s_time(sTime)
  {
      int is_set_up = setup_thread();
      if(is_set_up == FAIL){
          throw std::exception();
      }

  }
/**
 * initializes _env to use the right stack, and to run from the function 'entry_point', when we'll use
 * siglongjmp to jump into the thread.
 */
    int setup_thread ()
    {
        address_t sp = (address_t) stack + STACK_SIZE - sizeof (address_t);
        address_t pc = (address_t) entry_point;
        sigsetjmp (_env, 1);
        (_env->__jmpbuf)[JB_SP] = translate_address (sp);
        (_env->__jmpbuf)[JB_PC] = translate_address (pc);
        sigemptyset(&(_env->__saved_mask));
        if (sigemptyset(&(_env->__saved_mask)) < 0)
        {
            std::cerr << "SYSTEM_ERROR" << "SIG_EMPTY_SET_ERROR" << std::endl;
            return FAIL;
        }
        return SUCCESS;
    }
/**
 * Destructor function for the Thread class. Frees the memory used by the thread's stack and sets the stack pointer to nullptr.
 */
    virtual ~Thread() {
        free(stack);
        stack = nullptr;
    }
/**
 * Returns the state of the thread.
 *
 * @return The state of the thread.
 */
    state getState() {
        return t_state;
    }
/**
 * Returns a pointer to the sigjmp_buf object associated with the thread.
 *
 * @return A pointer to the sigjmp_buf object associated with the thread.
 */
    sigjmp_buf *getEnv() {
        return &_env;
    }
/**
 * Returns a pointer to the memory used by the thread's stack.
 *
 * @return A pointer to the memory used by the thread's stack.
 */
    void *getStack() const {
        return stack;
    }
    /**
 * Returns a pointer to the entry point function for the thread.
 *
 * @return A pointer to the entry point function for the thread.
 */
    thread_entry_point getEntryPoint(){
        return entry_point;
    }
    /**
 * Sets the stack pointer to a given memory address.
 *
 * @param stack A pointer to the memory address to set the stack pointer to.
 */
    void setStack(void *stack) {
        Thread::stack = stack;
    }
/**
 * Sets the state of the thread to a given value.
 *
 * @param tState The new state of the thread.
 */
    void setState(state tState) {
        t_state = tState;
    }
/**
 * Returns the ID of the thread.
 *
 * @return The ID of the thread.
 */
    const int getId() const {
        return t_id;
    }
/**
 * Returns the "sleep time" of the thread, which is the number of quantum units that the thread should sleep for before being woken up.
 *
 * @return The sleep time of the thread.
 */
    int getSTime()  {
        return s_time;
    }
/**
 * Sets the "sleep time" of the thread to a given value.
 *
 * @param sTime The new sleep time for the thread.
 */
    void setSTime(int sTime)  {
        s_time = sTime;
    }
    /**
 * Returns the quantum time of the thread, which is the number of quantum units that the thread has used so far in its current execution cycle.
 *
 * @return The quantum time of the thread.
 */
    int getQTime()  {
        return q_time;
    }
    /**
     * Increments the quantum time of the thread by 1.
     */
    void incQTime()  {
         q_time+=1;
    }
    /**
     * Decrements the "sleep time" of the thread by 1.
     */
    void decSTime()  {
         s_time-=1;
    }
};
Thread* create_new_thread(thread_entry_point entry_point,int minimal_id, int main);
#endif // THREAD_H
