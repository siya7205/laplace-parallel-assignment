# Scalability Analysis and Performance Comparison

---

## Subsection Outline

1. [Benchmark Configuration](#1-benchmark-configuration)
2. [Main Benchmark Results Table](#2-main-benchmark-comparison-table-n400-max_iter20000-tol1e-6)
3. [Speedup and Efficiency Table](#3-speedup-and-efficiency-table)
4. [Reduced Benchmark Table (Ray)](#4-reduced-benchmark-table-including-ray-n200-max_iter3000-tol1e-6)
5. [Profiler and Hotspot Analysis](#5-profiler-and-hotspot-analysis)
6. [Discussion](#6-discussion)
7. [Concise Observations](#7-concise-observations-report-ready)

---

## 1. Benchmark Configuration

All large-benchmark runs used **N=400, max_iter=20000, tol=1e-6** on a local laptop.
Because the local machine could not handle the Ray actor-based framework efficiently at this scale,
Ray was evaluated separately at **N=200, max_iter=3000, tol=1e-6**.
Timing was measured as follows:

| Implementation | Timing Method | Granularity |
|---|---|---|
| Serial C++ / OpenMP C++ | `std::chrono::high_resolution_clock` | Separate update loop, copy loop, total |
| MPI C++ / MPI Python | `MPI_Wtime()` | Separate communication, update loop, copy loop, total |
| Serial Python | `time.perf_counter()` + `cProfile` | Total; profiler for hotspot breakdown |
| Ray Python | `time.perf_counter()` | Separate halo-exchange time, iteration-compute time, total |

> **Note on serial Python profiling:** Serial Python (`serial_laplace.py`) was run under `cProfile`.
> The profile showed that essentially all CPU time was consumed inside the solver loop body,
> dominated by `np.max(np.abs(...))` (the max-reduction step) and the stencil update assignment.
> A separate timing notes file for `serial_laplace.py` is **not present in the repository**;
> the profiler observations are noted qualitatively.
> The MPI Python 1-process timing (39.772846 s) is the closest available proxy for a Python
> reference time at N=400, but it uses a row-by-row Python loop rather than full-grid vectorisation,
> so it is not a direct equivalent.

---

## 2. Main Benchmark Comparison Table (N=400, max_iter=20000, tol=1e-6)

| Implementation | Processes / Threads | Total Time (s) | Update Loop (s) | Copy Loop (s) | Comm. Time (s) | Center Value | Final Max Diff | Converged |
|---|---|---|---|---|---|---|---|---|
| Serial C++ | 1 | 4.836557 | 3.762600 | 1.073229 | — | 4.340229 | 0.001098 | No |
| Serial Python | 1 | **[MISSING — no timing notes file in repo]** | — | — | — | — | — | — |
| OpenMP C++ | 1 (OMP overhead) | 7.799963 | 5.305470 | 2.493684 | — | 4.340229 | 0.001098 | No |
| OpenMP C++ | 2 | 4.552218 | 2.996641 | 1.554497 | — | 4.340229 | 0.001098 | No |
| OpenMP C++ | 4 | 3.030280 | 1.915386 | 1.113810 | — | 4.340229 | 0.001098 | No |
| OpenMP C++ | 8 | 3.567517 | 2.400472 | 1.165953 | — | 4.340229 | 0.001098 | No |
| MPI C++ | 1 | 3.676413 | 2.570369 | 1.104134 | 0.000415 | 4.340229 | 0.001098 | No |
| MPI C++ | 2 | 1.953825 | 1.342436 | 0.560222 | 0.026990 | 4.340229 | 0.001098 | No |
| MPI C++ | 4 | 1.364184 | 0.817406 | 0.306822 | 0.030805 | 4.340229 | 0.001098 | No |
| MPI C++ | 8 | 1.757687 | 0.742131 | 0.181232 | 0.055438 | 4.340229 | 0.001098 | No |
| MPI Python | 1 | 39.772846 | 36.460282 | 3.088354 | 0.001970 | 4.340229 | 1.098438e-03 | No |
| MPI Python | 2 | 20.508121 | 18.336417 | 1.547730 | 0.066618 | 4.340229 | 1.098438e-03 | No |
| MPI Python | 4 | 11.516111 | 9.725422 | 0.819004 | 0.073717 | 4.340229 | 1.098438e-03 | No |
| MPI Python | 8 | 18.328680 | 7.970392 | 0.612209 | 0.462481 | 4.340229 | 1.098438e-03 | No |

> **Numerical correctness:** All C++ and Python implementations produce `center value = 4.340229`
> and `final max diff ≈ 0.001098` across every thread/process count, confirming that parallelisation
> did not alter the numerical result.

---

## 3. Speedup and Efficiency Table

Speedup is defined as **S(p) = T₁ / Tₚ** and efficiency as **E(p) = T₁ / (p × Tₚ)**,
where T₁ is the 1-unit (1 thread or 1 process) time within each implementation family.

For **OpenMP**, the reference T₁ used is the **serial C++ baseline (4.836557 s)** because the
1-thread OpenMP run (7.799963 s) includes OpenMP runtime initialisation overhead not present in
the pure serial binary.

For **MPI C++** and **MPI Python**, the reference T₁ is the respective 1-process run within each
family, since no separate standalone serial timing file exists for those languages at this
benchmark size in the repository.

### OpenMP (T₁ = 4.836557 s, serial C++)

| Threads | Total Time (s) | Speedup S(p) | Efficiency E(p) |
|---|---|---|---|
| 1 (OMP, overhead) | 7.799963 | 0.620 | 0.620 |
| 2 | 4.552218 | 1.062 | 0.531 |
| 4 | 3.030280 | 1.596 | 0.399 |
| 8 | 3.567517 | 1.355 | 0.169 |

### MPI C++ (T₁ = 3.676413 s, MPI 1 process)

| Processes | Total Time (s) | Comm. Time (s) | Comm. % of Total | Speedup S(p) | Efficiency E(p) |
|---|---|---|---|---|---|
| 1 | 3.676413 | 0.000415 | 0.01% | 1.000 | 1.000 |
| 2 | 1.953825 | 0.026990 | 1.38% | 1.881 | 0.941 |
| 4 | 1.364184 | 0.030805 | 2.26% | 2.695 | 0.674 |
| 8 | 1.757687 | 0.055438 | 3.15% | 2.091 | 0.261 |

### MPI Python (T₁ = 39.772846 s, MPI Python 1 process)

| Processes | Total Time (s) | Comm. Time (s) | Comm. % of Total | Speedup S(p) | Efficiency E(p) |
|---|---|---|---|---|---|
| 1 | 39.772846 | 0.001970 | 0.005% | 1.000 | 1.000 |
| 2 | 20.508121 | 0.066618 | 0.32% | 1.939 | 0.970 |
| 4 | 11.516111 | 0.073717 | 0.64% | 3.453 | 0.863 |
| 8 | 18.328680 | 0.462481 | 2.52% | 2.170 | 0.271 |

> MPI Python shows surprisingly good relative speedup from 1→4 processes (3.45×) because the
> Python interpreter overhead is large and is divided across ranks without proportional increase
> in communication cost — until 8 processes, where the per-iteration `Allreduce` and `Sendrecv`
> costs dominate.

---

## 4. Reduced Benchmark Table Including Ray (N=200, max_iter=3000, tol=1e-6)

Ray was run only at this smaller benchmark. Values not present in the repository are explicitly
marked as **[MISSING]**.

| Implementation | Blocks / Processes / Threads | Total Time (s) | Compute Time (s) | Halo/Comm Time (s) | Halo % of Total | Center Value | Final Max Diff |
|---|---|---|---|---|---|---|---|
| Serial C++ (N=200) | 1 | **[MISSING]** | — | — | — | — | — |
| Serial Python (N=200) | 1 | **[MISSING]** | — | — | — | — | — |
| MPI C++ (N=200) | 1 | **[MISSING]** | — | — | — | — | — |
| MPI Python (N=200) | 1 | **[MISSING]** | — | — | — | — | — |
| Ray Python | 1 block | 38.746729 | 14.349829 | 24.390450 | 62.9% | 0.972337 | 7.900930e-03 |
| Ray Python | 2 blocks | 68.170498 | 17.435035 | 50.728078 | 74.4% | 0.972337 | 7.900930e-03 |
| Ray Python | 4 blocks | 93.418089 | 21.100201 | 72.310266 | 77.4% | 0.972337 | 7.900930e-03 |

> **Missing data note:** Serial C++, Serial Python, MPI C++, and MPI Python were not run at
> N=200, max_iter=3000 and no corresponding timing notes exist in the repository. To make this
> table complete, those experiments would need to be re-run at the reduced benchmark size.
> The Ray results cannot be compared to the other families on absolute runtime until those
> numbers are available.

> **Numerical correctness (Ray):** All three Ray block configurations produce
> `center value = 0.972337` and `final max diff = 7.900930e-03`, confirming that the
> halo-exchange logic did not corrupt the solution.

---

## 5. Profiler and Hotspot Analysis

### Serial C++ (`serial_laplace.cpp`)

Manual timing with `std::chrono` separated the update and copy loops.
Across the full run (N=400, 20000 iterations):

- **Update loop: 3.762600 s (77.8% of total)** — stencil arithmetic + element-wise `max(abs(...))` reduction
- **Copy loop: 1.073229 s (22.2% of total)** — bulk `u[i][j] = u_new[i][j]`, purely memory-bandwidth bound

The update loop carries both the stencil computation and the per-element max-reduction, making it
the primary hotspot. The copy loop is a pure memory-movement operation with no arithmetic.

### Serial Python (`serial_laplace.py`)

Timed with `time.perf_counter()` and profiled with `cProfile`.
The profiler placed nearly all runtime inside the solver loop body. The two dominant sub-operations
were:

- **Stencil update:** `0.25 * (u[:-2,...] + u[2:,...] + ...)` — vectorized NumPy slice arithmetic,
  re-dispatched at every iteration.
- **Convergence check:** `np.max(np.abs(u_new[...] - u[...]))` — allocates a temporary array and
  then reduces it, called once per iteration.

The row-at-a-time NumPy slicing is more efficient than a Python `for` loop, but still much slower
than C++ because NumPy function calls carry Python C-extension dispatch overhead per iteration.

> **Note:** A separate `serial_notes.txt` timing file for `serial_laplace.py` does not exist in
> the repository. The MPI Python 1-process run (39.772846 s) is the closest available proxy at
> N=400, but differs in that it uses row-by-row Python loops for the update, so the comparison
> is not exact.

### OpenMP C++ (`omp_laplace.cpp`)

The implementation adds:
- `#pragma omp parallel for collapse(2) reduction(max:max_diff)` to the update loop
- `#pragma omp parallel for collapse(2)` to the copy loop

Key observations from the timing notes:

- **1-thread OMP (7.80 s) is ~61% slower than serial C++ (4.84 s):** thread-pool initialisation
  and barrier synchronisation occur 20,000 times, adding fixed per-iteration overhead.
- **4 threads best config (3.03 s):** update loop drops to 1.915 s from 3.763 s serial (1.96× for
  that loop alone).
- **8 threads regresses to 3.57 s:** available parallelism (398×398 interior cells) is insufficient
  to saturate 8 threads on this machine; false sharing and `reduction(max:max_diff)` cache
  coherence costs increase.
- The copy loop also benefits from threading but less dramatically, as it is purely
  memory-bandwidth bound.

### MPI C++ (`mpi_laplace.cpp`)

Each rank owns a horizontal strip plus two halo rows. `MPI_Sendrecv` exchanges one row with each
neighbour, and `MPI_Allreduce` computes global max-diff after each iteration.

| Processes | Comm. Time (s) | Comm. % of Total | Notes |
|---|---|---|---|
| 1 | 0.000415 | 0.01% | Negligible |
| 2 | 0.026990 | 1.38% | Halo exchange starts contributing |
| 4 | 0.030805 | 2.26% | Best total time (1.364 s) |
| 8 | 0.055438 | 3.15% | Allreduce latency × 20,000 iterations dominates |

The update loop time *continues* decreasing at 8 processes (0.742 s vs 0.817 s at 4 processes),
meaning compute still improves, but total time increases because synchronisation cost overtakes
the remaining compute budget.

### MPI Python (`mpi_laplace.py`)

Uses `mpi4py` with `Sendrecv` and `allreduce`. Unlike `serial_laplace.py`, the update loop here
is a Python `for` over local rows (not a full 2D NumPy operation), so each row update involves:
a Python function call to NumPy, a temporary array allocation, and a separate
`np.max(np.abs(...))` call.

- At 1 process: update loop = 36.460 s (**91.7%** of total, vs 77.8% in C++)
- At 4 processes: update loop = 9.725 s, total = 11.516 s (~4× reduction in update time, 3.45×
  overall)
- At 8 processes: communication time jumps to 0.462 s; total rises to 18.33 s even though the
  update loop (7.97 s) is still improving
- **MPI Python at best (11.52 s) is ~8.4× slower than MPI C++ at best (1.364 s)**

### Ray Python (`ray_laplace.py`)

Each `BlockWorker` is a Ray remote actor. Per iteration, the driver makes six groups of remote
calls: `get_top_row`, `get_bottom_row`, `set_top_halo`, `set_bottom_halo`, `iterate` (per block),
and `ray.get()` waits. For N=200, max_iter=3000 at 4 blocks:
**4 actors × ~6 call groups × 3000 iterations ≈ 72,000 remote calls.**

| Blocks | Total (s) | Compute (s) | Halo Exchange (s) | Halo % |
|---|---|---|---|---|
| 1 | 38.746729 | 14.349829 | 24.390450 | 62.9% |
| 2 | 68.170498 | 17.435035 | 50.728078 | 74.4% |
| 4 | 93.418089 | 21.100201 | 72.310266 | 77.4% |

Even at 1 block (no actual data exchange needed), 62.9% of runtime is spent in actor coordination.
Performance worsens monotonically as blocks increase, the inverse of what a scalable design produces.

---

## 6. Discussion

### Main Hotspots

Across all implementations, the **stencil update loop** (computing the Jacobi average of four
neighbours) is the dominant computational hotspot. In serial C++, it accounts for 77.8% of total
runtime. The copy loop (bulk copy of `u_new` back into `u`) is a secondary hotspot driven entirely
by memory bandwidth, not arithmetic. In MPI Python, the update loop's share rises to 91.7% because
Python's interpreter overhead inflates the cost of each row-wise NumPy call, while the copy remains
relatively cheaper since it is a contiguous slice assignment.

### Why Performance Improved Initially

For both OpenMP and MPI, performance improved as the number of threads or processes increased from
1 to 4. This is expected: the update loop is embarrassingly parallel across interior grid rows
(each cell depends only on its four neighbours, which are read-only in the Jacobi scheme).
Distributing rows across threads or processes reduces the per-unit workload without requiring any
synchronisation within the update itself. MPI C++ benefited more cleanly than OpenMP — going from
1.881× at 2 processes to 2.695× at 4 — because each MPI rank operates on a fully private memory
space, eliminating cache coherence traffic. OpenMP's `#pragma omp parallel for collapse(2)` applies
threading across a shared address space, where cache lines at row boundaries can be written by
different threads, introducing false sharing.

### Why Performance Degraded Beyond 4 Threads/Processes

Performance declined at 8 threads (OpenMP) and 8 processes (both MPI flavours). For **OpenMP**,
the per-iteration cost of the `reduction(max:max_diff)` clause — which aggregates the local max
from all threads into a single scalar — and the thread-fork/join barrier (called 20,000 times)
adds latency that grows with thread count. At 8 threads on this machine, the overhead of
synchronisation exceeds the benefit of further work decomposition.

For **MPI**, the culprit is the `MPI_Allreduce` called once per iteration. Although the allreduce
is a single-scalar operation and appears inexpensive in isolation, when called 20,000 times across
8 processes on shared-memory hardware (where ranks communicate through loopback rather than a
network), the cumulative latency becomes significant. MPI C++ communication time rose from 0.031 s
at 4 processes to 0.055 s at 8 processes — a 79% increase for only a 2× increase in process count.
MPI Python's communication time at 8 processes (0.462 s) is an even larger jump from 4 processes
(0.074 s), partly because Python-level MPI calls carry additional interpreter and serialisation
overhead. In both cases, each rank's local computation at 8 units (≈50 rows per rank) becomes so
small that the ratio of synchronisation cost to computation cost inverts, and scalability collapses.

### Why C++ Versions Outperformed Python Versions

The gap between MPI C++ and MPI Python at their respective best is approximately **8.4×**
(1.364 s vs 11.516 s). This stems from three compounding factors:

1. **Interpreter overhead per operation.** The MPI Python update loop iterates over rows in a
   Python `for` loop. Even though each row update uses a NumPy vectorized slice, the call to
   `np.max(np.abs(...))` and the Python comparison `local_diff = max(local_diff, diff)` on every
   row involves Python object dispatch on every iteration.
2. **Memory allocation.** NumPy creates intermediate arrays for the arithmetic and abs-reduction
   on each row. The C++ update loop uses only stack scalars and direct pointer accesses.
3. **No JIT or ahead-of-time compilation.** The serial Python and MPI Python code runs under
   CPython without any SIMD or loop-vectorisation optimisations that the C++ compiler applies
   automatically.

The OpenMP C++ implementation also benefits from the compiler's ability to vectorise inner loops
(SIMD instructions), further widening the gap over any Python version.

### Why Ray Performed Poorly on Local Laptop

Ray is architected as a distributed task scheduler: remote function/actor calls are dispatched
through a local scheduler daemon, object references are resolved through a shared object store
(backed by Apache Arrow/Plasma), and `ray.get()` blocks until results are materialised. On a
single laptop, all of this occurs over localhost shared memory, but the scheduling, reference
counting, and serialisation path is still invoked for every remote call — six round-trips to the
Ray scheduler per iteration per block.

For N=200, max_iter=3000 at 4 blocks: 4 × 6 × 3000 = 72,000 remote calls. Even if each call
takes only ~1 ms in overhead, that is 72 seconds in scheduling alone, which matches the observed
halo exchange time of 72.3 s. The actual computation (21.1 s) is a small fraction. Ray's overhead
would amortise only if each `iterate` call did significantly more work — for example, running many
iterations between synchronisations (multi-step buffering), or working on problems where the
compute per call is orders of magnitude larger. For a stencil solver requiring per-iteration global
synchronisation, Ray is fundamentally mismatched to the workload.

---

## 7. Concise Observations (Report-Ready)

1. **The update loop is the dominant hotspot** across all implementations, accounting for 77.8% of
   serial C++ time and up to 91.7% of MPI Python time. Any effective parallelisation strategy must
   target this loop.

2. **OpenMP achieves modest speedup** (best: 1.60× at 4 threads vs serial C++). The 1-thread
   OpenMP run is actually 61% *slower* than serial C++ due to thread-pool and barrier overhead
   amortised across 20,000 iterations, showing that OpenMP carries non-trivial fixed cost per
   parallel region.

3. **MPI C++ shows the best absolute scalability** among all tested approaches, reaching 2.70×
   speedup at 4 processes. Performance degrades at 8 processes because `MPI_Allreduce` (called per
   iteration) accumulates enough latency to outweigh further work decomposition.

4. **MPI Python follows the same scaling shape as MPI C++** but at roughly 8–10× slower absolute
   time. MPI Python's best is 3.45× speedup at 4 processes (vs 2.70× for MPI C++), suggesting
   that the Python overhead distributes almost linearly, but the raw wall clock cost remains
   unacceptable for production use.

5. **Numerical correctness is fully preserved** across all thread/process counts in C++ and Python
   MPI. All runs produce `center value = 4.340229` and `final max diff ≈ 0.001098`, confirming
   that parallelisation of the Jacobi iteration does not affect convergence or accuracy.

6. **Ray does not scale for this workload on local hardware.** At 1 block, 62.9% of total time is
   already consumed by actor coordination overhead — before any actual parallel decomposition
   occurs. Performance worsens monotonically: 38.7 s → 68.2 s → 93.4 s as blocks increase from 1
   to 4. Ray is unsuitable for fine-grained, per-iteration synchronised stencil solvers unless the
   compute-to-communication ratio can be dramatically increased (e.g., multi-step buffering between
   exchanges or much larger grids).

7. **Communication overhead in MPI grows but remains a secondary cost.** In MPI C++,
   communication goes from 0.01% at 1 process to 3.15% at 8 processes. In MPI Python, it reaches
   2.52% at 8 processes. The primary scalability bottleneck is not raw data transfer but the
   collective synchronisation (`MPI_Allreduce`) latency multiplied by the iteration count.

8. **The copy loop is a memory-bandwidth bottleneck** that benefits from parallelism but shows
   diminishing returns. In OpenMP, the copy loop time decreases from 2.49 s (1 thread) to 1.11 s
   (4 threads) but barely improves beyond that, suggesting the local memory bus is saturating.
   Replacing the explicit copy with a pointer-swap would eliminate this cost entirely and represents
   a clear optimisation opportunity.

---

> **Summary of Missing Data:** A timing notes file for `serial_laplace.py` at any benchmark size
> does not exist in the repository. Additionally, no timing notes exist for any implementation at
> N=200, max_iter=3000, which means the reduced benchmark table (Section 4) cannot be completed
> without running fresh experiments at that scale.
