#include <cstdlib>
#include <cassert>
#include "Thread.hpp"
#include "Scheduler.hpp"
#include "PerformanceTools.hpp"

#define STACK_SIZE 2<<18

std::ostream& operator<< ( std::ostream& o, const ThreadQueue& tq ) {
    tq.dump( o );
}

Thread * thread_init() {
  coro* me = coro_init();
  Thread * master = (Thread *)malloc(sizeof(Thread));
  assert(master != NULL);
  master->co = me;
  master->sched = NULL;
  master->next = NULL;
  master->id = 0; // master always id 0
  master->done = 0;
  
  TAU_PROFILE_STMT( master->tau_taskid=0 );

  return master;
}

// keeps track of last id assigned
TAU_PROFILE_STMT( int thread_last_tau_taskid=0 );

static void tramp(struct coro * me, void * arg) {
  // Pass control back and forth a few times to get the info we need.
  coro* master = (coro *)arg;
  Thread* my_thr = (Thread *)coro_invoke(me, master, NULL);
  thread_func f = (thread_func)coro_invoke(me, master, NULL);
  void* f_arg = coro_invoke(me, master, NULL);
  // Next time we're invoked, it'll be for real.
  coro_invoke(me, master, NULL);

  StateTimer::setThreadState( StateTimer::SYSTEM );
  StateTimer::enterState_system();
  
  // create new Tau task, and top level timer for the task
  int new_taskid;
  TAU_CREATE_TASK(new_taskid);
  TAU_PROFILE_STMT( my_thr->tau_taskid = new_taskid );
  TAU_PROFILE_STMT( thread_last_tau_taskid = new_taskid );
  GRAPPA_PROFILE_CREATE( mainprof, "start_thread", "()", TAU_USER1 );
  GRAPPA_PROFILE_THREAD_START( mainprof, my_thr );

  // call thread target function
  f(my_thr, f_arg);

  // stop top level Tau task timer
  GRAPPA_PROFILE_THREAD_STOP( mainprof, my_thr );

  // We shouldn't return, but if we do, kill the Thread.
  thread_exit(my_thr, NULL);
}

/// Spawn a new Thread belonging to the Scheduler.
/// Current Thread is parent. Does NOT enqueue into any scheduling queue.
Thread * thread_spawn(Thread * me, Scheduler * sched,
                     thread_func f, void * arg) {
  assert( sched->get_current_thread() == me );
 
  // allocate the Thread and stack
  Thread * thr = new Thread( sched );
  thr->co = coro_spawn(me->co, tramp, STACK_SIZE);
  sched->assignTid( thr );

  // Pass control to the trampoline a few times quickly to set up
  // the call of <f>.  Could also create a struct with all this data?
  
  
  coro_invoke(me->co, thr->co, (void *)me->co);
  coro_invoke(me->co, thr->co, thr);
  coro_invoke(me->co, thr->co, (void *)f);
  coro_invoke(me->co, thr->co, (void *)arg);
  
  
  return thr;
}

void thread_exit(Thread * me, void * retval) {

  // Reuse the queue field for a return value.
  me->next = (Thread *)retval;

  // process join
  me->done = 1;
  Thread * waiter = me->joinqueue.dequeue();
  while (waiter!=NULL) {
    me->sched->thread_wake( waiter );
    waiter = me->joinqueue.dequeue();
  }
  
  // call master Thread
  me->sched->thread_on_exit();
 
  // never returns
  exit(EXIT_FAILURE);
}

void destroy_thread(Thread * thr) {
  destroy_coro(thr->co);
  free (thr);
}

