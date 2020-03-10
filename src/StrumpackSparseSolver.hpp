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
 */
/**
 * \file StrumpackSparseSolver.hpp
 * \brief Contains the definition of the sequential/multithreaded
 * sparse solver class.
 */
#ifndef STRUMPACK_SPARSE_SOLVER_HPP
#define STRUMPACK_SPARSE_SOLVER_HPP

#include <new>
#include <memory>
#include <vector>
#include <string>

#include "StrumpackConfig.hpp"
#include "StrumpackOptions.hpp"
#include "sparse/CSRMatrix.hpp"
#include "dense/DenseMatrix.hpp"

/**
 * All of STRUMPACK is contained in the strumpack namespace.
 */
namespace strumpack {

  // forward declatations
  template<typename scalar_t,typename integer_t> class MatrixReordering;
  template<typename scalar_t,typename integer_t> class EliminationTree;
  class TaskTimer;

  /**
   * \class StrumpackSparseSolver
   *
   * \brief StrumpackSparseSolver is the main sequential or
   * multithreaded sparse solver class.
   *
   * This is the main interface to STRUMPACK's sparse solver. Use this
   * for a sequential or multithreaded sparse solver. For the fully
   * distributed solver, see StrumpackSparseSolverMPIDist.
   *
   * \tparam scalar_t can be: float, double, std::complex<float> or
   * std::complex<double>.
   *
   * \tparam integer_t defaults to a regular int. If regular int
   * causes 32 bit integer overflows, you should switch to
   * integer_t=int64_t instead. This should be a __signed__ integer
   * type.
   *
   * \see StrumpackSparseSolverMPIDist, StrumpackSparseSolverMPI
   */
  template<typename scalar_t,typename integer_t=int>
  class StrumpackSparseSolver {
    using SpMat_t = CompressedSparseMatrix<scalar_t,integer_t>;
    using Tree_t = EliminationTree<scalar_t,integer_t>;
    using Reord_t = MatrixReordering<scalar_t,integer_t>;
    using DenseM_t = DenseMatrix<scalar_t>;
    using DenseMW_t = DenseMatrixWrapper<scalar_t>;

  public:

    /**
     * Constructor of the StrumpackSparseSolver class, taking command
     * line arguments.
     *
     * \param argc number of arguments, i.e, number of elements in
     * the argv array
     * \param argv command line arguments. Add -h or --help to have a
     * description printed
     * \param verbose flag to enable/disable output to cout
     * \param root flag to denote whether this process is the root MPI
     * process, only the root will print certain messages to cout
     */
    StrumpackSparseSolver
    (int argc, char* argv[], bool verbose=true, bool root=true);

    /**
     * Constructor of the StrumpackSparseSolver class.
     *
     * \param verbose flag to enable/disable output to cout
     * \param root flag to denote whether this process is the root MPI
     * process. Only the root will print certain messages
     * \see set_from_options
     */
    StrumpackSparseSolver(bool verbose=true, bool root=true);

    /**
     * (Virtual) destructor of the StrumpackSparseSolver class.
     */
    virtual ~StrumpackSparseSolver();

    /**
     * Associate a (sequential) CSRMatrix with this solver.
     *
     * This matrix will not be modified. An internal copy will be
     * made, so it is safe to delete the data immediately after
     * calling this function. When this is called on a
     * StrumpackSparseSolverMPIDist object, the input matrix will
     * immediately be distributed over all the processes in the
     * communicator associated with the solver. This method is thus
     * collective on the MPI communicator associated with the solver.
     *
     * \param A A CSRMatrix<scalar_t,integer_t> object, will
     * internally be duplicated
     *
     * \see set_csr_matrix
     */
    virtual void set_matrix(const CSRMatrix<scalar_t,integer_t>& A);

    /**
     * Associate a (sequential) NxN CSR matrix with this solver.
     *
     * This matrix will not be modified. An internal copy will be
     * made, so it is safe to delete the data immediately after
     * calling this function. When this is called on a
     * StrumpackSparseSolverMPIDist object, the input matrix will
     * immediately be distributed over all the processes in the
     * communicator associated with the solver. This method is thus
     * collective on the MPI communicator associated with the solver.
     * See the manual for a description of the CSR format. You can
     * also use the CSRMatrix class.
     *
     * \param N number of rows and columns of the CSR input matrix.
     * \param row_ptr indices in col_ind and values for the start of
     * each row. Nonzeros for row r are in [row_ptr[r],row_ptr[r+1])
     * \param col_ind column indices of each nonzero
     * \param values nonzero values
     * \param symmetric_pattern denotes whether the sparsity
     * __pattern__ of the input matrix is symmetric, does not require
     * the matrix __values__ to be symmetric
     *
     * \see set_matrix
     */
    virtual void set_csr_matrix
    (integer_t N, const integer_t* row_ptr, const integer_t* col_ind,
     const scalar_t* values, bool symmetric_pattern=false);

    /**
     * This can only be used to UPDATE the nonzero values of the
     * matrix. So it should be called with exactly the same sparsity
     * pattern (row_ptr and col_ind) as used to set the initial matrix
     * (using set_matrix or set_csr_matrix). This routine can be
     * called after the factorization. In that case, for the next
     * ordering phase, the permutation previously computed will be
     * reused to permute the updated matrix values
     *
     *
     * TODO
     */
    void update_matrix_values
    (integer_t N, const integer_t* row_ptr, const integer_t* col_ind,
     const scalar_t* values, bool symmetric_pattern);

    /**
     * TODO
     *
     */
    void update_matrix_values
    (const CSRMatrix<scalar_t,integer_t>& A);

    /**
     * Compute matrix reorderings for numerical stability and to
     * reduce fill-in.
     *
     * Start computation of the matrix reorderings. See the relevant
     * options to control the matrix reordering in the manual. A first
     * reordering is the MC64 column permutation for numerical
     * stability. This can be disabled if the matrix has large nonzero
     * diagonal entries. MC64 optionally also performs row and column
     * scaling. Next, a fill-reducing reordering is computed. This is
     * done with the nested dissection algortihms of either
     * (PT-)Scotch, (Par)Metis or a simple geometric nested dissection
     * code which only works on regular meshes.
     *
     * \param nx this (optional) parameter is only meaningful when the
     * matrix corresponds to a stencil on a regular mesh. The stencil
     * is assumed to be at most 3 points wide in each dimension and
     * only contain a single degree of freedom per grid point. The nx
     * parameter denotes the number of grid points in the first
     * spatial dimension.
     * \param ny see parameters nx. Parameter ny denotes the number of
     * gridpoints in the second spatial dimension.
     * This should only be set if the mesh is 2 or 3 dimensional.
     * \param nz See parameters nx. Parameter nz denotes the number of
     * gridpoints in the third spatial dimension.
     * This should only be set if the mesh is 3 dimensional.
     * \return error code
     * \see SPOptions
     */
    virtual ReturnCode reorder
    (int nx=1, int ny=1, int nz=1, int components=1, int width=1);


    /**
     * Perform sparse matrix reordering, with a user-supplied
     * permutation vector. Using this will ignore the reordering
     * method selected in the options struct.
     *
     * \param p permutation vector, should be of size N, the size of
     * the sparse matrix associated with this solver
     * \param base is the permutation 0 or 1 based?
     */
    virtual ReturnCode reorder(const int* p, int base=0);

    /**
     * Perform numerical factorization of the sparse input matrix.
     *
     * This is the computationally expensive part of the
     * code. However, once the factorization is performed, it can be
     * reused for multiple solves. This requires that a valid matrix
     * has been assigned to this solver. internally this check whether
     * the matrix has been reordered (with a call to reorder), and if
     * not, it will call reorder.
     *
     * \return error code
     * \see set_matrix, set_csr_matrix
     */
    ReturnCode factor();

    /**
     * Solve a linear system with a single right-hand side. Before
     * being able to solve a linear system, the matrix needs to be
     * factored. One can call factor() explicitly, or if this was not
     * yet done, this routine will call factor() internally.
     *
     * \param b input, will not be modified. Pointer to the right-hand
     * side. Array should be lenght N, the dimension of the input
     * matrix for StrumpackSparseSolver and
     * StrumpackSparseSolverMPI. For StrumpackSparseSolverMPIDist, the
     * length of b should be correspond the partitioning of the
     * block-row distributed input matrix.
     * \param x Output, pointer to the solution vector.  Array should
     * be lenght N, the dimension of the input matrix for
     * StrumpackSparseSolver and StrumpackSparseSolverMPI. For
     * StrumpackSparseSolverMPIDist, the length of b should be
     * correspond the partitioning of the block-row distributed input
     * matrix.
     * \param use_initial_guess set to true if x contains an intial
     * guess to the solution. This is mainly useful when using an
     * iterative solver. If set to false, x should not be set (but
     * should be allocated).
     * \return error code
     */
    virtual ReturnCode solve
    (const scalar_t* b, scalar_t* x, bool use_initial_guess=false);

    /**
     * Solve a linear system with a single or multiple right-hand
     * sides. Before being able to solve a linear system, the matrix
     * needs to be factored. One can call factor() explicitly, or if
     * this was not yet done, this routine will call factor()
     * internally.
     *
     * \param b input, will not be modified. DenseMatrix containgin
     * the right-hand side vector/matrix. Should have N rows, with N
     * the dimension of the input matrix for StrumpackSparseSolver and
     * StrumpackSparseSolverMPI. For StrumpackSparseSolverMPIDist, the
     * number or rows of b should be correspond to the partitioning of
     * the block-row distributed input matrix.
     * \param x Output, pointer to the solution vector.  Array should
     * be lenght N, the dimension of the input matrix for
     * StrumpackSparseSolver and StrumpackSparseSolverMPI. For
     * StrumpackSparseSolverMPIDist, the length of b should be
     * correspond the partitioning of the block-row distributed input
     * matrix.
     * \param use_initial_guess set to true if x contains an intial
     * guess to the solution.  This is mainly useful when using an
     * iterative solver.  If set to false, x should not be set (but
     * should be allocated).
     * \return error code
     * \see DenseMatrix, solve(), factor()
     */
    virtual ReturnCode solve
    (const DenseM_t& b, DenseM_t& x, bool use_initial_guess=false);

    /**
     * Return the object holding the options for this sparse solver.
     */
    SPOptions<scalar_t>& options();

    /**
     * Return the object holding the options for this sparse solver.
     */
    const SPOptions<scalar_t>& options() const;

    /**
     * Parse the command line options passed in the constructor, and
     * modify the options object accordingly. Run with option -h or
     * --help to see a list of supported options. Or check the
     * SPOptions documentation.
     */
    void set_from_options();

    /**
     * Parse the command line options, and modify the options object
     * accordingly. Run with option -h or --help to see a list of
     * supported options. Or check the SPOptions documentation.
     *
     * \param argc number of options in argv
     * \param argv list of options
     */
    void set_from_options(int argc, char* argv[]);

    /**
     * Return the maximum rank encountered in any of the HSS matrices
     * used to compress the sparse triangular factors. This should be
     * called after the factorization phase. For the
     * StrumpackSparseSolverMPI and StrumpackSparseSolverMPIDist
     * distributed memory solvers, this routine is collective on the
     * MPI communicator.
     */
    int maximum_rank() const;

    /**
     * Return the number of nonzeros in the (sparse) factors. This is
     * known as the fill-in. This should be called after computing the
     * numerical factorization. For the StrumpackSparseSolverMPI and
     * StrumpackSparseSolverMPIDist distributed memory solvers, this
     * routine is collective on the MPI communicator.
     */
    std::size_t factor_nonzeros() const;

    /**
     * Return the amount of memory taken by the sparse factorization
     * factors. This is the fill-in. It is simply computed as
     * factor_nonzeros() * sizeof(scalar_t), so it does not include
     * any overhead from the metadata for the datastructures. This
     * should be called after the factorization. For the
     * StrumpackSparseSolverMPI and StrumpackSparseSolverMPIDist
     * distributed memory solvers, this routine is collective on the
     * MPI communicator.
     */
    std::size_t factor_memory() const;

    /**
     * Return the number of iterations performed by the outer (Krylov)
     * iterative solver. Call this after calling the solve routine.
     */
    int Krylov_iterations() const;

    /**
     * Create a gnuplot script to draw/plot the sparse factors. Only
     * do this for small matrices! It is very slow!
     *
     * \param name filename of the generated gnuplot script. Running
     * \verbatim gnuplot plotname.gnuplot \endverbatim will generate a
     * pdf file.
     */
    void draw(const std::string& name) const;

  protected:
    virtual void setup_tree();
    virtual void setup_reordering();
    virtual int compute_reordering
    (const int* p, int base, int nx, int ny, int nz,
     int components, int width);
    virtual void separator_reordering();

    virtual SpMat_t* matrix() { return mat_.get(); }
    virtual Reord_t* reordering() { return nd_.get(); }
    virtual Tree_t* tree() { return tree_.get(); }
    virtual const SpMat_t* matrix() const { return mat_.get(); }
    virtual const Reord_t* reordering() const { return nd_.get(); }
    virtual const Tree_t* tree() const { return tree_.get(); }

    void papi_initialize();
    inline long long dense_factor_nonzeros() const;
    void print_solve_stats(TaskTimer& t) const;
    virtual void perf_counters_start();
    virtual void perf_counters_stop(const std::string& s);
    virtual void synchronize() {}
    virtual void communicate_ordering() {}
    virtual void print_flop_breakdown_HSS() const;
    virtual void print_flop_breakdown_HODLR() const;
    virtual void flop_breakdown_reset() const;
    virtual void reduce_flop_counters() const {}

    ReturnCode internal_reorder
    (const int* p, int base, int nx, int ny, int nz,
     int components, int width);

    SPOptions<scalar_t> opts_;
    bool is_root_;
    std::vector<integer_t> cperm_;
    std::vector<scalar_t> Dr_, Dc_; // row/col scaling
    std::new_handler old_handler_;
    std::ostream* rank_out_ = nullptr;
    bool factored_ = false;
    bool reordered_ = false;
    int Krylov_its_ = 0;

#if defined(STRUMPACK_USE_PAPI)
    float rtime_ = 0., ptime_ = 0.;
    long_long _flpops = 0;
#endif
#if defined(STRUMPACK_COUNT_FLOPS)
    long long int f0_ = 0, ftot_ = 0, fmin_ = 0, fmax_ = 0;
    long long int b0_ = 0, btot_ = 0, bmin_ = 0, bmax_ = 0;
#endif

  private:
    void permute_matrix_values();
    void print_wrong_sparsity_error();

    std::unique_ptr<CSRMatrix<scalar_t,integer_t>> mat_;
    std::unique_ptr<MatrixReordering<scalar_t,integer_t>> nd_;
    std::unique_ptr<EliminationTree<scalar_t,integer_t>> tree_;
  };

} //end namespace strumpack

#endif // STRUMPACK_SPARSE_SOLVER_HPP
