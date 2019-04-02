#define BENCHMARK "OSU UPC++ Broadcast Latency Test"
/*
 * Copyright (C) 2002-2015 the Network-Based Computing Laboratory
 * (NBCL), The Ohio State University.
 *
 * Contact: Dr. D. K. Panda (panda@cse.ohio-state.edu)
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level OMB directory.
 */

#include <stdio.h>
#include <upcxx/upcxx.hpp>
#include <upcxx/backend.hpp>
#include <upcxx/allocate.hpp>
#include <upcxx/broadcast.hpp>
#include <stdlib.h>
extern "C" {
#include <osu_common.h>
#include <osu_coll.h>
}
using namespace std;
using namespace upcxx;

#define root 0
#define VERIFY 0

int
main (int argc, char **argv)
{
  upcxx::init(); 

  double avg_time, max_time, min_time;
  int i = 0, size;
  int skip;
  int64_t t_start = 0, t_stop = 0, timer=0;
  int max_msg_size = 1<<20, full = 0;

  if (process_args(argc, argv, rank_me(), &max_msg_size, &full, HEADER)) {
    return 0;
  }

  std::vector<char> src((max_msg_size*sizeof(char)));
  std::vector<char> dst((max_msg_size*sizeof(char))); 
  double time_src;
  double time_dst;

  if (rank_n() < 2) {
    if (rank_me() == 0) {
      fprintf(stderr, "This test requires at least two processes\n");
    }
    return -1;
  }


  /*
   * put a barrier
   */
  barrier();

  print_header(HEADER, rank_me(), full);

  for (size=1; size <=max_msg_size; size *= 2) {
    if (size > LARGE_MESSAGE_SIZE) {
      skip = SKIP_LARGE;
      iterations = iterations_large;
    } else {
      skip = SKIP;
    }

    timer=0;
    for (i=0; i < iterations + skip ; i++) {
      t_start = getMicrosecondTimeStamp();
      upcxx::broadcast<char>(src.data(), size*sizeof(char), root);
      t_stop = getMicrosecondTimeStamp();

      if (i>=skip) {
	timer+=t_stop-t_start;
      }
      barrier();
    }

    barrier();

    double* lsrc = &time_src;
    lsrc[0] = (1.0 * timer) / iterations;

    upcxx::reduce_one<double>(&time_src, &time_dst, 1,
			      upcxx::op_fast_max, root);
    if (rank_me()==root) {
      double* ldst = &time_dst;
      max_time = ldst[0];
    }

    upcxx::reduce_one<double>(&time_src, &time_dst, 1,
			      upcxx::op_fast_min, root);
    if (rank_me()==root) {
      double* ldst = &time_dst;
      min_time = ldst[0];
    }

    upcxx::reduce_one<double>(&time_src, &time_dst, 1,
			      upcxx::op_fast_add, root);
    if (rank_me()==root) {
      double* ldst = &time_dst;
      avg_time = ldst[0]/rank_n();
    }     
    barrier();
    print_data(rank_me(), full, size*sizeof(char), avg_time, min_time,
	       max_time, iterations);
  }


  upcxx::finalize();

  return EXIT_SUCCESS;
}

/* vi: set sw=4 sts=4 tw=80: */
