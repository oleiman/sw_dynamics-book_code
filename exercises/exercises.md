% Understanding Software Dynamics
% Exercises and Notes

# Chapter 2

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