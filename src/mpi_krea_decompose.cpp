#include <mpi.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

struct Rect {
    int x1, y1, x2, y2;
    char letter;
};

void fill_rect(vector<string>& grid, const Rect& r, char ch) {
    for (int y = r.y1; y <= r.y2; ++y) {
        for (int x = r.x1; x <= r.x2; ++x) {
            if (y >= 0 && y < (int)grid.size() && x >= 0 && x < (int)grid[0].size()) {
                grid[y][x] = ch;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int W = 72;
    const int H = 24;

    vector<Rect> rects;

    // ---------------- K ----------------
    rects.push_back({1, 2, 2, 19, 'K'});    // vertical
    rects.push_back({3, 10, 4, 11, 'K'});   // center
    rects.push_back({3, 8, 4, 9, 'K'});
    rects.push_back({5, 6, 6, 7, 'K'});
    rects.push_back({7, 4, 8, 5, 'K'});
    rects.push_back({3, 12, 4, 13, 'K'});
    rects.push_back({5, 14, 6, 15, 'K'});
    rects.push_back({7, 16, 8, 17, 'K'});

    // ---------------- R ----------------
    rects.push_back({13, 2, 14, 19, 'R'});  // left vertical
    rects.push_back({15, 2, 20, 3, 'R'});   // top bar
    rects.push_back({19, 4, 20, 7, 'R'});   // upper right vertical
    rects.push_back({15, 8, 19, 9, 'R'});   // middle bar
    rects.push_back({16, 10, 17, 11, 'R'}); // diagonal leg
    rects.push_back({18, 12, 19, 13, 'R'});
    rects.push_back({20, 14, 21, 15, 'R'});
    rects.push_back({22, 16, 23, 19, 'R'});

    // ---------------- E ----------------
    rects.push_back({27, 2, 28, 19, 'E'});  // left vertical
    rects.push_back({29, 2, 35, 3, 'E'});   // top bar
    rects.push_back({29, 10, 34, 11, 'E'}); // middle bar
    rects.push_back({29, 18, 35, 19, 'E'}); // bottom bar

    // ---------------- A ----------------
    rects.push_back({41, 6, 42, 19, 'A'});  // left leg
    rects.push_back({49, 6, 50, 19, 'A'});  // right leg
    rects.push_back({43, 2, 48, 5, 'A'});   // top cap
    rects.push_back({43, 10, 48, 11, 'A'}); // middle bar

    int num_rects = (int)rects.size();

    vector<string> letter_grid(H, string(W, '.'));
    vector<string> owner_grid(H, string(W, '.'));

    for (int i = 0; i < num_rects; ++i) {
        int owner = i % size;
        char owner_char = '0' + (owner % 10);
        fill_rect(letter_grid, rects[i], rects[i].letter);
        fill_rect(owner_grid, rects[i], owner_char);
    }

    // Print ownership cleanly, rank by rank
    for (int r = 0; r < size; ++r) {
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == r) {
            cout << "Rank " << rank << " owns rectangles: ";
            bool first = true;
            for (int i = 0; i < num_rects; ++i) {
                if (i % size == rank) {
                    if (!first) cout << ", ";
                    cout << i << "(" << rects[i].letter << ")";
                    first = false;
                }
            }
            cout << "\n";
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        cout << "\nKREA domain decomposition using rectangles\n";
        cout << "Total rectangles: " << num_rects << "\n";
        cout << "MPI processes: " << size << "\n\n";

        cout << "Letter view:\n\n";
        for (const auto& row : letter_grid) {
            cout << row << "\n";
        }

        cout << "\nOwnership view (digits show MPI rank):\n\n";
        for (const auto& row : owner_grid) {
            cout << row << "\n";
        }

        cout << "\nRectangle list:\n";
        for (int i = 0; i < num_rects; ++i) {
            cout << "Rect " << setw(2) << i
                 << " | Letter " << rects[i].letter
                 << " | Owner rank " << (i % size)
                 << " | (" << rects[i].x1 << "," << rects[i].y1 << ") to ("
                 << rects[i].x2 << "," << rects[i].y2 << ")\n";
        }

        cout << "\nExplanation:\n";
        cout << "The word KREA is represented as a union of rectangular subdomains.\n";
        cout << "This follows the Chapter 8.4 idea of decomposing a word-shaped\n";
        cout << "domain into rectangles for parallel assignment across processors.\n";
    }

    MPI_Finalize();
    return 0;
}
