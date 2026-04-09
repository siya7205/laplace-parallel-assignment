#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>

using namespace std;

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N = 200;
    int max_iter = 10000;
    double tol = 1e-6;

    if (argc > 1) N = stoi(argv[1]);
    if (argc > 2) max_iter = stoi(argv[2]);
    if (argc > 3) tol = stod(argv[3]);

    int base_rows = N / size;
    int extra = N % size;
    int local_rows = base_rows + (rank < extra ? 1 : 0);

    int start_row = rank * base_rows + min(rank, extra);

    vector<vector<double>> u(local_rows + 2, vector<double>(N, 0.0));
    vector<vector<double>> u_new(local_rows + 2, vector<double>(N, 0.0));

    // Top boundary condition = 100
    for (int i = 1; i <= local_rows; ++i) {
        int global_i = start_row + (i - 1);
        if (global_i == 0) {
            for (int j = 0; j < N; ++j) {
                u[i][j] = 100.0;
                u_new[i][j] = 100.0;
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double total_start = MPI_Wtime();

    int iter = 0;
    double global_diff = 0.0;
    double comm_time = 0.0;
    double update_time = 0.0;
    double copy_time = 0.0;
    bool converged = false;

    for (iter = 0; iter < max_iter; ++iter) {
        double comm_start = MPI_Wtime();

        if (rank > 0) {
            MPI_Sendrecv(
                u[1].data(), N, MPI_DOUBLE, rank - 1, 0,
                u[0].data(), N, MPI_DOUBLE, rank - 1, 1,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE
            );
        }

        if (rank < size - 1) {
            MPI_Sendrecv(
                u[local_rows].data(), N, MPI_DOUBLE, rank + 1, 1,
                u[local_rows + 1].data(), N, MPI_DOUBLE, rank + 1, 0,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE
            );
        }

        double comm_end = MPI_Wtime();
        comm_time += (comm_end - comm_start);

        double local_diff = 0.0;

        double update_start = MPI_Wtime();
        for (int i = 1; i <= local_rows; ++i) {
            int global_i = start_row + (i - 1);
            if (global_i == 0 || global_i == N - 1) continue;

            for (int j = 1; j < N - 1; ++j) {
                u_new[i][j] = 0.25 * (
                    u[i - 1][j] +
                    u[i + 1][j] +
                    u[i][j - 1] +
                    u[i][j + 1]
                );

                double diff = fabs(u_new[i][j] - u[i][j]);
                if (diff > local_diff) {
                    local_diff = diff;
                }
            }
        }
        double update_end = MPI_Wtime();
        update_time += (update_end - update_start);

        double copy_start = MPI_Wtime();
        for (int i = 1; i <= local_rows; ++i) {
            int global_i = start_row + (i - 1);
            if (global_i == 0 || global_i == N - 1) continue;

            for (int j = 1; j < N - 1; ++j) {
                u[i][j] = u_new[i][j];
            }
        }
        double copy_end = MPI_Wtime();
        copy_time += (copy_end - copy_start);

        MPI_Allreduce(&local_diff, &global_diff, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        if (global_diff < tol) {
            converged = true;
            break;
        }
    }

    double total_end = MPI_Wtime();
    double total_time = total_end - total_start;
    int iterations_reported = converged ? iter + 1 : iter;

    vector<int> recvcounts(size), displs(size);
    for (int r = 0; r < size; ++r) {
        recvcounts[r] = (base_rows + (r < extra ? 1 : 0)) * N;
        displs[r] = (r == 0) ? 0 : displs[r - 1] + recvcounts[r - 1];
    }

    vector<double> sendbuf(local_rows * N);
    for (int i = 0; i < local_rows; ++i) {
        for (int j = 0; j < N; ++j) {
            sendbuf[i * N + j] = u[i + 1][j];
        }
    }

    vector<double> fullgrid;
    if (rank == 0) {
        fullgrid.resize(N * N);
    }

    MPI_Gatherv(
        sendbuf.data(), local_rows * N, MPI_DOUBLE,
        fullgrid.data(), recvcounts.data(), displs.data(), MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );

    if (rank == 0) {
        cout << fixed << setprecision(6);
        cout << "Grid size: " << N << "x" << N << "\n";
        cout << "MPI processes: " << size << "\n";
        cout << "Iterations: " << iterations_reported << "\n";
        cout << "Final max diff: " << global_diff << "\n";
        cout << "Center value: " << fullgrid[(N / 2) * N + (N / 2)] << "\n";
        cout << "Converged: " << (converged ? "true" : "false") << "\n";
        cout << "Total time: " << total_time << " seconds\n";
        cout << "Communication time: " << comm_time << " seconds\n";
        cout << "Update loop time: " << update_time << " seconds\n";
        cout << "Copy loop time: " << copy_time << " seconds\n";

        ofstream fout("results/solution_csv/cpp_mpi_solution.csv");
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                fout << fullgrid[i * N + j];
                if (j != N - 1) fout << ",";
            }
            fout << "\n";
        }
        fout.close();
    }

    MPI_Finalize();
    return 0;
}
