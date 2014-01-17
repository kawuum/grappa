
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Semaphore.hpp"
#include "Metrics.hpp"
#include "TaskingScheduler.hpp"

BOOST_AUTO_TEST_SUITE( Semaphore_tests );

using namespace Grappa;

void yield() {
  Worker * thr = impl::global_scheduler.get_current_thread();
  spawn( [thr] {
      impl::global_scheduler.thread_wake( thr );
    });
  impl::global_scheduler.thread_suspend();
}


BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    CountingSemaphore s(1);

    // bit vector
    int data = 1;
    int count = 0;
    CompletionEvent ce(6);
    
    spawn([&s,&data,&count,&ce]{
      for( int i = 0; i < 3; ++i ) {
        decrement( &s );
        VLOG(1) << "Task 1 running.";
        data <<= 1;
        data |= 1;
        count++;
        ce.complete();
        increment( &s );
        yield();
      }
    });

    spawn([&s,&data,&count,&ce]{
      for( int i = 0; i < 3; ++i ) {
        decrement( &s );
        VLOG(1) << "Task 2 running.";
        data <<= 1;
        count++;
        ce.complete();
        increment( &s );
        yield();
      }
    });

    ce.wait();    
    

    BOOST_CHECK_EQUAL( count, 6 );
  
    Metrics::merge_and_dump_to_file();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();

