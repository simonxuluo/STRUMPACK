/*
 * STRUMPACK -- STRUctured Matrices PACKage, Copyright (c) 2014, The
 * Regents of the University of California, through Lawrence Berkeley
 * National Laboratory (subject to receipt of any required approvals
 * from the U.S. Dept. of Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE. This software is owned by the U.S. Department of Energy. As
 * such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * Developers: Pieter Ghysels, Francois-Henry Rouet, Xiaoye S. Li.
 *             (Lawrence Berkeley National Lab, Computational Research
 *             Division).
 *
 */
#include <iostream>
#include "StrumpackSparseSolver.hpp"
#include "StrumpackSparseSolverMixedPrecision.hpp"
#include "sparse/CSRMatrix.hpp"

using namespace strumpack;

int main(int argc, char* argv[]) {
  int n = 30; // matrix size
  int m = 1;  // number of right-hand sides
  if (argc > 1) n = atoi(argv[1]); // get grid size
  else std::cout << "# please provide grid size" << std::endl;

  std::cout << "# Solving 3d " << n
            <<"^3 Poisson problem, with " << m << " right hand sides"
            << std::endl;

#if 1 /* mixed precision solver */

  StrumpackSparseSolverMixedPrecision<float,double,int> spss;
  /** options for the outer solver */
  // spss.options().set_Krylov_solver(KrylovSolver::REFINE);
  // spss.options().set_Krylov_solver(KrylovSolver::PREC_BICGSTAB);
  spss.options().set_Krylov_solver(KrylovSolver::PREC_GMRES);
  spss.options().set_from_command_line(argc, argv);

  /* options for the inner solver */
  spss.solver().options().set_Krylov_solver(KrylovSolver::DIRECT);
  spss.solver().options().set_matching(MatchingJob::NONE);
  spss.solver().options().set_reordering_method(ReorderingStrategy::GEOMETRIC);
  spss.solver().options().set_from_command_line(argc, argv);

#else /* standard solver */

  StrumpackSparseSolver<double,int> spss;
  spss.options().set_matching(MatchingJob::NONE);
  spss.options().set_reordering_method(ReorderingStrategy::GEOMETRIC);
  spss.options().set_from_command_line(argc, argv);

#endif

  int n2 = n * n;
  int N = n * n2;
  int nnz = 7 * N - 6 * n2;
  CSRMatrix<double,int> A(N, nnz);
  auto cptr = A.ptr();
  auto rind = A.ind();
  auto val = A.val();

  nnz = 0;
  cptr[0] = 0;
  for (int xdim=0; xdim<n; xdim++)
    for (int ydim=0; ydim<n; ydim++)
      for (int zdim=0; zdim<n; zdim++) {
        int ind = zdim+ydim*n+xdim*n2;
        val[nnz] = 6.0;
        rind[nnz++] = ind;
        if (zdim > 0)  { val[nnz] = -1.0; rind[nnz++] = ind-1; } // left
        if (zdim < n-1){ val[nnz] = -1.0; rind[nnz++] = ind+1; } // right
        if (ydim > 0)  { val[nnz] = -1.0; rind[nnz++] = ind-n; } // front
        if (ydim < n-1){ val[nnz] = -1.0; rind[nnz++] = ind+n; } // back
        if (xdim > 0)  { val[nnz] = -1.0; rind[nnz++] = ind-n2; } // up
        if (xdim < n-1){ val[nnz] = -1.0; rind[nnz++] = ind+n2; } // down
        cptr[ind+1] = nnz;
      }
  A.set_symm_sparse();

  DenseMatrix<double> b(N, m), x(N, m), x_exact(N, m);

  x_exact.random();
  A.spmv(x_exact, b);

  spss.set_matrix(A);
  spss.reorder(n, n, n);
  spss.factor();

  // spss.move_to_gpu();
  spss.solve(b, x);
  // spss.remove_from_gpu();

  std::cout << "# COMPONENTWISE SCALED RESIDUAL = "
            << A.max_scaled_residual(x.data(), b.data()) << std::endl;

  return 0;
}
