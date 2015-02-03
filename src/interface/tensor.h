#ifndef __TENSOR_H__
#define __TENSOR_H__

#include "functions.h"
#include "semiring.h"
#include "../tensor/untyped_tensor.h"
#include "world.h"
#include <vector>

namespace CTF {

  template <typename dtype> class Idx_Tensor;
  template <typename dtype> class Sparse_Tensor;


  /**
   * \defgroup CTF CTF: C++ Tensor interface
   * @{
   */

  /**
   * \brief an instance of a tensor within a CTF world
   */
  template <typename dtype=double>
  class Tensor : public CTF_int::tensor {
    public:
      /** \brief templated semiring on which tensor elements and operations are defined */
      //Semiring<dtype> typed_ring;

      /**
       * \brief default constructor
       */
      Tensor();

      /**
       * \brief copies a tensor (setting data to zero or copying A)
       * \param[in] A tensor to copy
       * \param[in] copy whether to copy the data of A into the new tensor
       */
      Tensor(Tensor const & A,
             bool           copy = true);

      /**
       * \brief defines tensor filled with zeros on the default semiring
       * \param[in] order_ number of dimensions of tensor
       * \param[in] len_ edge lengths of tensor
       * \param[in] sym_ symmetries of tensor (e.g. symmetric matrix -> sym={SY, NS})
       * \param[in] wrld_ a world for the tensor to live in
       * \param[in] name_ an optionary name for the tensor
       * \param[in] profile_ set to 1 to profile contractions involving this tensor
       */
      /*Tensor(int          dim,
             int const *  len,
             int const *  sym,
             World &      wrld,
             char const * name = NULL,
             int          profile = 0);*/


      /**
       * \brief defines a tensor filled with zeros on a specified semiring
       * \param[in] order_ number of dimensions of tensor
       * \param[in] len_ edge lengths of tensor
       * \param[in] sym_ symmetries of tensor (e.g. symmetric matrix -> sym={SY, NS})
       * \param[in] world_ a world for the tensor to live in
       * \param[in] sr_ defines the tensor arithmetic for this tensor
       * \param[in] name_ an optionary name for the tensor
       * \param[in] profile_ set to 1 to profile contractions involving this tensor
       */
      Tensor(int             order,
             int const *     len,
             int const *     sym,
             World &         wrld,
             Semiring<dtype> sr = Ring<dtype>(),
             char const *    name = NULL,
             int             profile = 0);

      
      /**
       * \brief creates a zeroed out copy (data not copied) of a tensor in a different world
       * \param[in] A tensor whose characteristics to copy
       * \param[in] world_ a world for the tensor we are creating to live in, can be different from A
       */
      Tensor(Tensor const & A,
             World &        wrld);

      /**
       * \brief gives the values associated with any set of indices
       * The sparse data is defined in coordinate format. The tensor index (i,j,k,l) of a tensor with edge lengths
       * {m,n,p,q} is associated with the global index g via the formula g=i+j*m+k*m*n+l*m*n*p. The row index is first
       * and the column index is second for matrices, which means they are column major. 
       * \param[in] npair number of values to fetch
       * \param[in] global_idx index within global tensor of each value to fetch
       * \param[in,out] data a prealloced pointer to the data with the specified indices
       */
      void read(int64_t          npair,
                int64_t const *  global_idx,
                dtype *          data) const;
      
      /**
       * \brief gives the values associated with any set of indices
       * \param[in] npair number of values to fetch
       * \param[in,out] pairs a prealloced pointer to key-value pairs
       */
      void read(int64_t       npair,
                Pair<dtype> * pairs) const;
      
      /**
       * \brief sparse read: A[global_idx[i]] = alpha*A[global_idx[i]]+beta*data[i]
       * \param[in] npair number of values to read into tensor
       * \param[in] alpha scaling factor on read data
       * \param[in] beta scaling factor on value in initial values vector
       * \param[in] global_idx global index within tensor of value to add
       * \param[in] data values to add to the tensor
       */
      void read(int64_t          npair,
                dtype            alpha,
                dtype            beta,
                int64_t  const * global_idx,
                dtype *          data) const;

      /**
       * \brief sparse read: pairs[i].d = alpha*A[pairs[i].k]+beta*pairs[i].d
       * \param[in] npair number of values to read into tensor
       * \param[in] alpha scaling factor on read data
       * \param[in] beta scaling factor on value in initial pairs vector
       * \param[in] pairs key-value pairs to add to the tensor
       */
      void read(int64_t       npair,
                dtype         alpha,
                dtype         beta,
                Pair<dtype> * pairs) const;
     

      /**
       * \brief writes in values associated with any set of indices
       * The sparse data is defined in coordinate format. The tensor index (i,j,k,l) of a tensor with edge lengths
       * {m,n,p,q} is associated with the global index g via the formula g=i+j*m+k*m*n+l*m*n*p. The row index is first
       * and the column index is second for matrices, which means they are column major. 
       * \param[in] npair number of values to write into tensor
       * \param[in] global_idx global index within tensor of value to write
       * \param[in] data values to  write to the indices
       */
      void write(int64_t          npair,
                 int64_t  const * global_idx,
                 dtype const    * data);

      /**
       * \brief writes in values associated with any set of indices
       * \param[in] npair number of values to write into tensor
       * \param[in] pairs key-value pairs to write to the tensor
       */
      void write(int64_t             npair,
                 Pair<dtype> const * pairs);
      
      /**
       * \brief sparse add: A[global_idx[i]] = beta*A[global_idx[i]]+alpha*data[i]
       * \param[in] npair number of values to write into tensor
       * \param[in] alpha scaling factor on value to add
       * \param[in] beta scaling factor on original data
       * \param[in] global_idx global index within tensor of value to add
       * \param[in] data values to add to the tensor
       */
      void write(int64_t          npair,
                 dtype            alpha,
                 dtype            beta,
                 int64_t  const * global_idx,
                 dtype const *    data);

      /**
       * \brief sparse add: A[pairs[i].k] = alpha*A[pairs[i].k]+beta*pairs[i].d
       * \param[in] npair number of values to write into tensor
       * \param[in] alpha scaling factor on value to add
       * \param[in] beta scaling factor on original data
       * \param[in] pairs key-value pairs to add to the tensor
       */
      void write(int64_t             npair,
                 dtype               alpha,
                 dtype               beta,
                 Pair<dtype> const * pairs);
     
      /**
       * \brief contracts C[idx_C] = beta*C[idx_C] + alpha*A[idx_A]*B[idx_B]
       * \param[in] alpha A*B scaling factor
       * \param[in] A first operand tensor
       * \param[in] idx_A indices of A in contraction, e.g. "ik" -> A_{ik}
       * \param[in] B second operand tensor
       * \param[in] idx_B indices of B in contraction, e.g. "kj" -> B_{kj}
       * \param[in] beta C scaling factor
       * \param[in] idx_C indices of C (this tensor),  e.g. "ij" -> C_{ij}
       */
      void contract(dtype        alpha,
                    Tensor &     A,
                    char const * idx_A,
                    Tensor &     B,
                    char const * idx_B,
                    dtype        beta,
                    char const * idx_C);

      /**
       * \brief contracts computes fseq(A[idx_A],B[idx_B],C[idx_C])
       * \param[in] A first operand tensor
       * \param[in] idx_A indices of A in contraction, e.g. "ik" -> A_{ik}
       * \param[in] B second operand tensor
       * \param[in] idx_B indices of B in contraction, e.g. "kj" -> B_{kj}
       * \param[in] idx_C indices of C (this tensor),  e.g. "ij" -> C_{ij}
       * \param[in] fseq sequential operation to execute, default is multiply-add
       */
      void contract(Tensor &              A,
                    char const *          idx_A,
                    Tensor &              B,
                    char const *          idx_B,
                    char const *          idx_C,
                    Bivar_Function<dtype> fseq);

      /**
       * \brief sums B[idx_B] = beta*B[idx_B] + alpha*A[idx_A]
       * \param[in] alpha A scaling factor
       * \param[in] A first operand tensor
       * \param[in] idx_A indices of A in sum, e.g. "ij" -> A_{ij}
       * \param[in] beta B scaling factor
       * \param[in] idx_B indices of B (this tensor), e.g. "ij" -> B_{ij}
       */
      void sum(dtype        alpha,
               Tensor &     A,
               char const * idx_A,
               dtype        beta,
               char const * idx_B);
      
      /**
       * \brief  computes fseq(alpha,A[idx_A],beta*B[idx_B])
       * \param[in] A first operand tensor
       * \param[in] idx_A indices of A in sum, e.g. "ij" -> A_{ij}
       * \param[in] idx_B indices of B (this tensor), e.g. "ij" -> B_{ij}
       * \param[in] fseq sequential operation to execute, default is multiply-add
       */
      void sum(Tensor &               A,
               char const *           idx_A,
               char const *           idx_B,
               Univar_Function<dtype> fseq);
      
      /**
       * \brief scales A[idx_A] = alpha*A[idx_A]
       * \param[in] alpha A scaling factor
       * \param[in] idx_A indices of A (this tensor), e.g. "ij" -> A_{ij}
       */
      void scale(dtype        alpha,
                 char const * idx_A);

      /**
       * \brief computes fseq(alpha,A[idx_A])
       * \param[in] idx_A indices of A (this tensor), e.g. "ij" -> A_{ij}
       * \param[in] fseq sequential operation to execute, default is multiply-add
       */
      void scale(char const *        idx_A,
                 Endomorphism<dtype> fseq);


      /**
       * \brief estimate the time of a contraction C[idx_C] = A[idx_A]*B[idx_B]
       * \param[in] A first operand tensor
       * \param[in] idx_A indices of A in contraction, e.g. "ik" -> A_{ik}
       * \param[in] B second operand tensor
       * \param[in] idx_B indices of B in contraction, e.g. "kj" -> B_{kj}
       * \param[in] idx_C indices of C (this tensor),  e.g. "ij" -> C_{ij}
       * \return time in seconds, at the moment not at all precise
       */
      double estimate_time(Tensor &     A,
                           char const * idx_A,
                           Tensor &     B,
                           char const * idx_B,
                           char const * idx_C);
      
      /**
       * \brief estimate the time of a sum B[idx_B] = A[idx_A]
       * \param[in] A first operand tensor
       * \param[in] idx_A indices of A in contraction, e.g. "ik" -> A_{ik}
       * \param[in] idx_B indices of B in contraction, e.g. "kj" -> B_{kj}
       * \return time in seconds, at the moment not at all precise
       */
      double estimate_time(Tensor &     A,
                           char const * idx_A,
                           char const * idx_B);

      /**
       * \brief cuts out a slice (block) of this tensor A[offsets,ends)
       * \param[in] offsets bottom left corner of block
       * \param[in] ends top right corner of block
       * \return new tensor corresponding to requested slice
       */
      Tensor slice(int const * offsets,
                   int const * ends) const;
      
      /**
       * \brief cuts out a slice (block) of this tensor with corners specified by global index
       * \param[in] corner_off top left corner of block
       * \param[in] corner_end bottom right corner of block
       * \return new tensor corresponding to requested slice
      */
      Tensor slice(int64_t corner_off,
                   int64_t corner_end) const;
      
      /**
       * \brief cuts out a slice (block) of this tensor A[offsets,ends)
       * \param[in] offsets bottom left corner of block
       * \param[in] ends top right corner of block
       * \param[in] oworld the world in which the new tensor should be defined
       * \return new tensor corresponding to requested slice which lives on
       *          oworld
       */
      Tensor slice(int const * offsets,
                   int const * ends,
                   World *     oworld) const;

      /**
       * \brief cuts out a slice (block) of this tensor with corners specified by global index
       * \param[in] corner_off top left corner of block
       * \param[in] corner_end bottom right corner of block
       * \param[in] oworld the world in which the new tensor should be defined
       * \return new tensor corresponding to requested slice which lives on
       *          oworld
       */
      Tensor slice(int64_t corner_off,
                   int64_t corner_end,
                   World * oworld) const;
      
      
      /**
       * \brief cuts out a slice (block) of this tensor = B
       *   B[offsets,ends)=beta*B[offsets,ends) + alpha*A[offsets_A,ends_A)
       * \param[in] offsets bottom left corner of block
       * \param[in] ends top right corner of block
       * \param[in] alpha scaling factor of this tensor
       * \param[in] A tensor who owns pure-operand slice
       * \param[in] offsets bottom left corner of block of A
       * \param[in] ends top right corner of block of A
       * \param[in] alpha scaling factor of tensor A
       */
      void slice(int const *    offsets,
                 int const *    ends,
                 dtype          beta,
                 Tensor const & A,
                 int const *    offsets_A,
                 int const *    ends_A,
                 dtype          alpha);

      /**
       * \brief cuts out a slice (block) of this tensor = B
       *   B[offsets,ends)=beta*B[offsets,ends) + alpha*A[offsets_A,ends_A)
       * \param[in] corner_off top left corner of block
       * \param[in] corner_end bottom right corner of block       
       * \param[in] alpha scaling factor of this tensor
       * \param[in] A tensor who owns pure-operand slice
       * \param[in] corner_off top left corner of block of A
       * \param[in] corner_end bottom right corner of block of A
       * \param[in] alpha scaling factor of tensor A
       */
      void slice(int64_t        corner_off,
                 int64_t        corner_end,
                 dtype          beta,
                 Tensor const & A,
                 int64_t        corner_off_A,
                 int64_t        corner_end_A,
                 dtype          alpha);

      /**
       * \brief Apply permutation to matrix, potentially extracting a slice
       *              B[i,j,...] 
       *                = beta*B[...] + alpha*A[perms_A[0][i],perms_A[1][j],...]
       *
       * \param[in] beta scaling factor for values of tensor B (this)
       * \param[in] A specification of operand tensor A must live on 
                      the same World or a subset of the World on which B lives
       * \param[in] perms_A specifies permutations for tensor A, e.g. A[perms_A[0][i],perms_A[1][j]]
       *                    if a subarray NULL, no permutation applied to this index,
       *                    if an entry is -1, the corresponding entries of the tensor are skipped 
                              (A must then be smaller than B)
       * \param[in] alpha scaling factor for A tensor
       */
      void permute(dtype         beta,
                   Tensor &      A,
                   int * const * perms_A,
                   dtype         alpha);

      /**
       * \brief Apply permutation to matrix, potentially extracting a slice
       *              B[perms_B[0][i],perms_B[0][j],...] 
       *                = beta*B[...] + alpha*A[i,j,...]
       *
       * \param[in] perms_B specifies permutations for tensor B, e.g. B[perms_B[0][i],perms_B[1][j]]
       *                    if a subarray NULL, no permutation applied to this index,
       *                    if an entry is -1, the corresponding entries of the tensor are skipped 
       *                       (A must then be smaller than B)
       * \param[in] beta scaling factor for values of tensor B (this)
       * \param[in] A specification of operand tensor A must live on 
                      the same World or a superset of the World on which B lives
       * \param[in] alpha scaling factor for A tensor
       */
      void permute(int * const * perms_B,
                   dtype         beta,
                   Tensor &      A,
                   dtype         alpha);
      
     /**
       * \brief accumulates this tensor to a tensor object defined on a different world
       * \param[in] tsr a tensor object of the same characteristic as this tensor,
       *             but on a different world/MPI_comm
       * \param[in] alpha scaling factor for this tensor (default 1.0)
       * \param[in] beta scaling factor for tensor tsr (default 1.0)
       */
      void add_to_subworld(Tensor<dtype> * tsr,
                           dtype           alpha,
                           dtype           beta) const;
     /**
       * \brief accumulates this tensor to a tensor object defined on a different world
       * \param[in] tsr a tensor object of the same characteristic as this tensor,
       *             but on a different world/MPI_comm
       */

      void add_to_subworld(Tensor<dtype> * tsr) const;
      
      /**
       * \brief accumulates this tensor from a tensor object defined on a different world
       * \param[in] tsr a tensor object of the same characteristic as this tensor,
       *             but on a different world/MPI_comm
       * \param[in] alpha scaling factor for tensor tsr (default 1.0)
       * \param[in] beta scaling factor for this tensor (default 1.0)
       */
      void add_from_subworld(Tensor<dtype> * tsr,
                             dtype           alpha,
                             dtype           beta) const;
      /**
       * \brief accumulates this tensor from a tensor object defined on a different world
       * \param[in] tsr a tensor object of the same characteristic as this tensor,
       *             but on a different world/MPI_comm
       */
      void add_from_subworld(Tensor<dtype> * tsr) const;
      

      /**
       * \brief aligns data mapping with tensor A
       * \param[in] A align with this tensor
       */
      void align(Tensor const & A);

      /**
       * \brief performs a reduction on the tensor
       * \param[in] op reduction operation (see top of this cyclopstf.hpp for choices)
       */    
      dtype reduce(OP op);
      
      /**
       * \brief map data according to global index
       * \param[in]  map_funcfunction that takes indices and tensor element value and returns new value 
       */
      void map_tensor(dtype (*map_func)(int         order,
                                        int const * indices,
                                        dtype       elem));

      /**
       * \brief computes the entrywise 1-norm of the tensor
       */    
      dtype norm1(){ return reduce(OP_NORM1); };

      /**
       * \brief computes the frobenius norm of the tensor
       */    
      dtype norm2(){ return reduce(OP_NORM2); };

      /**
       * \brief finds the max absolute value element of the tensor
       */    
      dtype norm_infty(){ return reduce(OP_MAXABS); };

      /**
       * \brief gives the raw current local data with padding included
       * \param[out] size of local data chunk
       * \return pointer to local data
       */
      dtype * get_raw_data(int64_t * size) const;

      /**
       * \brief gives a read-only copy of the raw current local data with padding included
       * \param[out] size of local data chunk
       * \return pointer to read-only copy of local data
       */
      const dtype * raw_data(int64_t * size) const;

      /**
       * \brief gives the global indices and values associated with the local data
       * \param[out] npair number of local values
       * \param[out] global_idx index within global tensor of each data value
       * \param[out] data pointer to local values in the order of the indices
       */
      void read_local(int64_t  *  npair,
                      int64_t  ** global_idx,
                      dtype **    data) const;

      /**
       * \brief gives the global indices and values associated with the local data
       * \param[out] npair number of local values
       * \param[out] pairs pointer to local key-value pairs
       */
      void read_local(int64_t  *     npair,
                      Pair<dtype> ** pairs) const;

      /**
       * \brief collects the entire tensor data on each process (not memory scalable)
       * \param[out] npair number of values in the tensor
       * \param[out] data pointer to the data of the entire tensor
       */
      void read_all(int64_t  * npair,
                    dtype **   data) const;
      
      /**
       * \brief collects the entire tensor data on each process (not memory scalable)
       * \param[in,out] preallocated data pointer to the data of the entire tensor
       */
      int64_t read_all(dtype * data) const;

      /**
       * \brief obtains a small number of the biggest elements of the 
       *        tensor in sorted order (e.g. eigenvalues)
       * \param[in] n number of elements to collect
       * \param[in] data output data (should be preallocated to size at least n)
       *
       * WARNING: currently functional only for dtype=double
       */
      void get_max_abs(int     n,
                       dtype * data) const;

      /**
       * \brief turns on profiling for tensor
       */
      void profile_on();
      
      /**
       * \brief turns off profiling for tensor
       */
      void profile_off();

      /**
       * \brief sets tensor name
       * \param[in] name new for tensor
       */
      void set_name(char const * name);

      /**
       * \brief sets all values in the tensor to val
       */
      Tensor& operator=(dtype val);
      
      /**
       * \brief sets the tensor
       */
      void operator=(Tensor<dtype> A);
      
      /**
       * \brief associated an index map with the tensor for future operation
       * \param[in] idx_map_ index assignment for this tensor
       */
      Idx_Tensor<dtype> operator[](char const * idx_map);
      
      /**
       * \brief gives handle to sparse index subset of tensors
       * \param[in] indices, vector of indices to sparse tensor
       */
      Sparse_Tensor<dtype> operator[](std::vector<int64_t> indices);
      
      /**
       * \brief prints tensor data to file using process 0
       * \param[in] fp file to print to e.g. stdout
       * \param[in] cutoff do not print values of absolute value smaller than this
       */
      void print(FILE * fp = stdout, double cutoff = -1.0) const;

      /**
       * \brief prints two sets of tensor data side-by-side to file using process 0
       * \param[in] fp file to print to e.g. stdout
       * \param[in] A tensor to compare against
       * \param[in] cutoff do not print values of absolute value smaller than this
       */
      void compare(const Tensor<dtype>& A, FILE * fp = stdout, double cutoff = -1.0) const;

      /**
       * \brief frees CTF tensor
       */
      ~Tensor();
  };
}

#include "tensor.cxx"
#include "matrix.h"
#include "vector.h"
#include "scalar.h"
#include "sparse_tensor.h"

/**
 * @}
 */

#endif
