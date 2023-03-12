% Understanding Software Dynamics
% Exercises and Notes

# Chapter 2 - Measuring CPUs

## Ex. 1

Estimate latency for a single add instruction: $1$ cycle

## Ex. 2

```
debian@debian:~/co/sw_dynamics-book_code/exercises$ ./build/mystery1_0 
1000000000 iterations, 1779265420 cycles, 1.78 cycles/iteration
debian@debian:~/co/sw_dynamics-book_code/exercises$ ./build/mystery1_2 
1000000000 iterations, 0 cycles, 0.00 cycles/iteration
```

So what happened here? Likely the for loop was removed by the optimizer since the accumulator is never read outside the loop.

## Ex. 3

After uncommenting the printf at the end of mystery1.cc, which makes `sum` live, preventing the optimizer from eliding the loop.

```
debian@debian:~/co/sw_dynamics-book_code/exercises$ ./build/mystery1_sum_live_0 
1000000000 iterations, 1784986105 cycles, 1.78 cycles/iteration
1678599241 73000000000
debian@debian:~/co/sw_dynamics-book_code/exercises$ ./build/mystery1_sum_live_2
1000000000 iterations, 0 cycles, 0.00 cycles/iteration
1678599244 76000000000
```

Still looks like $0$ cycles, but why?

```
// ../2/mystery1_sum_live.cc:24:   fprintf(stdout, "%d iterations, %lu cycles, %4.2f cycles/iteration\n", 
	mov	w21, 51712	// tmp123,
	adrp	x1, .LC0	// tmp125,
	movk	w21, 0x3b9a, lsl 16	// tmp123,,
	add	x1, x1, :lo12:.LC0	//, tmp125,
```

The above code constructs the iteration count, but not until the first `fprintf`. Then for the second `fprintf`:

```
// ../2/mystery1_sum_live.cc:15:   int incr = t & 255;		// Unknown increment 0..255
	and	w3, w19, 255	// incr, t,
// ../2/mystery1_sum_live.cc:27:   fprintf(stdout, "%lu %lu\n", t, sum);	// Make sum live
	mov	x2, x19	//, t
	ldr	x0, [x20]	//, stdout
	adrp	x1, .LC1	// tmp133,
	smull	x3, w3, w21	//, incr, tmp123
```

Which simply calculates the sum by multiplying the number of iterations by the increment value. No need for a loop at all, so the loop is removed!

## Ex. 4

After declaring `incr` as `volatile`

```
debian@debian:~/co/sw_dynamics-book_code/exercises$ ./build/mystery1_volatile_incr_0 
1000000000 iterations, 1889502830 cycles, 1.89 cycles/iteration
1678599309 141000000000
debian@debian:~/co/sw_dynamics-book_code/exercises$ ./build/mystery1_volatile_incr_2
1000000000 iterations, 2148560410 cycles, 2.15 cycles/iteration
1678599312 144000000000
```

This looks better. Presumably the volatile keyword hints to the compiler that `icr`'s value might change at any time, so it can't assume that `sum = kIterations * incr`, even though that turns out to be the case.

Assembly output supports that, as there we see the loop once again in the optimized output:

```
.L2:
// ../2/mystery1_volatile_incr.cc:19:     sum += incr;
	ldr	w2, [sp, 60]	//, incr
// ../2/mystery1_volatile_incr.cc:18:   for (int i = 0; i < kIterations; ++i) {
	subs	w1, w1, #1	// ivtmp_7, ivtmp_7,
// ../2/mystery1_volatile_incr.cc:19:     sum += incr;
	add	x19, x19, x2, sxtw	// sum, sum, _13
// ../2/mystery1_volatile_incr.cc:18:   for (int i = 0; i < kIterations; ++i) {
	bne	.L2		//,
```

But why the slightly increased latency on the optimized version? Doesn't make a ton of sense since the loop body is still pretty heavily optimized compared to the `-O0` version.

The difference in the cycle count estimate is substantial (roughly $0.14$), though both estimates are about double the real answer.

## Ex. 5

Produced a cycle estimate of $.99$ cycles per `add`, which matches expectations for ARM-8.

### Modifications in mystery1_improvements.cc

- Two loops w/ diff numbers of iterations. Then we subtract the latency of the shorter from the longer, giving us a measurement that elides the loop overhead.
- More iterations by a factor of $10$. This probably smooths out any cache effects? Note entirely sure, but more iterations seems better.
- Make everything `unsigned long` to avoid inline sign extensions and the like.
  - The result is that our adds looks like `add	x19, x19, x1`
- Add loop unrolling (8 adds per loop iteration)
  - This is probably unnecessary because we're subtracting the latencies of the two loops.
  - NB: Removing unrolling has some effect on the result. It's less consistent +/- .02 cycles/add
- Add `asm volatile("" : "+g"(incr))` before every increment. This is a neat trick that suppresses certain loop optimization which would decrease either the number of iterations or the number of `add`s or both
  - see https://stackoverflow.com/a/14449998
- Also updated `timecounters.h` to match the target platform
  - updates scale factor to match observed 24MHz counter frequency and 3480MHz CPU clock of Apple M1

## Ex. 6

- $k1 = 1 \; k2 = 2 \rightarrow \; 0.0$ cycles/iteration
- $k1 = 10 \; k2 = 20 \rightarrow \; 0.0$  cycles/iteration
  - weirdly got one negative value, so the shorter loop took a bit longer possibly.
- $k1 = 100\; k2 = 200 \rightarrow \; 1.45$  cycles/iteration
- $k1 = 1000 \; k2 = 2000 \rightarrow \; 1.74$ cycles/iteration
  - varies significantly on successive runs. up to $>3$.
- $k1 = 1000 \; k2 = 1100 \rightarrow \; ?$ cycles/iteration
  - arbitrary values between $0$ and $>4$
- $k1 = 10000 \; k2 = 11000 \rightarrow \; ?$ cycles/iteration
  - same as above
- $k1 = 10000 \; k2 = 11000 \rightarrow \; ?$ cycles/iteration
- $k1 = 10000 \; k2 = 20000 \rightarrow \; 1.97$ cycles/iteration
  - Seems that doubling the value between the two loops produces a more consistent result.
- $k1 = 100000 \; k2 = 200000 \rightarrow \; ?$ cycles/iteration
  - Seems pretty arbitrary again, but settles back to 1.98.
  - This could have something to do with the increment seed?
- $k1 = 1000000 \; k2 = 2000000 \rightarrow \; ?$ cycles/iteration
- $k1 = 10000000 \; k2 = 20000000 \rightarrow \; 2.6$ cycles/iteration
- $k1 = 100000000 \; k2 = 200000000 \rightarrow \; .56$ cycles/iteration
- $k1 = 1000000000 \; k2 = 2000000000 \rightarrow \; .87$ cycles/iteration
- $k1 = 10000000000 \; k2 = 20000000000 \rightarrow \; .98$ cycles/iteration
  - Note that this is the approximate order of magnitude of the original improved experiments
- $k1 = 100000000000 \; k2 = 200000000000 \rightarrow \; 1.00$ cycles/iteration
  - Pretty perfect estimate but it takes a really long time.

Thoughts in no particular order:

- No guarantee (as far as I know) that our loop doesn't get interrupted by the scheduler or something. With a smaller number of iterations that would have an outsized effect on the overall cycle count for one or the other loop.

## Ex.7

Order of magnitude estimates for additional operations

- int64 multiply: 10
- int64 divide:10
- double add: 1
- double multiply: 10
- double divide: 10

## Ex. 8

```
debian@debian:~/co/sw_dynamics-book_code/exercises$ ./build/mystery1_operations_2
- int64 add:  1900000000 iterations, 1836207500 cycles, 0.97 cycles/iteration
- int64 mul:  1900000000 iterations, 5810037770 cycles, 3.06 cycles/iteration
- int64 div:  1900000000 iterations, 13549043020 cycles, 7.13 cycles/iteration
- double add: 1900000000 iterations, 5800610595 cycles, 3.05 cycles/iteration
- double mul: 1900000000 iterations, 7711530650 cycles, 4.06 cycles/iteration
- double div: 1900000000 iterations, 19311415230 cycles, 10.16 cycles/iteration
```

These instructions latencies all line up with those listed in the ARM Cortex-A75 optimization guide, which happens to appear at the top of google. The M2 chip is a bit beefier than this one, but the Cortex A75 is a fairly modern ARM chip that implements ARM-v8, so using the values therein as a post facto sanity check seems reasonable enough.

# Chapter 3 - Measuring Memory

## Ex. 1

Cache line (stride) results:

```
stride[16] naive 19 cy/ld, linear 47 cy/ld, scrambled 21 cy/ld
stride[32] naive 5 cy/ld, linear 7 cy/ld, scrambled 29 cy/ld
stride[64] naive 6 cy/ld, linear 14 cy/ld, scrambled 61 cy/ld
stride[128] naive 9 cy/ld, linear 31 cy/ld, scrambled 156 cy/ld
stride[256] naive 17 cy/ld, linear 79 cy/ld, scrambled 149 cy/ld
stride[512] naive 7 cy/ld, linear 77 cy/ld, scrambled 94 cy/ld
stride[1024] naive 7 cy/ld, linear 53 cy/ld, scrambled 151 cy/ld
stride[2048] naive 15 cy/ld, linear 82 cy/ld, scrambled 138 cy/ld
stride[4096] naive 14 cy/ld, linear 94 cy/ld, scrambled 187 cy/ld
```

Not getting super clean measurements on the M2, but I'd probably guess 64B or 128B. For this specific run, we see that strides of 128B and 256B produce roughly the same cy/ld value, which suggests that they correspond to the true L1 miss penalty, which in turn suggests a 128B cache line. Other runs exhibit a similar effect between 64B and 128B stride length, but the general pattern favors a 128B cache line.

NOTE: The true cache line size on this CPU is 128B.

## Ex. 2

Test of the exercise is to look at the 256B potential cache line size, but I think 512B would be more in the spirit here because the sample servers in the book have a 64B cache line whereas my M2 mac has 128B lines.

```
stride[512] naive 7 cy/ld, linear 67 cy/ld, scrambled 145 cy/ld
stride[512] naive 7 cy/ld, linear 82 cy/ld, scrambled 118 cy/ld
stride[512] naive 6 cy/ld, linear 70 cy/ld, scrambled 118 cy/ld
```

- As expected, naive scheme is way too fast. Though we know our stride has exceeded the cache line size by quite a lot, prefetching, parallel memory reads, and possibly other CPU optimizations defeat our measurement scheme.
- Linear scheme is slightly better, a bit less than half of what we expect to be the true miss penalty. Since our stride is 4x the cache line size, N+1 prefetching can't get us all the way there.
  - Possible that the CPU performs $>N+1$ prefetching? Or perhaps a more clever scheme? The Data-dependent prefetcher coming into play?

## Ex. 3

Setting `extrabit` to zero produces the following result:

```
stride[16] naive 5 cy/ld, linear 14 cy/ld, scrambled 34 cy/ld
stride[32] naive 10 cy/ld, linear 10 cy/ld, scrambled 51 cy/ld
stride[64] naive 11 cy/ld, linear 12 cy/ld, scrambled 66 cy/ld
stride[128] naive 7 cy/ld, linear 24 cy/ld, scrambled 65 cy/ld
stride[256] naive 7 cy/ld, linear 45 cy/ld, scrambled 75 cy/ld
stride[512] naive 9 cy/ld, linear 69 cy/ld, scrambled 86 cy/ld
stride[1024] naive 9 cy/ld, linear 50 cy/ld, scrambled 138 cy/ld
stride[2048] naive 14 cy/ld, linear 93 cy/ld, scrambled 202 cy/ld
stride[4096] naive 16 cy/ld, linear 109 cy/ld, scrambled 105 cy/ld
```

Here it looks like we don't reach the true miss penalty until 1024B stride (8x the cache line size. We know that clearing the extrabit defeats the different-row address pattern. So our results suggest that, with a 1024B stride, successive accesses are in different DRAM rows.

I suspect that QEMU is significantly distorting my measurements here. Running on "bare" metal, defeating diff-row accesses produces this with reasonable consistency:

```
stride[16] naive 11 cy/ld, linear 13 cy/ld, scrambled 23 cy/ld
stride[32] naive 5 cy/ld, linear 10 cy/ld, scrambled 45 cy/ld
stride[64] naive 5 cy/ld, linear 11 cy/ld, scrambled 66 cy/ld
stride[128] naive 5 cy/ld, linear 22 cy/ld, scrambled 78 cy/ld
stride[256] naive 6 cy/ld, linear 35 cy/ld, scrambled 93 cy/ld
stride[512] naive 7 cy/ld, linear 67 cy/ld, scrambled 90 cy/ld
stride[1024] naive 3 cy/ld, linear 21 cy/ld, scrambled 97 cy/ld
stride[2048] naive 4 cy/ld, linear 24 cy/ld, scrambled 96 cy/ld
stride[4096] naive 7 cy/ld, linear 24 cy/ld, scrambled 97 cy/ld
```

Though I note that the results _with_ the extrabit look pretty similar between QEMU and bare Mac OS.

## Ex. 4

Determining cache sizes. Adjusted the linesize to 128B to match M2 mac expectation, also adjusted max N to 2^18 as 2^19 would overflow the allocated 32MB array.

Results:

```
lgcount[4] load N cache lines, giving cy/ld. Repeat.  9 0 0 9
lgcount[5] load N cache lines, giving cy/ld. Repeat.  0 4 4 0
lgcount[6] load N cache lines, giving cy/ld. Repeat.  2 2 2 2
lgcount[7] load N cache lines, giving cy/ld. Repeat.  46 2 2 2
lgcount[8] load N cache lines, giving cy/ld. Repeat.  63 2 3 2
lgcount[9] load N cache lines, giving cy/ld. Repeat.  56 3 3 3
lgcount[10] load N cache lines, giving cy/ld. Repeat.  51 8 8 7
lgcount[11] load N cache lines, giving cy/ld. Repeat.  50 21 21 21
lgcount[12] load N cache lines, giving cy/ld. Repeat.  50 23 22 21
lgcount[13] load N cache lines, giving cy/ld. Repeat.  50 24 22 21
lgcount[14] load N cache lines, giving cy/ld. Repeat.  59 26 26 22
lgcount[15] load N cache lines, giving cy/ld. Repeat.  45 25 23 22
lgcount[16] load N cache lines, giving cy/ld. Repeat.  54 41 32 26
lgcount[17] load N cache lines, giving cy/ld. Repeat.  45 31 27 24
lgcount[18] load N cache lines, giving cy/ld. Repeat.  57 40 40 31
```

2^9 is the last N with uniformly fast re-reads. That would give an L1 cache size of 2^9 * 128 = 512 * 128 = 65KB, which is the cache size of the efficiency cores on the M2. Note that 2^10 produces still fairly fast but slightly slower re-reads. The M2's performance cores have double the L1 cache of the efficiency cores, so we could be hitting these the whole time. The big jump to ~20 cycle re-reads is most obviously the transition to L2.

Perf cores: 16MiB L2 / 128 = 131072 = 2^17
Efficiency cores: 4MiB L2 / 128 = 32768 = 2^15

Same kind of deal, where we see one profile up to the efficiency core L2 size, then another beyond that. Seems almost as though there's some sharing, but that doesn't make much sense. Maybe that the additional perf core cache is a bit slower? Like both have v fast 4MiB, then the additional 12 are a bit slower?