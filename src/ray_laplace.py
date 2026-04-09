import ray
import numpy as np
import sys
import time

@ray.remote
class BlockWorker:
    def __init__(self, start_row, local_rows, N):
        self.start_row = start_row
        self.local_rows = local_rows
        self.N = N
        self.u = np.zeros((local_rows + 2, N), dtype=np.float64)
        self.u_new = np.zeros_like(self.u)

        # Top boundary = 100
        for i in range(1, local_rows + 1):
            global_i = start_row + (i - 1)
            if global_i == 0:
                self.u[i, :] = 100.0
                self.u_new[i, :] = 100.0

    def get_top_row(self):
        return self.u[1, :].copy()

    def get_bottom_row(self):
        return self.u[self.local_rows, :].copy()

    def set_top_halo(self, row):
        self.u[0, :] = row

    def set_bottom_halo(self, row):
        self.u[self.local_rows + 1, :] = row

    def iterate(self):
        local_diff = 0.0

        for i in range(1, self.local_rows + 1):
            global_i = self.start_row + (i - 1)
            if global_i == 0 or global_i == self.N - 1:
                continue

            self.u_new[i, 1:-1] = 0.25 * (
                self.u[i - 1, 1:-1] +
                self.u[i + 1, 1:-1] +
                self.u[i, :-2] +
                self.u[i, 2:]
            )

            diff = np.max(np.abs(self.u_new[i, 1:-1] - self.u[i, 1:-1]))
            local_diff = max(local_diff, diff)

        for i in range(1, self.local_rows + 1):
            global_i = self.start_row + (i - 1)
            if global_i == 0 or global_i == self.N - 1:
                continue
            self.u[i, 1:-1] = self.u_new[i, 1:-1]

        return local_diff

    def get_data(self):
        return self.u[1:self.local_rows + 1, :]

def main():
    N = int(sys.argv[1]) if len(sys.argv) > 1 else 200
    max_iter = int(sys.argv[2]) if len(sys.argv) > 2 else 10000
    tol = float(sys.argv[3]) if len(sys.argv) > 3 else 1e-6
    num_blocks = int(sys.argv[4]) if len(sys.argv) > 4 else 4

    ray.init(ignore_reinit_error=True)

    base_rows = N // num_blocks
    extra = N % num_blocks

    workers = []
    start_row = 0
    for b in range(num_blocks):
        local_rows = base_rows + (1 if b < extra else 0)
        workers.append(BlockWorker.remote(start_row, local_rows, N))
        start_row += local_rows

    halo_time = 0.0
    iterate_time = 0.0
    converged = False
    final_diff = None

    total_start = time.perf_counter()

    for it in range(max_iter):
        t0 = time.perf_counter()

        top_rows = ray.get([w.get_top_row.remote() for w in workers])
        bottom_rows = ray.get([w.get_bottom_row.remote() for w in workers])

        halo_updates = []
        for b, w in enumerate(workers):
            if b > 0:
                halo_updates.append(w.set_top_halo.remote(bottom_rows[b - 1]))
            if b < num_blocks - 1:
                halo_updates.append(w.set_bottom_halo.remote(top_rows[b + 1]))
        ray.get(halo_updates)

        t1 = time.perf_counter()
        halo_time += (t1 - t0)

        t2 = time.perf_counter()
        local_diffs = ray.get([w.iterate.remote() for w in workers])
        global_diff = max(local_diffs)
        final_diff = global_diff
        t3 = time.perf_counter()
        iterate_time += (t3 - t2)

        if global_diff < tol:
            converged = True
            break

    total_end = time.perf_counter()
    total_time = total_end - total_start
    iterations_reported = (it + 1) if converged else it

    blocks = ray.get([w.get_data.remote() for w in workers])
    fullgrid = np.vstack(blocks)

    print(f"Grid size: {N}x{N}")
    print(f"Ray blocks: {num_blocks}")
    print(f"Iterations: {iterations_reported}")
    print(f"Final max diff: {final_diff:.6e}")
    print(f"Center value: {fullgrid[N // 2, N // 2]:.6f}")
    print(f"Converged: {converged}")
    print(f"Total time: {total_time:.6f} seconds")
    print(f"Halo exchange time: {halo_time:.6f} seconds")
    print(f"Iteration compute time: {iterate_time:.6f} seconds")

    np.savetxt("results/solution_csv/python_ray_solution.csv", fullgrid, delimiter=",", fmt="%.6f")

    ray.shutdown()

if __name__ == "__main__":
    main()
