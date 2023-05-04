#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

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

#include <iostream>
#include <list>
#include <array>
#include <map>
#include <vector>
#include "uthreads.h"
#include <signal.h>
#include <sys/time.h>
#include <setjmp.h>
#include "thread.h"
#include <unordered_set>
#include <algorithm>
#include <queue>
#include <functional>


#define LIBRARY_ERROR "thread library error: "
#define SYSTEM_ERROR "system error: "
#define THREAD_NOT_FOUND "thread doesn't exist"
#define SIGACTION_ERROR "sigaction error"
#define PROC_MASK_ERROR "sigprocmask error"
#define FAIL (-1)
#define SUCCESS 0
#define JUMPING_SIGNAL 55
#define CONVERT_RATIO_SEC_TO_MCS 1000000
#define NUM_OF_THREAD_ERROR "number of thread is greater then max"
#define QUANTUM_USECS_MUST_BE_NON_POSITIVE "quantum Usecs must be positive"
#define ATTEMPT_TO_BLOCK_MAIN_THREAD_ERROR "main thread should not be blocked"
#define ENTRYPOINT_ERROR "illegal entry point - should be defined and not null"
#define JUMP 1
#define QUANTUM_START_TIME 0
#define TIMER_ERROR "settimer error"
#define ATTEMPT_TO_SLEEP_MAIN_THREAD_ERROR "error main thread should not be sleeping"
#define SIG_ADD_SET_ERROR "sigaddset error"
#define MEMORY_ALLOCATION_ERROR "error when trying to allocate new memory"
#define SIGNAL_SETEMPTY_ERROR "sigemptyset error"
#define SLEEP_NUM_QUANTUMS_SHOULD_BE_POSITIVE_ERROR "sleeping input must be positive"
#define CHANGE_SIGNAL_UNBLOCK_CHECK blocking_virtual_time_signal (signals_set, SIG_UNBLOCK) == FAIL
#define CHANGE_SIGNAL_BLOCK_CHECK blocking_virtual_time_signal (signals_set, SIG_BLOCK) == FAIL
#define T_ID_MAIN_THREAD  0
#define MIN_QUANT_ALLOWED 0

void schedule (int sig);
int blocking_virtual_time_signal (sigset_t &signals_set, int signal);
void calling_next_thread ();
std::string error_massage(int signal);
int blocking_virtual_time_signal(sigset_t &signals_set, int signal);
void thread_resuming (int tid);
void thread_blocking (int tid);
void all_delete ();
void thread_deleting (int tid);
int uthread_get_tid ();
std::vector<Thread*> fill_null_pointer();
void  time_config(int quantum_usecs, itimerval &timer_to_change);
void sleepers_care_taking ();
void calling_next_thread ();
void switch_to_upcoming_ready ();
int uthread_get_quantums (int tid);
int uthread_get_total_quantums ();
int uthread_sleep (int num_quantums);
int uthread_resume (int tid);
int uthread_terminate (int tid);
int uthread_spawn (thread_entry_point entry_point);
int t_id_check(int tid);


/**
 * Data structre
 */

std::vector<Thread*> t_ids(MAX_THREAD_NUM);
int quantum_t_global = 1;
int num_threads = 1;
std::vector<Thread *> ready_threads;
std::map<int, Thread *> blocked_threads;
std::unordered_set<Thread*> sleepers_map;
std::priority_queue<int, std::vector<int>, std::greater<int>> minimal_id_heap;
Thread *thread_running = nullptr;
struct itimerval timer{};

/**
 * Utils Functions
 */


/**
 * This function returns an error message corresponding to a given signal value.
 * @param signal
 * @return error msg
 */
std::string error_massage(int signal){
    switch (signal) {
        case SIG_BLOCK:
            return SIGNAL_SETEMPTY_ERROR;
        case SIG_UNBLOCK:
            return SIG_ADD_SET_ERROR;
        case SIG_SETMASK:
            return PROC_MASK_ERROR;
        default:
            return "Unknown error.";
    }
}

/**
 * This function updates the state of a thread that needs to be blocked.
 * If the thread's current state is either SLEEP or BLOCKED_AND_SLEEP, its state is set to BLOCKED_AND_SLEEP.
 * Otherwise, its state is set to BLOCKED.
 *
 * @param thread_to_block A pointer to the thread that needs to be blocked.
 * @param tid The thread ID of the thread being blocked.
 */
void update_states_of_blocked(Thread *thread_to_block, int tid){
    if (thread_to_block->getState() == SLEEP ||
        thread_to_block->getState() == BLOCKED_AND_SLEEP)
    {
        thread_to_block->setState(BLOCKED_AND_SLEEP);
    }
    else
    {
        thread_to_block->setState(BLOCKED);
    }
}

/**
 * Deletes all the threads that are currently running in the program.
 */
void all_delete ()
{
    for (Thread *toDelete: t_ids)
    {
        if (toDelete != nullptr)
        {
            thread_deleting(toDelete->getId());
        }
    }
}

int handle_error(const std::string &err_type, const std::string &err_msg){
    std::cerr << err_type << err_msg << std::endl;
    return FAIL;
}

/**
 * Sets up a signal handler for the virtual timer signal (SIGVTALRM) and blocks any other signals in the signals_set.
 *
 * @param signals_set a reference to a sigset_t object that will be used to block signals
 * @param signal the signal number to be handled
 * @return SUCCESS if the signal handler and signal blocking were set up successfully, FAIL otherwise
 */
int blocking_virtual_time_signal(sigset_t &signals_set, int signal) {
    int result = SUCCESS;
    if(sigemptyset(&signals_set) == FAIL){
        result = FAIL;
        std::cerr << SYSTEM_ERROR << error_massage(signal) << std::endl;
    }

    if (result != FAIL && sigaddset(&signals_set, SIGVTALRM) == FAIL) {
        result = FAIL;
    }

    if (result != FAIL && sigprocmask(signal, &signals_set, nullptr) == FAIL) {
        result = FAIL;
    }
    return result;
}


/**
 * Configures the time intervals of a timer using the given quantum_usecs.
 *
 * @param quantum_usecs the time in microseconds for each interval of the timer
 * @param timer_to_change a reference to the itimerval object to change
 */
void  time_config(int quantum_usecs, itimerval &timer_to_change) {
    int tv_sec = quantum_usecs / CONVERT_RATIO_SEC_TO_MCS;
    int tv_usec = quantum_usecs - (quantum_usecs / CONVERT_RATIO_SEC_TO_MCS) * CONVERT_RATIO_SEC_TO_MCS;

    timer_to_change.it_value.tv_sec = tv_sec;
    timer_to_change.it_value.tv_usec = tv_usec;

    timer_to_change.it_interval.tv_sec = tv_sec;
    timer_to_change.it_interval.tv_usec = tv_usec;
}

/**
 * Creates a vector of pointers to Thread objects, sets all the pointers to null, and returns the vector.
 *
 * @return a vector of pointers to Thread objects with all pointers initialized to null.
 */std::vector<Thread*> fill_null_pointer() {
    std::vector<Thread*> ids(MAX_THREAD_NUM);
    for (size_t i = 0; i < ids.size(); ++i) {
        ids[i] = nullptr;
    }
    return ids;
}
/**
 * Returns the ID of the currently running thread.
 *
 * @return the ID of the currently running thread.
 */
int uthread_get_tid ()
{
    return thread_running->getId();
}

/**
 * transitioning from blocked state to sleeping or ready state
 * @param tid the thread that transitioning
 */
void thread_resuming (int tid)
{
    Thread* thread_to_resume = t_ids[tid];

    switch (thread_to_resume->getState()) {
        case BLOCKED:
            thread_to_resume->setState(READY);
            ready_threads.push_back(thread_to_resume);
            break;
        case BLOCKED_AND_SLEEP:
            thread_to_resume->setState(SLEEP);
            break;
        default:
            break;
    }
    blocked_threads.erase(tid);
}

/**
 * Blocks a specified thread by its ID (tid), removing it from the list of ready threads
 * and adding it to the list of blocked threads.
 *
 * @param tid The thread ID of the thread to be blocked.
 */
void thread_blocking(int tid)
{

    Thread *thread_to_block = t_ids[tid];


    update_states_of_blocked(thread_to_block, tid);

    auto it = std::find(ready_threads.begin(), ready_threads.end(), thread_to_block);

    if (it != ready_threads.end()) {
        ready_threads.erase(it);
    }

    blocked_threads.insert(std::pair<int, Thread *>(tid, thread_to_block));
}

/**
 * @brief Resumes a blocked thread with ID tid and moves it to the READY state.
 *
 * Resuming a thread in a RUNNING or READY state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume (int tid)
{
    if(t_id_check(tid) == FAIL){
        return FAIL;
    }
    sigset_t signals_set;
    if (CHANGE_SIGNAL_BLOCK_CHECK)
    {
        return FAIL;
    }
    Thread *thread_to_resume = t_ids[tid];
    if (thread_to_resume->getState() == BLOCKED ||
        thread_to_resume->getState() == BLOCKED_AND_SLEEP)
    {
        thread_resuming(tid);
    }
    if (CHANGE_SIGNAL_UNBLOCK_CHECK)
    {
        return FAIL;
    }
    return SUCCESS;
}

/**
 * create the main thread
 * @return
 */
int initiate_main ()
{   int min_id = minimal_id_heap.top();
    auto *new_thread = create_new_thread(nullptr, min_id, 1);
    if (new_thread == nullptr)
    {
        return handle_error(SYSTEM_ERROR, MEMORY_ALLOCATION_ERROR);

    }
    minimal_id_heap.pop();
    t_ids[new_thread->getId()] = new_thread;
    sigsetjmp (*new_thread->getEnv(), 1);
    thread_running = new_thread;
    return SUCCESS;
}

/**
 * @brief initializes the thread library.
 *
 * You may assume that this function is called before any other thread library function, and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init (int quantum_usecs)
{
    if (quantum_usecs <= MIN_QUANT_ALLOWED)
    {
        return handle_error(LIBRARY_ERROR, QUANTUM_USECS_MUST_BE_NON_POSITIVE);
    }
    //initilize heap
    for (int i = 0; i < MAX_THREAD_NUM; ++i) {
        minimal_id_heap.push(i);
    }

    t_ids = fill_null_pointer();
    struct sigaction sa = {nullptr};

    sa.sa_handler = &schedule;
    if (sigaction (SIGVTALRM, &sa, nullptr) < 0)
    {
        return handle_error(SYSTEM_ERROR, SIGACTION_ERROR);
    }

    if (initiate_main() == FAIL)
    {
        return FAIL;
    }

    time_config(quantum_usecs, timer);

    if (setitimer (ITIMER_VIRTUAL, &timer, nullptr))
    {
        return handle_error(SYSTEM_ERROR, TIMER_ERROR);
    }
    return SUCCESS;
}

/**
 * @brief Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 * // Jump to the new thread's saved context
 * The thread is added to the end of the READY threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each thread should be allocated with a stack of size STACK_SIZE bytes.
 *
 * @return On success, return the ID of the created thread. On failure, return -1.
*/
int uthread_spawn (thread_entry_point entry_point)
{

    if(entry_point == nullptr) {
        return handle_error(LIBRARY_ERROR,ENTRYPOINT_ERROR);
    }
    if (num_threads == MAX_THREAD_NUM)
    {
        return handle_error(LIBRARY_ERROR,NUM_OF_THREAD_ERROR);
    }
    sigset_t signals_set;
    if (CHANGE_SIGNAL_BLOCK_CHECK)
    {
        return FAIL;
    }
    int min_id = minimal_id_heap.top();
    auto *new_thread = create_new_thread(entry_point, min_id, 0);
    if (new_thread == nullptr)
    {
        return handle_error(SYSTEM_ERROR,MEMORY_ALLOCATION_ERROR);
    }
    minimal_id_heap.pop();
    ready_threads.push_back (new_thread);
    t_ids[new_thread->getId()] = new_thread;
    num_threads++;
    if (CHANGE_SIGNAL_UNBLOCK_CHECK)
    {
        return FAIL;
    }
    return new_thread->getId();
}

/**
 * Deletes the thread associated with the given thread ID (tid).
 *
 * @param tid The thread ID of the thread to be deleted.
 */
void thread_deleting (int tid)
{
    Thread *threadForDelete = t_ids[tid];
    switch (threadForDelete->getState()) {
        case READY: {

            auto it = std::find(ready_threads.begin(), ready_threads.end(),
                                threadForDelete);
            if (it != ready_threads.end()) {
                ready_threads.erase(it);
            }
            break;
        }
        case BLOCKED:
            blocked_threads.erase (thread_running->getId());
            break;
        case SLEEP :
            sleepers_map.erase (threadForDelete);
            break;
        case BLOCKED_AND_SLEEP:
            blocked_threads.erase (thread_running->getId());
            sleepers_map.erase (threadForDelete);
            break;
        default:
            break;
    }
    free (threadForDelete->getStack());
    threadForDelete->setStack(nullptr);
    delete threadForDelete;
    t_ids[tid] = nullptr;
    minimal_id_heap.push(tid);
}

/**
 * This function gets the next thread from the ready list and jumps to it.
 * The thread that is currently running is updated to the next ready thread,
 * and its state is set to RUNNING. The function also increases the quantum time
 * and removes the thread from the ready list. Finally, it performs a non-local
 * jump to the new thread's saved context.
 */
void switch_to_upcoming_ready()
{
    thread_running = ready_threads.front();
    thread_running->setState(RUNNING);
    thread_running->incQTime();
    quantum_t_global += 1;
    ready_threads.erase(ready_threads.begin());
    siglongjmp(*thread_running->getEnv(), JUMP);
}

int t_id_check(int tid){
    if (tid < 0 || tid > MAX_THREAD_NUM || t_ids[tid] == nullptr)
    {
        return handle_error(LIBRARY_ERROR, THREAD_NOT_FOUND);
    }
    return SUCCESS;
}


/**
 * @brief Terminates the thread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this thread should be released. If no thread with ID tid exists it
 * is considered an error. Terminating the main thread (tid == 0) will result in the termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the thread was successfully terminated and -1 otherwise. If a thread terminates
 * itself or the main thread is terminated, the function does not return.
*/
int uthread_terminate (int tid)
{
    if(t_id_check(tid) == FAIL){
            return FAIL;
    }
    sigset_t signals_set;
    if (CHANGE_SIGNAL_BLOCK_CHECK)
    {
        return FAIL;
    }
    if (tid == T_ID_MAIN_THREAD)
    {
        all_delete();
        exit (SUCCESS);
    }
    num_threads--;
    thread_deleting(tid);
    if (uthread_get_tid () == tid)
    {
        schedule (JUMPING_SIGNAL);
    }
    if (CHANGE_SIGNAL_UNBLOCK_CHECK)
    {
        return FAIL;
    }
    return SUCCESS;
}

/**
 * @brief Blocks the thread with ID tid. The thread may be resumed later using uthread_resume.
 *
 * If no thread with ID tid exists it is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision should be made.
 * Blocking a thread in BLOCKED state has no effect and is not considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block (int tid)
{
    if(t_id_check(tid) == FAIL){
        return FAIL;
    }
    if (tid == T_ID_MAIN_THREAD)
    {
        return handle_error(LIBRARY_ERROR, ATTEMPT_TO_BLOCK_MAIN_THREAD_ERROR);
    }
    sigset_t signals_set;
    if (CHANGE_SIGNAL_BLOCK_CHECK)
    {
        return FAIL;
    }
    thread_blocking(tid);
    if (CHANGE_SIGNAL_UNBLOCK_CHECK)
    {
        return FAIL;
    }
    if (uthread_get_tid () == tid)
    {
        schedule (JUMPING_SIGNAL);
    }
    return SUCCESS;
}

/**
 * @brief Blocks the RUNNING thread for num_quantums quantums.
 *
 * Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY threads list.
 * The number of quantums refers to the number of times a new quantum starts, regardless of the reason. Specifically,
 * the quantum of the thread which has made the call to uthread_sleep isn’t counted.
 * It is considered an error if the main thread (tid==0) calls this function.
 *
 * @return On success, return 0. On failure, return -1.
*/

int uthread_sleep (int num_quantums)
{
    if (num_quantums <= MIN_QUANT_ALLOWED)
    {
        std::cerr << LIBRARY_ERROR << SLEEP_NUM_QUANTUMS_SHOULD_BE_POSITIVE_ERROR << std::endl;
    }
    int tid = uthread_get_tid ();
    if (tid == T_ID_MAIN_THREAD)
    {
        return handle_error(LIBRARY_ERROR, ATTEMPT_TO_SLEEP_MAIN_THREAD_ERROR);
    }
    sigset_t signals_set;
    if (CHANGE_SIGNAL_BLOCK_CHECK)
    {
        return FAIL;
    }
    thread_running->setSTime(num_quantums + 1);
    sleepers_map.insert (thread_running);
    if (thread_running->getState() != BLOCKED)
    {
        thread_running->setState(SLEEP);

    }
    else
    {
        thread_running->setState(BLOCKED_AND_SLEEP);
    }
    if (CHANGE_SIGNAL_UNBLOCK_CHECK)
    {
        return FAIL;
    }
    schedule (JUMPING_SIGNAL);
    return SUCCESS;
}




/**
 * @brief Returns the total number of quantums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums ()
{ return quantum_t_global; }

/*
 * @brief Returns the number of quantums the thread with ID tid was in RUNNING state.
 *
 * On the first time a thread runs, the function should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state when this function is called, include
 * also the current quantum). If no thread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums (int tid)
{
    if(t_id_check(tid) == FAIL){
        return FAIL;
    }
    return t_ids[tid]->getQTime();
}

/**
 * This function switches to the next thread in the queue of ready threads.
 * It uses a non-local goto (setjmp/longjmp) to switch to the context of the next thread.
 *
 * @note This function assumes that there is a global variable named "thread_running"
 * that contains a pointer to the currently running thread, and that each thread object
 * has a method called "getEnv()" that returns a pointer to its saved execution context.
 * The JUMP macro is assumed to be defined in a header file included elsewhere in the program.
 *
 * @return void
 */
void calling_next_thread()
{
    int ret_val = sigsetjmp(*thread_running->getEnv(), 1);


    if (ret_val != JUMP) {

        switch_to_upcoming_ready();
    }
}

/**
 * This function handles the threads that are currently in the SLEEP state.
 * It decreases the sleep time for each sleeping thread, and if a thread's sleep time
 * reaches 0, it is moved to the READY state and added to the ready_threads list.
 * If the thread is in BLOCKED_AND_SLEEP state, it is moved to the BLOCKED state.
 */
void sleepers_care_taking()
{
    std::list<Thread*> ids_to_wakeup{};
    for (Thread* sleeping_thread : sleepers_map)
    {
        sleeping_thread->decSTime();
        if (sleeping_thread->getSTime() == 0)
        {
            ids_to_wakeup.push_back(sleeping_thread);
        }
    }
    for (Thread* thread_to_wakeup : ids_to_wakeup)
    {
        if (thread_to_wakeup->getState() == SLEEP)
        {
            thread_to_wakeup->setState(READY);
            ready_threads.push_back(thread_to_wakeup);
        }
        else
        {
            thread_to_wakeup->setState(BLOCKED);
        }
        sleepers_map.erase(thread_to_wakeup);
    }
}

/**
 * This function is called whenever a signal is received.
 * It uses a non-local goto (setjmp/longjmp) to switch between threads,
 * and maintains a queue of ready threads.
 *
 * @note This function assumes that there is a global variable named "thread_running"
 * that contains a pointer to the currently running thread, and that each thread object
 * has a method called "getEnv()" that returns a pointer to its saved execution context.
 * It also assumes the existence of certain macros that are not defined in the provided code.
 *
 * @param sig the signal number
 *
 * @return void
 */
void schedule (int sig)
{
    setitimer (ITIMER_VIRTUAL, &timer, nullptr);
    sigset_t signals_set;
    if (CHANGE_SIGNAL_BLOCK_CHECK)
    {
        return;
    }
    sleepers_care_taking();
    if (sig == SIGVTALRM)
    {
        int ret_val = sigsetjmp (*thread_running->getEnv(), 1);
        if (ret_val != JUMP)
        {
            ready_threads.push_back (thread_running);
            thread_running->setState(READY);

            switch_to_upcoming_ready();
        }
    }
    else
    {
        calling_next_thread();
    }
    if (CHANGE_SIGNAL_UNBLOCK_CHECK)
    {
        return;
    }
}
