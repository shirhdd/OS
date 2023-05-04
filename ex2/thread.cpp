
#include <vector>
#include <iostream>
#include "thread.h"
#define SLEEPING_START_TIME 0
#define STACK_SIZE 4096
#define QUANTUM_START_TIME 0
#define MAIN_QUANTUM_START_TIME 1


/**
 * "Translates" a memory address by performing some bitwise operations on it.
 *
 * @param addr The memory address to be translated.
 * @return The translated memory address.
 */
address_t translate_address (address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
            : "=g" (ret)
            : "0" (addr));
    return ret;
}
/**
 * Creates a new thread object and returns a pointer to it.
 *
 * @param entry_point The entry point function for the new thread.
 * @param minimal_id The minimal ID for the new thread.
 * @param main Whether or not the new thread is the main thread.
 * @return A pointer to the newly created thread object, or nullptr if the creation failed.
 */
Thread* create_new_thread(thread_entry_point entry_point,int minimal_id, int main){
    int q_time = QUANTUM_START_TIME;
    state thread_state = READY;
    if(main == 1 ){
        thread_state = RUNNING;
        q_time = MAIN_QUANTUM_START_TIME;
    }
    void *new_stack = (void *) malloc (4096);
    if (new_stack == nullptr)
    {
        return nullptr;
    }
    try {
        return new(std::nothrow) Thread(thread_state, entry_point,
                                        new_stack, minimal_id,
                                        q_time, SLEEPING_START_TIME);
    }
    catch (const std::exception& ex){
        return nullptr;
    }
}

