// Sample mystery program to measure how long an add takes. Flawed.
// Copyright 2021 Richard L. Sites

#include <stdint.h>
#include <stdio.h>
#include <time.h>		// time()
#include "timecounters.h"

// const unsigned long kIterations1 = 328ul * 20000000ul;
// const unsigned long kIterations2 = 672ul * 20000000ul;

const unsigned long kIterations1 = 100000000000ul;
const unsigned long kIterations2 = 200000000000ul;

int main (int argc, const char** argv) { 
  unsigned long sum = 0;

  time_t t = time(NULL);	// A number that the compiler does not know
  unsigned long incr = t & 255;		// Unknown increment 0..255

  int64_t startcy1 = GetCycles();
  for (unsigned long i = 0; i < kIterations1; i += 1) {
    asm volatile("" : "+g"(incr));
    sum += incr;
  }
  int64_t elapsed1 = GetCycles() - startcy1;


  int64_t startcy2 = GetCycles();
  for (unsigned long i = 0; i < kIterations2; i += 1) {
    asm volatile("" : "+g"(incr));
    sum += incr;
  }
  int64_t elapsed2 = GetCycles() - startcy2;

  double felapsed = elapsed2 - elapsed1;

  unsigned long adjustedIterations = kIterations2 - kIterations1;

  fprintf(stdout, "%lu iterations, %lu cycles, %4.2f cycles/iteration\n", 
          adjustedIterations, elapsed2 - elapsed1, felapsed / adjustedIterations);

  fprintf(stdout, "VCT Freq: %lu\n", GetFreq());
  
  fprintf(stdout, "%lu %lu\n", t, sum);	// Make sum live

  return 0;
}
