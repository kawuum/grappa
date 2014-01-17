
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <iostream>
#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "Metrics.hpp"
#include "Delegate.hpp"
#include "Collective.hpp"
#include "HistogramMetric.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"
#include "PerformanceTools.hpp"

BOOST_AUTO_TEST_SUITE( Metrics_tests );

using namespace Grappa;

//equivalent to: static SimpleMetric<int> foo("foo", 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<int>, foo, 0);

//equivalent to: static SimpleMetric<double> bar("bar", 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, bar, 0);

GRAPPA_DEFINE_METRIC(SummarizingMetric<int>, baz, 0);

GRAPPA_DEFINE_METRIC(HistogramMetric, rand_msg, 0);

GRAPPA_DEFINE_METRIC(MaxMetric<uint64_t>, maz, 0);

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    CHECK(Grappa::cores() >= 2); // at least 2 nodes for these tests...

    foo++;
    bar = 3.14;

    baz += 1;
    baz += 4;
    baz += 9;

    delegate::call(1, []() -> bool {
      foo++;
      foo++;
      bar = 5.41;
      baz += 16;
      baz += 25;
      baz += 36;

      BOOST_CHECK( baz.value() == (16+25+36) );
      BOOST_CHECK( foo.value() == 2 );
      BOOST_CHECK( bar.value() == 5.41 );

      return true;
    });
  
    Metrics::print();

    delegate::call(1, []() -> bool {
      Metrics::print();
      return true;
    });

    Metrics::reset_all_cores();
    Metrics::start_tracing();

#ifdef HISTOGRAM_SAMPLED
    VLOG(1) << "testing histogram sampling";
    int64_t N = 1<<20;  
    auto xs = Grappa::global_alloc<int64_t>(N);
    forall(xs, N, [N](int64_t i, int64_t& x) { x = rand() % N; });
  
    on_all_cores([xs,N]{
      for (int64_t i=0; i<N; i++) {
        rand_msg = delegate::read(xs+i);
      }
    });
#endif

    on_all_cores([] {
        maz.add( (Grappa::mycore()+Grappa::cores()-1)%Grappa::cores() );
        maz.add( 1 );
    });

    Metrics::stop_tracing();
    Metrics::merge_and_print();
    //Metrics::dump_stats_blob();
  
    call_on_all_cores([]{ Metrics::reset(); });
    Metrics::merge_and_print();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
