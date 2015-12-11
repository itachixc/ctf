/** \addtogroup examples 
  * @{ 
  * \defgroup btwn_central betweenness centrality
  * @{ 
  * \brief betweenness centrality computation
  */

#include <ctf.hpp>
#include <float.h>
using namespace CTF;
struct path {
  int w; // weighted distance
  int h; // number of hops
  int m; // multiplictiy
  path(int w_, int h_, int m_){ w=w_; h=h_; m=m_; }
  path(path const & p){ w=p.w; h=p.h; m=p.m; }
  path(){};
};

Semiring<path> get_path_semiring(int n){

  //struct for path with w=path weight, h=#hops
  MPI_Op opath;

  MPI_Op_create(
      [](void * a, void * b, int * n, MPI_Datatype*){ 
        for (int i=0; i<*n; i++){ 
          if (((path*)a)[i].w <= ((path*)b)[i].w){
            ((path*)b)[0] = ((path*)a)[0];
          }
        }
      },
      1, &opath);

  //tropical semiring with hops carried by winner of min
  Semiring<path> p(path(INT_MAX/2,0,1), 
                   [](path a, path b){ 
                     if (a.w<b.w){ return a; }
                     else if (b.w<a.w){ return b; }
                     else { return path(a.w, std::min(a.h,b.h), a.m+b.m); }
                   },
                   opath,
                   path(0,0,1),
                   [](path a, path b){ return path(a.w+b.w, a.h+b.h, a.m*b.m); });

  return p;
}


struct cpath {
  int w; // weighted distance
  int m; // multiplictiy
  double c; // centrality score
  cpath(int w_, int m_, double c_){ w=w_; m=m_; c=c_;}
  cpath(cpath const & p){ w=p.w; m=p.m; c=p.c; }
  cpath(){};
};


Monoid<cpath> get_cpath_monoid(int n){

  //struct for cpath with w=cpath weight, h=#hops
  MPI_Op ocpath;

  MPI_Op_create(
      [](void * a, void * b, int * n, MPI_Datatype*){ 
        for (int i=0; i<*n; i++){ 
          if (((cpath*)a)[i].w <= ((cpath*)b)[i].w){
            ((cpath*)b)[0] = ((cpath*)a)[0];
          }
        }
      },
      1, &ocpath);

  Monoid<cpath> cp(cpath(-INT_MAX/2,1,0.), 
                  [](cpath a, cpath b){ 
                    if (a.w>b.w){ return a; }
                    else if (b.w>a.w){ return b; }
                    else { return cpath(a.w, a.m, a.c+b.c); }
                  }, ocpath);

  return cp;
}



namespace CTF {
  template <>  
  inline void Set<path>::print(char const * a, FILE * fp) const {
    fprintf(fp,"(w=%d h=%d m=%d)",((path*)a)[0].w,((path*)a)[0].h,((path*)a)[0].m);
  }
  template <>  
  inline void Set<cpath>::print(char const * a, FILE * fp) const {
    fprintf(fp,"(w=%d m=%d c=%lf)",((cpath*)a)[0].w,((cpath*)a)[0].m,((cpath*)a)[0].c);
  }
}

void btwn_cnt_fast(Matrix<int> A, int b, Vector<double> & v){
  World dw = *A.wrld;
  int n = A.nrow;
  ((Transform<int>)([=](int& w){ w = INT_MAX/2; }))(A["ii"]);
  Semiring<path> p = get_path_semiring(n);
  Monoid<cpath> cp = get_cpath_monoid(n);

  for (int ib=0; ib<n; ib+=b){
    int k = std::min(b, n-ib);
    Tensor<int> iA = A.slice(ib*n, (ib+k-1)*n+n-1);
    Matrix<path> B(n, k, dw, p, "B");
    B["ij"] = ((Function<int,path>)([](int i){ return path(i, 1, 1); }))(iA["ij"]);
    
    for (int i=0; i<n; i++){
      Matrix<path> B2(n, k, dw, p, "B2");
      B2["ij"] = ((Function<path,int,path>)([](path p, int i){ return path(p.w+i, p.h+1, p.m); }))(B["kj"],A["ik"]);
      ((Transform<path,path>)([](path a, path & b){ if (a.w < b.w || (a.w == b.w && a.m > b.m)) b=a; }))(B2["ij"], B["ij"]);
    }
    ((Transform<path>)([=](path& w){ w = path(-INT_MAX/2, 0, 1); }))(B["ii"]);
//    B.print();
    Matrix<cpath> cB(n, k, dw, cp, "cB");
    ((Transform<path,cpath>)([](path p, cpath & cp){ cp = cpath(p.w, p.m, 0.); }))(B["ij"],cB["ij"]);
  //  ((Transform<cpath>)([=](cpath& w){ w = cpath(-INT_MAX/2, 1, 0); }))(cB["ii"]);
    for (int i=0; i<n; i++){
      Matrix<cpath> cB2(n, k, dw, cp, "cB2");
      cB2["ij"] += cB["ij"];
      ((Transform<cpath>)([](cpath & p){ p.c=0.; }))(cB2["ij"]);
      //cB2["ij"] += ((Function<int,cpath,cpath>)([](int i, cpath p){ return cpath(p.w-i, p.m, (1.+p.c)/p.m); }))(A["ki"],cB["kj"]);
      int n3[]={n,n,n};
      Tensor<cpath> cB3(3,n3, dw, cp, "cB3");
      cB3["ijk"] += ((Function<int,cpath,cpath>)([](int i, cpath p){ return cpath(p.w-i, p.m, (1.+p.c)/p.m); }))(A["ki"],cB["kj"]);
      cB3.print();
      cB2["ij"] += cB3["ijk"];
      ((Transform<cpath,cpath>)([](cpath a, cpath & b){ b.c=a.c*b.m; }))(cB2["ij"], cB["ij"]);
      cB.print();
      A.print();
    ((Transform<cpath>)([](cpath & p){ p.c=0; }))(cB["ii"]);
//      ((Transform<path,cpath>)([](path p, cpath & cp){ cp.w = p.w; cp.m = p.m; }))(B["ij"],cB["ij"]);
    }
    assert(n==k);
    Matrix<> dcB(n,n,dw);
    dcB["ij"] = ((Function<cpath,double>)([](cpath p){ return p.c; }))(cB["ij"]);
    Tensor<double> scB = dcB.slice(0,n*(n-1));
    scB.print();
    v["i"] += ((Function<cpath,double>)([](cpath a){ return a.c; }))(cB["ij"]);
  }
}


void btwn_cnt_naive(Matrix<int> & A, Vector<double> & str_cnt){
  World dw = *A.wrld;
  int n = A.nrow;
 
  Semiring<path> p = get_path_semiring(n);
  Monoid<cpath> cp = get_cpath_monoid(n);
  //path matrix to contain distance matrix
  Matrix<path> P(n, n, dw, p, "P");

  Function<int,path> setw([](int w){ return path(w, 1, 1); });

  P["ij"] = setw(A["ij"]);
  
  ((Transform<path>)([=](path& w){ w = path(INT_MAX/2, 0, 1); }))(P["ii"]);

  //sparse path matrix to contain all paths of exactly i hops
  Matrix<path> Pi(n, n, dw, p);
  
  for (int i=1; i<n; i=i<<1){
    Pi["ij"] = P["ij"];
    Pi.sparsify([=](path p){ return (p.h == i); });
    Matrix<path> P2(n, n, dw, p, "P2");
    P2["ij"] = Pi["ik"]*P["kj"];
    ((Transform<path,path>)([](path a, path & b){ if (a.w < b.w || (a.w == b.w && a.m > b.m)) b=a; }))(P2["ij"], P["ij"]);
    //P.print();
  }
  ((Transform<path>)([=](path& p){ p = path(INT_MAX/2, 0, 1); }))(P["ii"]);
  ((Transform<path>)([=](path& p){ if (p.w >= INT_MAX/2) p = path(INT_MAX/2, 0, 1); }))(P["ij"]);
//  P.print();
 
  //Vector<int> str_cnt(n, dw, "stress centrality scores");

  int lenn[3] = {n,n,n};
  Tensor<double> ostv(3, lenn, dw, Ring<double>(), "ostv");
  Tensor<cpath> postv(3, lenn, dw, cp, "postv");

  postv["ijk"] += ((Function<path,cpath>)([](path p){ return cpath(p.w, p.m, 0.0); }))(P["ik"]);

  ((Transform<path,path,cpath>)(
    [=](path a, path b, cpath & c){ 
      if (c.w<INT_MAX/2 && a.w+b.w == c.w){ c.c = ((double)a.m*b.m)/c.m; } 
      else { c.c = 0; }
    }
  ))(P["ij"],P["jk"],postv["ijk"]);
  //ostv.print();
  ostv["ijk"] = ((Function<cpath,double>)([](cpath p){ return p.c; }))(postv["ijk"]);
  Tensor<double> sostv = ostv.slice(0,n*n*(n-1)+n-1);
  sostv.print();

  str_cnt["j"] += ostv["ijk"];

  //return str_cnt;
}

// calculate APSP on a graph of n nodes distributed on World (communicator) dw
int btwn_cnt(int     n,
             World & dw,
             int     niter=0){


  //tropical semiring, define additive identity to be INT_MAX/2 to prevent integer overflow
  Semiring<int> s(INT_MAX/2, 
                  [](int a, int b){ return std::min(a,b); },
                  MPI_MIN,
                  0,
                  [](int a, int b){ return a+b; });

  //random adjacency matrix
  Matrix<int> A(n, n, dw, s, "A");
/*  if (dw.rank == 0){
    int64_t inds[n-1];
    int vals[n-1];
    for (int i=0; i<n-1; i++){
      inds[i] = i*n+i+1;
      vals[i] = 1;
    }
    A.write(n-1, inds, vals);
  } else { 
    A.write(0, NULL, NULL);
  }
  if (dw.rank == 0){
    int64_t inds[n-1];
    int vals[n-1];
    for (int i=1; i<n; i++){
      inds[i-1] = i*n+i-1;
      vals[i-1] = 2;
    }
    A.write(n-1, inds, vals);
  } else { 
    A.write(0, NULL, NULL);
  }
  if (dw.rank == 0){
    int64_t inds[2] = {n-1, (n-1)*n};
    int vals[2] = {3, 3};
    A.write(2,inds,vals);
  } else A.write(0, NULL, NULL);*/
  srand(dw.rank+1);
//  A.fill_random(1, 4);//std::min(n*n,100)); 
  if (dw.rank == 0){
    int64_t inds[n*(n-1)/2];
    int vals[n*(n-1)/2];
    int c = 0;
    for (int i=0; i<n; i++){
      for (int j=0; j<i; j++){
        inds[c] = i*n+j;
        vals[c] = (rand()%4)+1;
        c++;
      }
    }
    A.write(n*(n-1)/2, inds, vals);
  } A.write(0, NULL, NULL);
  A["ii"] = 0;
//  A["ij"] += A["ji"];

  //A.sparsify([=](int a){ return a<50; });

  Vector<double> v1(n,dw);
  Vector<double> v2(n,dw);

  btwn_cnt_naive(A, v1);
 
  btwn_cnt_fast(A, n, v2);

  v1.print();
  v2.print();
//  v1.compare(v2);

  Scalar<> sc(dw);
  sc[""] = ((Function<>)([](double a, double b){ return std::abs(a-b); }))(v1["i"],v2["i"]);
 
  int pass = (sc.get_val() <= 0.01);
  if (dw.rank == 0){
    MPI_Reduce(MPI_IN_PLACE, &pass, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    if (pass) 
      printf("{ APSP by path doubling } passed \n");
    else
      printf("{ APSP by path doubling } failed \n");
  } else 
    MPI_Reduce(&pass, MPI_IN_PLACE, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
  return pass;
} 


#ifndef TEST_SUITE
char* getCmdOption(char ** begin,
                   char ** end,
                   const   std::string & option){
  char ** itr = std::find(begin, end, option);
  if (itr != end && ++itr != end){
    return *itr;
  }
  return 0;
}


int main(int argc, char ** argv){
  int rank, np, n, pass, niter;
  int const in_num = argc;
  char ** input_str = argv;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &np);

  if (getCmdOption(input_str, input_str+in_num, "-n")){
    n = atoi(getCmdOption(input_str, input_str+in_num, "-n"));
    if (n < 0) n = 7;
  } else n = 7;

  if (getCmdOption(input_str, input_str+in_num, "-niter")){
    niter = atof(getCmdOption(input_str, input_str+in_num, "-niter"));
    if (niter < 0) niter = 10;
  } else niter = 10;

  {
    World dw(argc, argv);

    if (rank == 0){
      printf("Computing betweenness centrality of dense graph with %d nodes using dense and sparse path doubling\n",n);
    }
    pass = btwn_cnt(n, dw, niter);
    assert(pass);
  }

  MPI_Finalize();
  return 0;
}
/**
 * @} 
 * @}
 */

#endif
