/*Copyright (c) 2011, Edgar Solomonik, all rights reserved.*/

#include "../shared/util.h"
#include "sum_tsr.h"

/**
 * \brief copies generic tsum object
 */
template<typename dtype>
tsum<dtype>::tsum(tsum<dtype> * other){
  A = other->A;
  alpha = other->alpha;
  B = other->B;
  beta = other->beta;
  buffer = NULL;
}

/**
 * \brief deallocates tsum_virt object
 */
template<typename dtype>
tsum_virt<dtype>::~tsum_virt() {
  CTF_free(virt_dim);
  delete rec_tsum;
}

/**
 * \brief copies tsum object
 */
template<typename dtype>
tsum_virt<dtype>::tsum_virt(tsum<dtype> * other) : tsum<dtype>(other) {
  tsum_virt * o         = (tsum_virt*)other;
  rec_tsum      = o->rec_tsum->clone();
  num_dim       = o->num_dim;
  virt_dim      = (int*)CTF_alloc(sizeof(int)*num_dim);
  memcpy(virt_dim, o->virt_dim, sizeof(int)*num_dim);

  ndim_A        = o->ndim_A;
  blk_sz_A      = o->blk_sz_A;
  idx_map_A     = o->idx_map_A;

  ndim_B        = o->ndim_B;
  blk_sz_B      = o->blk_sz_B;
  idx_map_B     = o->idx_map_B;
}

/**
 * \brief copies tsum object
 */
template<typename dtype>
tsum<dtype> * tsum_virt<dtype>::clone() {
  return new tsum_virt(this);
}


/**
 * \brief returns the number of bytes of buffer space
   we need
 * \return bytes needed
 */
template<typename dtype>
int64_t tsum_virt<dtype>::mem_fp(){
  return (ndim_A+ndim_B+3*num_dim)*sizeof(int);
}

/**
 * \brief iterates over the dense virtualization block grid and contracts
 */
template<typename dtype>
void tsum_virt<dtype>::run(){
  int * idx_arr, * lda_A, * lda_B, * beta_arr;
  int * ilda_A, * ilda_B;
  int64_t i, off_A, off_B;
  int nb_A, nb_B, alloced, ret; 
  TAU_FSTART(sum_virt);

  if (this->buffer != NULL){    
    alloced = 0;
    idx_arr = (int*)this->buffer;
  } else {
    alloced = 1;
    ret = CTF_alloc_ptr(mem_fp(), (void**)&idx_arr);
    LIBT_ASSERT(ret==0);
  }
  
  lda_A = idx_arr + num_dim;
  lda_B = lda_A + ndim_A;
  ilda_A = lda_B + ndim_B;
  ilda_B = ilda_A + num_dim;
  

#define SET_LDA_X(__X)                                                  \
do {                                                                    \
  nb_##__X = 1;                                                         \
  for (i=0; i<ndim_##__X; i++){                                 \
    lda_##__X[i] = nb_##__X;                                            \
    nb_##__X = nb_##__X*virt_dim[idx_map_##__X[i]];     \
  }                                                                     \
  memset(ilda_##__X, 0, num_dim*sizeof(int));                   \
  for (i=0; i<ndim_##__X; i++){                                 \
    ilda_##__X[idx_map_##__X[i]] += lda_##__X[i];                       \
  }                                                                     \
} while (0)
  SET_LDA_X(A);
  SET_LDA_X(B);
#undef SET_LDA_X
  
  /* dynammically determined size */ 
  beta_arr = (int*)CTF_alloc(sizeof(int)*nb_B);
 
  memset(idx_arr, 0, num_dim*sizeof(int));
  memset(beta_arr, 0, nb_B*sizeof(int));
  off_A = 0, off_B = 0;
  rec_tsum->alpha = this->alpha;
  rec_tsum->beta = this->beta;
  for (;;){
    rec_tsum->A = this->A + off_A*blk_sz_A;
    rec_tsum->B = this->B + off_B*blk_sz_B;
    rec_tsum->beta      = beta_arr[off_B]>0 ? 1.0 : this->beta;
    beta_arr[off_B]     = 1;
    rec_tsum->run();

    for (i=0; i<num_dim; i++){
      off_A -= ilda_A[i]*idx_arr[i];
      off_B -= ilda_B[i]*idx_arr[i];
      idx_arr[i]++;
      if (idx_arr[i] >= virt_dim[i])
        idx_arr[i] = 0;
      off_A += ilda_A[i]*idx_arr[i];
      off_B += ilda_B[i]*idx_arr[i];
      if (idx_arr[i] != 0) break;
    }
    if (i==num_dim) break;
  }
  if (alloced){
    CTF_free(idx_arr);
  }
  CTF_free(beta_arr);
  TAU_FSTOP(sum_virt);
}



/**
 * \brief deallocates tsum_replicate object
 */
template<typename dtype>
tsum_replicate<dtype>::~tsum_replicate() {
  delete rec_tsum;
  for (int i=0; i<ncdt_A; i++){
    FREE_CDT(cdt_A[i]);
  }
  if (ncdt_A > 0)
    CTF_free(cdt_A);
  for (int i=0; i<ncdt_B; i++){
    FREE_CDT(cdt_B[i]);
  }
  if (ncdt_B > 0)
    CTF_free(cdt_B);
}

/**
 * \brief copies tsum object
 */
template<typename dtype>
tsum_replicate<dtype>::tsum_replicate(tsum<dtype> * other) : tsum<dtype>(other) {
  tsum_replicate<dtype> * o = (tsum_replicate<dtype>*)other;
  rec_tsum = o->rec_tsum->clone();
  size_A = o->size_A;
  size_B = o->size_B;
  ncdt_A = o->ncdt_A;
  ncdt_B = o->ncdt_B;
}

/**
 * \brief copies tsum object
 */
template<typename dtype>
tsum<dtype> * tsum_replicate<dtype>::clone() {
  return new tsum_replicate<dtype>(this);
}

/**
 * \brief returns the number of bytes of buffer space
   we need 
 * \return bytes needed
 */
template<typename dtype>
int64_t tsum_replicate<dtype>::mem_fp(){
  return 0;
}


/**
 * \brief performs replication along a dimension, generates 2.5D algs
 */
template<typename dtype>
void tsum_replicate<dtype>::run(){
  int brank, i;

  for (i=0; i<ncdt_A; i++){
    POST_BCAST(this->A, size_A*sizeof(dtype), COMM_CHAR_T, 0, cdt_A[i], 0);
  }
 /* for (i=0; i<ncdt_B; i++){
    POST_BCAST(this->B, size_B*sizeof(dtype), COMM_CHAR_T, 0, cdt_B[i], 0);
  }*/
  brank = 0;
  for (i=0; i<ncdt_B; i++){
    brank += cdt_B[i].rank;
  }
  if (brank != 0) std::fill(this->B, this->B+size_B, 0.0);

  rec_tsum->A           = this->A;
  rec_tsum->B           = this->B;
  rec_tsum->alpha       = this->alpha;
  rec_tsum->beta        = brank != 0 ? 0.0 : this->beta;

  rec_tsum->run();
  
  for (i=0; i<ncdt_B; i++){
    /* FIXME Won't work for single precision */
    ALLREDUCE(MPI_IN_PLACE, this->B, size_B*(sizeof(dtype)/sizeof(double)), MPI_DOUBLE, MPI_SUM, cdt_B[i]);
  }


}

template class tsum<double>;
template class tsum< std::complex<double> >;
template class tsum_virt<double>;
template class tsum_virt< std::complex<double> >;
template class tsum_replicate<double>;
template class tsum_replicate< std::complex<double> >;
