// Sample mystery program to measure how long an add takes. Flawed.
// Copyright 2021 Richard L. Sites

#include <stdint.h>
#include <stdio.h>
#include <time.h>		// time()
#include "timecounters.h"

// const unsigned long kIterations1 = 328ul * 20000000ul;
// const unsigned long kIterations2 = 672ul * 20000000ul;

const unsigned long kIterations1 = 100000000ul;
const unsigned long kIterations2 = 2000000000ul;

#define TEST_LATENCY(T, init, expr) {	\
  T accum = init; \
  int64_t startcy1 = GetCycles(); \
  for (unsigned long i = 0; i < kIterations1; i += 1) { \
    asm volatile("" : "+g"(incr)); \
    expr;			   \
  } \
  int64_t elapsed1 = GetCycles() - startcy1; \
  int64_t startcy2 = GetCycles(); \
  for (unsigned long i = 0; i < kIterations2; i += 1) { \
    asm volatile("" : "+g"(incr)); \
    expr;			   \
  } \
  int64_t elapsed2 = GetCycles() - startcy2; \
  double felapsed = elapsed2 - elapsed1; \
  fprintf(stdout, "%lu iterations, %lu cycles, %4.2f cycles/iteration\n", \
          adjustedIterations, elapsed2 - elapsed1, felapsed / adjustedIterations); \
  fprintf(stdout, "%lu\n", accum); \
  }


int main (int argc, const char** argv) { 
  time_t t = time(NULL);	// A number that the compiler does not know
  unsigned long incr = (t & 255) | 0b1;	// Unknown increment 0..255
  double incr_d = ((double)incr) / 255.0;

  unsigned long adjustedIterations = kIterations2 - kIterations1;

  TEST_LATENCY(uint64_t, 0, accum += incr);
  TEST_LATENCY(uint64_t, 1, accum *= incr);
  TEST_LATENCY(uint64_t, UINT64_MAX, accum /= incr);
  TEST_LATENCY(double, 0.0, accum += incr_d);
  TEST_LATENCY(double, 1.0, accum *= incr_d);
  TEST_LATENCY(double, 1.0, accum /= incr_d);

  return 0;
}
