# Laplace Parallel Assignment

This project contains implementations of a 2D Laplace equation solver using:

- Serial C++
- Serial Python
- OpenMP C++
- MPI C++
- MPI Python
- Ray Python

It also includes an MPI C++ domain decomposition program for the word **KREA**, inspired by rectangular domain decomposition ideas.

## Problem solved

I solve the 2D Laplace equation on a square domain using a finite-difference discretization and Jacobi iteration.

## Boundary conditions

- Top boundary = 100
- Bottom boundary = 0
- Left boundary = 0
- Right boundary = 0

## Folder structure

- `src/` → source code
- `results/` → timings, profiles, solution outputs, plots
- `report/` → final PDF report

## Planned files

- `src/serial_laplace.py`
- `src/serial_laplace.cpp`
- `src/omp_laplace.cpp`
- `src/mpi_laplace.cpp`
- `src/mpi_laplace.py`
- `src/ray_laplace.py`
- `src/mpi_krea_decompose.cpp`

## Notes

All implementations use the same mathematical formulation so that performance comparisons are meaningful.
