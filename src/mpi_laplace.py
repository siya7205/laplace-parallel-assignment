from mpi4py import MPI
import numpy as np
import sys

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

N = int(sys.argv[1]) if len(sys.argv) > 1 else 200
max_iter = int(sys.argv[2]) if len(sys.argv) > 2 else 10000
tol = float(sys.argv[3]) if len(sys.argv) > 3 else 1e-6

base_rows = N // size
extra = N % size
local_rows = base_rows + (1 if rank < extra else 0)
start_row = rank * base_rows + min(rank, extra)

u = np.zeros((local_rows + 2, N), dtype=np.float64)
u_new = np.zeros_like(u)

# Top boundary = 100
for i in range(1, local_rows + 1):
    global_i = start_row + (i - 1)
    if global_i == 0:
        u[i, :] = 100.0
        u_new[i, :] = 100.0

comm.Barrier()
total_start = MPI.Wtime()

comm_time = 0.0
update_time = 0.0
copy_time = 0.0
converged = False
final_diff = None

for it in range(max_iter):
    t0 = MPI.Wtime()

    if rank > 0:
        comm.Sendrecv(sendbuf=u[1, :], dest=rank - 1, sendtag=0,
                      recvbuf=u[0, :], source=rank - 1, recvtag=1)

    if rank < size - 1:
        comm.Sendrecv(sendbuf=u[local_rows, :], dest=rank + 1, sendtag=1,
                      recvbuf=u[local_rows + 1, :], source=rank + 1, recvtag=0)

    t1 = MPI.Wtime()
    comm_time += (t1 - t0)

    local_diff = 0.0

    t2 = MPI.Wtime()
    for i in range(1, local_rows + 1):
        global_i = start_row + (i - 1)
        if global_i == 0 or global_i == N - 1:
            continue

        u_new[i, 1:-1] = 0.25 * (
            u[i - 1, 1:-1] +
            u[i + 1, 1:-1] +
            u[i, :-2] +
            u[i, 2:]
        )

        diff = np.max(np.abs(u_new[i, 1:-1] - u[i, 1:-1]))
        local_diff = max(local_diff, diff)
    t3 = MPI.Wtime()
    update_time += (t3 - t2)

    t4 = MPI.Wtime()
    for i in range(1, local_rows + 1):
        global_i = start_row + (i - 1)
        if global_i == 0 or global_i == N - 1:
            continue
        u[i, 1:-1] = u_new[i, 1:-1]
    t5 = MPI.Wtime()
    copy_time += (t5 - t4)

    global_diff = comm.allreduce(local_diff, op=MPI.MAX)
    final_diff = global_diff

    if global_diff < tol:
        converged = True
        break

total_end = MPI.Wtime()
total_time = total_end - total_start
iterations_reported = (it + 1) if converged else it

sendbuf = u[1:local_rows + 1, :].ravel()
recvcounts = np.array(
    [(base_rows + (1 if r < extra else 0)) * N for r in range(size)],
    dtype=int
)
displs = np.zeros(size, dtype=int)
displs[1:] = np.cumsum(recvcounts[:-1])

if rank == 0:
    fullgrid = np.empty(N * N, dtype=np.float64)
else:
    fullgrid = None

comm.Gatherv(sendbuf, [fullgrid, recvcounts, displs, MPI.DOUBLE], root=0)

if rank == 0:
    fullgrid = fullgrid.reshape((N, N))
    print(f"Grid size: {N}x{N}")
    print(f"MPI processes: {size}")
    print(f"Iterations: {iterations_reported}")
    print(f"Final max diff: {final_diff:.6e}")
    print(f"Center value: {fullgrid[N // 2, N // 2]:.6f}")
    print(f"Converged: {converged}")
    print(f"Total time: {total_time:.6f} seconds")
    print(f"Communication time: {comm_time:.6f} seconds")
    print(f"Update loop time: {update_time:.6f} seconds")
    print(f"Copy loop time: {copy_time:.6f} seconds")

    np.savetxt("results/solution_csv/python_mpi_solution.csv", fullgrid, delimiter=",", fmt="%.6f")
