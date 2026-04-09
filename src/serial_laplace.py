import sys
import time
import numpy as np

def solve_laplace(N=200, max_iter=10000, tol=1e-6):
    u = np.zeros((N, N), dtype=np.float64)
    u_new = np.zeros_like(u)

    # Boundary conditions
    u[0, :] = 100.0
    u_new[0, :] = 100.0

    start = time.perf_counter()
    final_diff = None
    converged = False

    for it in range(max_iter):
        u_new[1:-1, 1:-1] = 0.25 * (
            u[:-2, 1:-1] +
            u[2:, 1:-1] +
            u[1:-1, :-2] +
            u[1:-1, 2:]
        )

        diff = np.max(np.abs(u_new[1:-1, 1:-1] - u[1:-1, 1:-1]))
        u[1:-1, 1:-1] = u_new[1:-1, 1:-1]

        final_diff = diff
        if diff < tol:
            converged = True
            break

    end = time.perf_counter()

    print(f"Grid size: {N}x{N}")
    print(f"Iterations: {it + 1}")
    print(f"Final max diff: {final_diff:.6e}")
    print(f"Center value: {u[N//2, N//2]:.6f}")
    print(f"Converged: {converged}")
    print(f"Time: {end - start:.6f} seconds")

    np.savetxt("results/solution_csv/python_serial_solution.csv", u, delimiter=",", fmt="%.6f")

if __name__ == "__main__":
    N = int(sys.argv[1]) if len(sys.argv) > 1 else 200
    max_iter = int(sys.argv[2]) if len(sys.argv) > 2 else 10000
    tol = float(sys.argv[3]) if len(sys.argv) > 3 else 1e-6
    solve_laplace(N, max_iter, tol)
