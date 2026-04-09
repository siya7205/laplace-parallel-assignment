#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <chrono>
#include <iomanip>

using namespace std;
using clock_type = chrono::high_resolution_clock;

int main(int argc, char* argv[]) {
    int N = 200;
    int max_iter = 10000;
    double tol = 1e-6;

    if (argc > 1) N = stoi(argv[1]);
    if (argc > 2) max_iter = stoi(argv[2]);
    if (argc > 3) tol = stod(argv[3]);

    vector<vector<double>> u(N, vector<double>(N, 0.0));
    vector<vector<double>> u_new(N, vector<double>(N, 0.0));

    for (int j = 0; j < N; ++j) {
        u[0][j] = 100.0;
        u_new[0][j] = 100.0;
    }

    int iter = 0;
    double max_diff = 0.0;
    bool converged = false;

    double update_time = 0.0;
    double copy_time = 0.0;

    auto total_start = clock_type::now();

    for (iter = 0; iter < max_iter; ++iter) {
        max_diff = 0.0;

        auto update_start = clock_type::now();
        for (int i = 1; i < N - 1; ++i) {
            for (int j = 1; j < N - 1; ++j) {
                u_new[i][j] = 0.25 * (
                    u[i - 1][j] +
                    u[i + 1][j] +
                    u[i][j - 1] +
                    u[i][j + 1]
                );

                double diff = fabs(u_new[i][j] - u[i][j]);
                if (diff > max_diff) {
                    max_diff = diff;
                }
            }
        }
        auto update_end = clock_type::now();
        update_time += chrono::duration<double>(update_end - update_start).count();

        auto copy_start = clock_type::now();
        for (int i = 1; i < N - 1; ++i) {
            for (int j = 1; j < N - 1; ++j) {
                u[i][j] = u_new[i][j];
            }
        }
        auto copy_end = clock_type::now();
        copy_time += chrono::duration<double>(copy_end - copy_start).count();

        if (max_diff < tol) {
            converged = true;
            break;
        }
    }

    auto total_end = clock_type::now();
    double total_time = chrono::duration<double>(total_end - total_start).count();
    int iterations_reported = converged ? iter + 1 : iter;

    cout << fixed << setprecision(6);
    cout << "Grid size: " << N << "x" << N << "\n";
    cout << "Iterations: " << iterations_reported << "\n";
    cout << "Final max diff: " << max_diff << "\n";
    cout << "Center value: " << u[N / 2][N / 2] << "\n";
    cout << "Converged: " << (converged ? "true" : "false") << "\n";
    cout << "Total time: " << total_time << " seconds\n";
    cout << "Update loop time: " << update_time << " seconds\n";
    cout << "Copy loop time: " << copy_time << " seconds\n";

    ofstream fout("results/solution_csv/cpp_serial_solution.csv");
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            fout << u[i][j];
            if (j != N - 1) fout << ",";
        }
        fout << "\n";
    }
    fout.close();

    return 0;
}
