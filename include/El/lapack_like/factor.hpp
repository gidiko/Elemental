/*
   Copyright (c) 2009-2016, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License,
   which can be found in the LICENSE file in the root directory, or at
   http://opensource.org/licenses/BSD-2-Clause
*/
#ifndef EL_FACTOR_HPP
#define EL_FACTOR_HPP

#include <El/lapack_like/perm.hpp>
#include <El/lapack_like/util.hpp>
#include <El/lapack_like/factor/ldl/sparse/symbolic.hpp>
#include <El/lapack_like/factor/ldl/sparse/numeric.hpp>

namespace El {

// Cholesky
// ========
template<typename F>
void Cholesky( UpperOrLower uplo, Matrix<F>& A );
template<typename F>
void Cholesky
( UpperOrLower uplo, AbstractDistMatrix<F>& A, bool scalapack=false );
template<typename F>
void Cholesky( UpperOrLower uplo, DistMatrix<F,STAR,STAR>& A );

template<typename F>
void ReverseCholesky( UpperOrLower uplo, Matrix<F>& A );
template<typename F>
void ReverseCholesky( UpperOrLower uplo, AbstractDistMatrix<F>& A );
template<typename F>
void ReverseCholesky( UpperOrLower uplo, DistMatrix<F,STAR,STAR>& A );

template<typename F>
void Cholesky( UpperOrLower uplo, Matrix<F>& A, Permutation& P );
template<typename F>
void Cholesky
( UpperOrLower uplo, AbstractDistMatrix<F>& A, DistPermutation& P );

template<typename F>
void CholeskyMod
( UpperOrLower uplo,
  Matrix<F>& T,
  Base<F> alpha,
  Matrix<F>& V );
template<typename F>
void CholeskyMod
( UpperOrLower uplo,
  AbstractDistMatrix<F>& T,
  Base<F> alpha,
  AbstractDistMatrix<F>& V );

template<typename F>
void HPSDCholesky( UpperOrLower uplo, Matrix<F>& A );
template<typename F>
void HPSDCholesky( UpperOrLower uplo, AbstractDistMatrix<F>& A );

namespace cholesky {

template<typename F>
void SolveAfter
( UpperOrLower uplo,
  Orientation orientation,
  const Matrix<F>& A,
        Matrix<F>& B );
template<typename F>
void SolveAfter
( UpperOrLower uplo,
  Orientation orientation,
  const AbstractDistMatrix<F>& A,
        AbstractDistMatrix<F>& B );

template<typename F>
void SolveAfter
( UpperOrLower uplo,
  Orientation orientation,
  const Matrix<F>& A,
  const Permutation& P,
        Matrix<F>& B );
template<typename F>
void SolveAfter
( UpperOrLower uplo,
  Orientation orientation,
  const AbstractDistMatrix<F>& A,
  const DistPermutation& P,
        AbstractDistMatrix<F>& B );

} // namespace cholesky

// LDL
// ===
namespace LDLPivotTypeNS {
enum LDLPivotType
{
    BUNCH_KAUFMAN_A,
    BUNCH_KAUFMAN_C,
    BUNCH_KAUFMAN_D,
    BUNCH_KAUFMAN_BOUNDED,
    BUNCH_PARLETT,
    LDL_WITHOUT_PIVOTING
    /* TODO: Diagonal pivoting? */
};
}
using namespace LDLPivotTypeNS;

template<typename Real>
Real LDLPivotConstant( LDLPivotType pivType )
{
    // TODO: Check that the Bunch-Parlett choice is optimal
    switch( pivType )
    {
    case BUNCH_KAUFMAN_A:
    case BUNCH_PARLETT:   return (1+Sqrt(Real(17)))/8;
    case BUNCH_KAUFMAN_D: return Real(0.525);
    default:
        LogicError("No default constant exists for this pivot type");
        return 0;
    }
}

struct LDLPivot
{
    Int nb;
    Int from[2];
};

template<typename Real>
struct LDLPivotCtrl {
  LDLPivotType pivotType;
  Real gamma;

  LDLPivotCtrl( LDLPivotType piv=BUNCH_KAUFMAN_A )
  : pivotType(piv), gamma(LDLPivotConstant<Real>(piv)) { }
};

// Return the L (and D) from an LDL factorization of A (without pivoting)
// ----------------------------------------------------------------------
template<typename F>
void LDL( Matrix<F>& A, bool conjugate );
template<typename F>
void LDL( AbstractDistMatrix<F>& A, bool conjugate );
template<typename F>
void LDL( DistMatrix<F,STAR,STAR>& A, bool conjugate );

// Return an implicit representation of a pivoted LDL factorization of A
// ---------------------------------------------------------------------
template<typename F>
void LDL
( Matrix<F>& A,
  Matrix<F>& dSub,
  Permutation& P,
  bool conjugate,
  const LDLPivotCtrl<Base<F>>& ctrl=LDLPivotCtrl<Base<F>>() );
template<typename F>
void LDL
( AbstractDistMatrix<F>& A,
  AbstractDistMatrix<F>& dSub,
  DistPermutation& P,
  bool conjugate,
  const LDLPivotCtrl<Base<F>>& ctrl=LDLPivotCtrl<Base<F>>() );

// All fronts of L are required to be initialized to the expansions of the
// original sparse matrix before calling LDL.
template<typename F>
void LDL
( const ldl::NodeInfo& info,
        ldl::Front<F>& L,
  LDLFrontType newType=LDL_2D );
template<typename F>
void LDL
( const ldl::DistNodeInfo& info,
        ldl::DistFront<F>& L,
  LDLFrontType newType=LDL_2D );

namespace ldl {

// Compute the inertia triplet of a Hermitian matrix's LDL^H factorization
// -----------------------------------------------------------------------
template<typename F>
InertiaType Inertia
( const Matrix<Base<F>>& d,
  const Matrix<F>& dSub );
template<typename F>
InertiaType Inertia
( const AbstractDistMatrix<Base<F>>& d,
  const AbstractDistMatrix<F>& dSub );

// Multiply vectors using an implicit representation of an LDL factorization
// -------------------------------------------------------------------------
template<typename F>
void MultiplyAfter
( const Matrix<F>& A,
        Matrix<F>& B,
  bool conjugated );
template<typename F>
void MultiplyAfter
( const AbstractDistMatrix<F>& A,
        AbstractDistMatrix<F>& B,
  bool conjugated );

// Multiply vectors using an implicit representation of a pivoted LDL fact.
// ------------------------------------------------------------------------
template<typename F>
void MultiplyAfter
( const Matrix<F>& A,
  const Matrix<F>& dSub,
  const Permutation& P,
        Matrix<F>& B,
  bool conjugated );
template<typename F>
void MultiplyAfter
( const AbstractDistMatrix<F>& A,
  const AbstractDistMatrix<F>& dSub,
  const DistPermutation& P,
        AbstractDistMatrix<F>& B,
  bool conjugated );

// Solve linear systems using an implicit LDL factorization
// --------------------------------------------------------
template<typename F>
void SolveAfter
( const Matrix<F>& A,
        Matrix<F>& B,
  bool conjugated );
template<typename F>
void SolveAfter
( const AbstractDistMatrix<F>& A,
        AbstractDistMatrix<F>& B,
  bool conjugated );

template<typename F>
void SolveAfter
( const vector<Int>& invMap, const NodeInfo& info,
  const Front<F>& front, Matrix<F>& X );
template<typename F>
void SolveAfter
( const DistMap& invMap, const DistNodeInfo& info,
  const DistFront<F>& front, DistMultiVec<F>& X );

template<typename F>
void SolveAfter
( const NodeInfo& info,
  const Front<F>& front, MatrixNode<F>& X );
template<typename F>
void SolveAfter
( const DistNodeInfo& info,
  const DistFront<F>& front, DistMultiVecNode<F>& X );
template<typename F>
void SolveAfter
( const DistNodeInfo& info,
  const DistFront<F>& front, DistMatrixNode<F>& X );

template<typename F>
Int SolveWithIterativeRefinement
( const SparseMatrix<F>& A,
  const vector<Int>& invMap, const NodeInfo& info,
  const Front<F>& front, Matrix<F>& y,
  Base<F> relTolRefine,      Int maxRefineIts );
template<typename F>
Int SolveWithIterativeRefinement
( const DistSparseMatrix<F>& A,
  const DistMap& invMap, const DistNodeInfo& info,
  const DistFront<F>& front, DistMultiVec<F>& y,
  Base<F> relTolRefine, Int maxRefineIts );

// Solve linear system with the implicit representations of L, D, and P
// --------------------------------------------------------------------
template<typename F>
void SolveAfter
( const Matrix<F>& A,
  const Matrix<F>& dSub,
  const Permutation& P,
        Matrix<F>& B,
  bool conjugated );
template<typename F>
void SolveAfter
( const AbstractDistMatrix<F>& A,
  const AbstractDistMatrix<F>& dSub,
  const DistPermutation& P,
        AbstractDistMatrix<F>& B,
  bool conjugated );

} // namespace ldl

// Solve a linear system with a regularized factorization
// ======================================================
enum RegSolveAlg
{
  REG_SOLVE_FGMRES,
  REG_SOLVE_LGMRES
};

template<typename Real>
struct RegSolveCtrl
{
    RegSolveAlg alg=REG_SOLVE_FGMRES;
    Real relTol;
    Real relTolRefine;
    Int maxIts=4;
    Int maxRefineIts=2;
    Int restart=4;
    bool progress=false;
    bool time=false;

    RegSolveCtrl()
    {
        const Real eps = limits::Epsilon<Real>();
        relTol = Pow(eps,Real(0.5));
        relTolRefine = Pow(eps,Real(0.8));
    }
};

namespace reg_ldl {

template<typename F>
Int RegularizedSolveAfter
( const SparseMatrix<F>& A,
  const Matrix<Base<F>>& reg,
  const vector<Int>& invMap,
  const ldl::NodeInfo& info,
  const ldl::Front<F>& front,
        Matrix<F>& B,
        Base<F> relTolRefine,
        Int maxRefineIts,
        bool progress=false,
        bool time=false );
template<typename F>
Int RegularizedSolveAfter
( const DistSparseMatrix<F>& A,
  const DistMultiVec<Base<F>>& reg,
  const DistMap& invMap,
  const ldl::DistNodeInfo& info,
  const ldl::DistFront<F>& front,
        DistMultiVec<F>& B,
        Base<F> relTolRefine,
        Int maxRefineIts,
        bool progress=false,
        bool time=false );
template<typename F>
Int RegularizedSolveAfter
( const DistSparseMatrix<F>& A,
  const DistMultiVec<Base<F>>& reg,
  const DistMap& invMap,
  const ldl::DistNodeInfo& info,
  const ldl::DistFront<F>& front,
        DistMultiVec<F>& B,
        ldl::DistMultiVecNodeMeta& meta,
        Base<F> relTolRefine,
        Int maxRefineIts,
        bool progress=false,
        bool time=false );

template<typename F>
Int RegularizedSolveAfter
( const SparseMatrix<F>& A,
  const Matrix<Base<F>>& reg,
  const Matrix<Base<F>>& d,
  const vector<Int>& invMap,
  const ldl::NodeInfo& info,
  const ldl::Front<F>& front,
        Matrix<F>& B,
        Base<F> relTolRefine,
        Int maxRefineIts,
        bool progress=false,
        bool time=false );
template<typename F>
Int RegularizedSolveAfter
( const DistSparseMatrix<F>& A,
  const DistMultiVec<Base<F>>& reg,
  const DistMultiVec<Base<F>>& d,
  const DistMap& invMap,
  const ldl::DistNodeInfo& info,
  const ldl::DistFront<F>& front,
        DistMultiVec<F>& B,
        Base<F> relTolRefine,
        Int maxRefineIts,
        bool progress=false,
        bool time=false );
template<typename F>
Int RegularizedSolveAfter
( const DistSparseMatrix<F>& A,
  const DistMultiVec<Base<F>>& reg,
  const DistMultiVec<Base<F>>& d,
  const DistMap& invMap,
  const ldl::DistNodeInfo& info,
  const ldl::DistFront<F>& front,
        DistMultiVec<F>& B,
        ldl::DistMultiVecNodeMeta& meta,
        Base<F> relTolRefine,
        Int maxRefineIts,
        bool progress=false,
        bool time=false );

template<typename F>
Int SolveAfter
( const SparseMatrix<F>& A,
  const Matrix<Base<F>>& reg,
  const vector<Int>& invMap,
  const ldl::NodeInfo& info,
  const ldl::Front<F>& front,
        Matrix<F>& B,
  const RegSolveCtrl<Base<F>>& ctrl );
template<typename F>
Int SolveAfter
( const DistSparseMatrix<F>& A,
  const DistMultiVec<Base<F>>& reg,
  const DistMap& invMap,
  const ldl::DistNodeInfo& info,
  const ldl::DistFront<F>& front,
        DistMultiVec<F>& B,
  const RegSolveCtrl<Base<F>>& ctrl );
template<typename F>
Int SolveAfter
( const DistSparseMatrix<F>& A,
  const DistMultiVec<Base<F>>& reg,
  const DistMap& invMap,
  const ldl::DistNodeInfo& info,
  const ldl::DistFront<F>& front,
        DistMultiVec<F>& B,
        ldl::DistMultiVecNodeMeta& meta,
  const RegSolveCtrl<Base<F>>& ctrl );

template<typename F>
Int SolveAfter
( const SparseMatrix<F>& A,
  const Matrix<Base<F>>& reg,
  const Matrix<Base<F>>& d,
  const vector<Int>& invMap,
  const ldl::NodeInfo& info,
  const ldl::Front<F>& front,
        Matrix<F>& B,
  const RegSolveCtrl<Base<F>>& ctrl );
template<typename F>
Int SolveAfter
( const DistSparseMatrix<F>& A,
  const DistMultiVec<Base<F>>& reg,
  const DistMultiVec<Base<F>>& d,
  const DistMap& invMap,
  const ldl::DistNodeInfo& info,
  const ldl::DistFront<F>& front,
        DistMultiVec<F>& B,
  const RegSolveCtrl<Base<F>>& ctrl );
template<typename F>
Int SolveAfter
( const DistSparseMatrix<F>& A,
  const DistMultiVec<Base<F>>& reg,
  const DistMultiVec<Base<F>>& d,
  const DistMap& invMap,
  const ldl::DistNodeInfo& info,
  const ldl::DistFront<F>& front,
        DistMultiVec<F>& B,
        ldl::DistMultiVecNodeMeta& meta,
  const RegSolveCtrl<Base<F>>& ctrl );

} // namespace reg_ldl

// LU
// ==

// NOTE: This is not yet made use of, but the fully-pivoted version of LU
//       should (soon?) accept it as an argument and potentially return one or
//       more of the permutation matrices as the identity
namespace LUPivotTypeNS {
enum LUPivotType
{
    LU_PARTIAL,
    LU_FULL,
    LU_ROOK, /* not yet supported */
    LU_WITHOUT_PIVOTING
};
}
using namespace LUPivotTypeNS;

// LU without pivoting
// -------------------
template<typename F>
void LU( Matrix<F>& A );
template<typename F>
void LU( AbstractDistMatrix<F>& A );
template<typename F>
void LU( DistMatrix<F,STAR,STAR>& A );

// LU with partial pivoting
// ------------------------
template<typename F>
void LU( Matrix<F>& A, Permutation& P );
template<typename F>
void LU( AbstractDistMatrix<F>& A, DistPermutation& P );

// LU with full pivoting
// ---------------------
// P A Q^T = L U
template<typename F>
void LU
( Matrix<F>& A,
  Permutation& P,
  Permutation& Q );
template<typename F>
void LU
( AbstractDistMatrix<F>& A,
  DistPermutation& P,
  DistPermutation& Q );

// Rank-one modification of a partially-pivoted LU factorization
// -------------------------------------------------------------
template<typename F>
void LUMod
(       Matrix<F>& A,
        Permutation& P,
  const Matrix<F>& u,
  const Matrix<F>& v,
  bool conjugate=true,
  Base<F> tau=Base<F>(1)/Base<F>(10) );
template<typename F>
void LUMod
(       AbstractDistMatrix<F>& A,
        DistPermutation& P,
  const AbstractDistMatrix<F>& u,
  const AbstractDistMatrix<F>& v,
  bool conjugate=true,
  Base<F> tau=Base<F>(1)/Base<F>(10) );

namespace lu {

// Solve linear systems using an implicit unpivoted LU factorization
// -----------------------------------------------------------------
template<typename F>
void SolveAfter
( Orientation orientation,
  const Matrix<F>& A,
        Matrix<F>& B );
template<typename F>
void SolveAfter
( Orientation orientation,
  const AbstractDistMatrix<F>& A,
        AbstractDistMatrix<F>& B );

// Solve linear systems using an implicit partially-pivoted LU factorization
// -------------------------------------------------------------------------
template<typename F>
void SolveAfter
( Orientation orientation,
  const Matrix<F>& A,
  const Permutation& P,
        Matrix<F>& B );
template<typename F>
void SolveAfter
( Orientation orientation,
  const AbstractDistMatrix<F>& A,
  const DistPermutation& P,
        AbstractDistMatrix<F>& B );

// Solve linear systems using an implicit fully-pivoted LU factorization
// ---------------------------------------------------------------------
template<typename F>
void SolveAfter
( Orientation orientation,
  const Matrix<F>& A,
  const Permutation& P,
  const Permutation& Q,
        Matrix<F>& B );
template<typename F>
void SolveAfter
( Orientation orientation,
  const AbstractDistMatrix<F>& A,
  const DistPermutation& P,
  const DistPermutation& Q,
        AbstractDistMatrix<F>& B );

} // namespace lu

// LQ
// ==

// Overwrite A with both L and the scaled Householder vectors
// ----------------------------------------------------------
template<typename F>
void LQ
( Matrix<F>& A,
  Matrix<F>& householderScalars,
  Matrix<Base<F>>& signature );
template<typename F>
void LQ
( AbstractDistMatrix<F>& A,
  AbstractDistMatrix<F>& householderScalars,
  AbstractDistMatrix<Base<F>>& signature );

namespace lq {

// Apply Q using its implicit representation
// -----------------------------------------
template<typename F>
void ApplyQ
( LeftOrRight side, Orientation orientation,
  const Matrix<F>& A,
  const Matrix<F>& householderScalars,
  const Matrix<Base<F>>& signature,
        Matrix<F>& B );
template<typename F>
void ApplyQ
( LeftOrRight side, Orientation orientation,
  const AbstractDistMatrix<F>& A,
  const AbstractDistMatrix<F>& householderScalars,
  const AbstractDistMatrix<Base<F>>& signature,
        AbstractDistMatrix<F>& B );

// Solve a linear system with the implicit representations of L and Q
// ------------------------------------------------------------------
template<typename F>
void SolveAfter
( Orientation orientation,
  const Matrix<F>& A,
  const Matrix<F>& householderScalars,
  const Matrix<Base<F>>& signature,
  const Matrix<F>& B,
        Matrix<F>& X );
template<typename F>
void SolveAfter
( Orientation orientation,
  const AbstractDistMatrix<F>& A,
  const AbstractDistMatrix<F>& householderScalars,
  const AbstractDistMatrix<Base<F>>& signature,
  const AbstractDistMatrix<F>& B,
        AbstractDistMatrix<F>& X );

// Overwrite A with L
// ------------------
template<typename F>
void ExplicitTriang( Matrix<F>& A );
template<typename F>
void ExplicitTriang( AbstractDistMatrix<F>& A );

// Overwrite A with Q
// ------------------
template<typename F>
void ExplicitUnitary( Matrix<F>& A );
template<typename F>
void ExplicitUnitary( AbstractDistMatrix<F>& A );

// Return both L and Q such that A = L Q
// -------------------------------------
template<typename F>
void Explicit( Matrix<F>& L, Matrix<F>& A );
template<typename F>
void Explicit( AbstractDistMatrix<F>& L, AbstractDistMatrix<F>& A );

} // namespace lq

// QR factorization
// ================

template<typename Real>
struct QRCtrl
{
    bool colPiv=false;

    bool boundRank=false;
    Int maxRank=0;

    bool adaptive=false;
    Real tol=Real(0);

    bool alwaysRecomputeNorms=false;

    // Selecting for the smallest norm first is an important preprocessing
    // step for LLL suggested by Wubben et al.
    //
    // Ideally a black-box reduction operation could be provided by the user
    // instead, as it is often the case that one may desire a custom pivoting
    // rule.
    bool smallestFirst=false;
};

// Return an implicit representation of Q and R such that A = Q R
// --------------------------------------------------------------
template<typename F>
void QR
( Matrix<F>& A,
  Matrix<F>& householderScalars,
  Matrix<Base<F>>& signature );
template<typename F>
void QR
( AbstractDistMatrix<F>& A,
  AbstractDistMatrix<F>& householderScalars,
  AbstractDistMatrix<Base<F>>& signature );

// Return an implicit representation of (Q,R,Omega) such that A Omega^T ~= Q R
// ---------------------------------------------------------------------------
template<typename F>
void QR
( Matrix<F>& A,
  Matrix<F>& householderScalars,
  Matrix<Base<F>>& signature,
  Permutation& Omega,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );
template<typename F>
void QR
( AbstractDistMatrix<F>& A,
  AbstractDistMatrix<F>& householderScalars,
  AbstractDistMatrix<Base<F>>& signature,
  DistPermutation& Omega,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );

namespace qr {

// Apply Q using its implicit representation
// -----------------------------------------
template<typename F>
void ApplyQ
( LeftOrRight side,
  Orientation orientation,
  const Matrix<F>& A,
  const Matrix<F>& householderScalars,
  const Matrix<Base<F>>& signature,
        Matrix<F>& B );
template<typename F>
void ApplyQ
( LeftOrRight side,
  Orientation orientation,
  const AbstractDistMatrix<F>& A,
  const AbstractDistMatrix<F>& householderScalars,
  const AbstractDistMatrix<Base<F>>& signature,
        AbstractDistMatrix<F>& B );

// Solve a linear system with the implicit QR factorization
// --------------------------------------------------------
template<typename F>
void SolveAfter
( Orientation orientation,
  const Matrix<F>& A,
  const Matrix<F>& householderScalars,
  const Matrix<Base<F>>& signature,
  const Matrix<F>& B,
        Matrix<F>& X );
template<typename F>
void SolveAfter
( Orientation orientation,
  const AbstractDistMatrix<F>& A,
  const AbstractDistMatrix<F>& householderScalars,
  const AbstractDistMatrix<Base<F>>& signature,
  const AbstractDistMatrix<F>& B,
        AbstractDistMatrix<F>& X );
// TODO: Version which involves permutation matrix

// Cholesky-based QR
// -----------------
template<typename F>
void Cholesky( Matrix<F>& A, Matrix<F>& R );
template<typename F>
void Cholesky( AbstractDistMatrix<F>& A, AbstractDistMatrix<F>& R );

// Return R (with non-negative diagonal) such that A = Q R or A Omega^T = Q R
// --------------------------------------------------------------------------
template<typename F>
void ExplicitTriang
( Matrix<F>& A,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );
template<typename F>
void ExplicitTriang
( AbstractDistMatrix<F>& A,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );

// Return Q such that either A = Q R or A Omega^T = Q R
// ----------------------------------------------------
template<typename F>
void ExplicitUnitary
( Matrix<F>& A,
  bool thinQ=true,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );
template<typename F>
void ExplicitUnitary
( AbstractDistMatrix<F>& A,
  bool thinQ=true,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );

// Return both Q and R such that A = Q R or A Omega^T = Q R
// --------------------------------------------------------
template<typename F>
void Explicit
( Matrix<F>& A,
  Matrix<F>& R,
  bool thinQ=true,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );
template<typename F>
void Explicit
( AbstractDistMatrix<F>& A,
  AbstractDistMatrix<F>& R,
  bool thinQ=true,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );

// Return (Q,R,Omega) such that A Omega^T = Q R
// --------------------------------------------
// NOTE: Column-pivoting is performed regardless of the value of ctrl.colPiv
template<typename F>
void Explicit
( Matrix<F>& A,
  Matrix<F>& R,
  Matrix<Int>& Omega,
  bool thinQ=true,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );
template<typename F>
void Explicit
( AbstractDistMatrix<F>& A,
  AbstractDistMatrix<F>& R,
  AbstractDistMatrix<Int>& Omega,
  bool thinQ=true,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );

// Swap neighboring columns (j,j+1) and update the QR factorization
// ----------------------------------------------------------------
template<typename F>
void NeighborColSwap
( Matrix<F>& Q,
  Matrix<F>& R,
  Int j );

// Swap disjoint sets of neighboring columns and update the QR factorization
// -------------------------------------------------------------------------
template<typename F>
void DisjointNeighborColSwaps
(       Matrix<F>& Q,
        Matrix<F>& R,
  const Matrix<Int>& colSwaps );

template<typename F>
struct TreeData
{
    Matrix<F> QR0, householderScalars0;
    Matrix<Base<F>> signature0;
    vector<Matrix<F>> QRList;
    vector<Matrix<F>> householderScalarsList;
    vector<Matrix<Base<F>>> signatureList;

    TreeData( Int numStages=0 )
    : QRList(numStages),
      householderScalarsList(numStages),
      signatureList(numStages)
    { }

    TreeData( TreeData<F>&& treeData )
    : QR0(move(treeData.QR0)),
      householderScalars0(move(treeData.householderScalars0)),
      signature0(move(treeData.signature0)),
      QRList(move(treeData.QRList)),
      householderScalarsList(move(treeData.householderScalarsList)),
      signatureList(move(treeData.signatureList))
    { }

    TreeData<F>& operator=( TreeData<F>&& treeData )
    {
        QR0 = move(treeData.QR0);
        householderScalars0 = move(treeData.householderScalars0);
        signature0 = move(treeData.signature0);
        QRList = move(treeData.QRList);
        householderScalarsList = move(treeData.householderScalarsList);
        signatureList = move(treeData.signatureList);
        return *this;
    }
};

// Return an implicit tall-skinny QR factorization
template<typename F>
TreeData<F> TS( const AbstractDistMatrix<F>& A );

// Return an explicit tall-skinny QR factorization
template<typename F>
void ExplicitTS( AbstractDistMatrix<F>& A, AbstractDistMatrix<F>& R );

namespace ts {

template<typename F>
Matrix<F>& RootQR
( const AbstractDistMatrix<F>& A, TreeData<F>& treeData );

template<typename F>
const Matrix<F>& RootQR
( const AbstractDistMatrix<F>& A, const TreeData<F>& treeData );

template<typename F>
void Reduce( const AbstractDistMatrix<F>& A, TreeData<F>& treeData );

template<typename F>
void Scatter( AbstractDistMatrix<F>& A, const TreeData<F>& treeData );

} // namespace ts

} // namespace qr

// RQ
// ==
template<typename F>
void RQ
( Matrix<F>& A,
  Matrix<F>& householderScalars,
  Matrix<Base<F>>& signature );
template<typename F>
void RQ
( AbstractDistMatrix<F>& A,
  AbstractDistMatrix<F>& householderScalars,
  AbstractDistMatrix<Base<F>>& signature );

namespace rq {

template<typename F>
void ApplyQ
( LeftOrRight side,
  Orientation orientation,
  const Matrix<F>& A,
  const Matrix<F>& householderScalars,
  const Matrix<Base<F>>& signature,
        Matrix<F>& B );
template<typename F>
void ApplyQ
( LeftOrRight side,
  Orientation orientation,
  const AbstractDistMatrix<F>& A,
  const AbstractDistMatrix<F>& householderScalars,
  const AbstractDistMatrix<Base<F>>& signature,
        AbstractDistMatrix<F>& B );

template<typename F>
void SolveAfter
( Orientation orientation,
  const Matrix<F>& A,
  const Matrix<F>& householderScalars,
  const Matrix<Base<F>>& signature,
  const Matrix<F>& B,
        Matrix<F>& X );
template<typename F>
void SolveAfter
( Orientation orientation,
  const AbstractDistMatrix<F>& A,
  const AbstractDistMatrix<F>& householderScalars,
  const AbstractDistMatrix<Base<F>>& signature,
  const AbstractDistMatrix<F>& B,
        AbstractDistMatrix<F>& X );

// TODO: Think about ensuring this ordering is consistent with lq::Explicit
template<typename F>
void Cholesky( Matrix<F>& A, Matrix<F>& R );
template<typename F>
void Cholesky( AbstractDistMatrix<F>& A, AbstractDistMatrix<F>& R );

template<typename F>
void ExplicitTriang( Matrix<F>& A );
template<typename F>
void ExplicitTriang( AbstractDistMatrix<F>& A );

} // namespace rq

// Generalized QR
// ==============
template<typename F>
void GQR
( Matrix<F>& A,
  Matrix<F>& householderScalarsA,
  Matrix<Base<F>>& signatureA,
  Matrix<F>& B,
  Matrix<F>& householderScalarsB,
  Matrix<Base<F>>& signatureB );
template<typename F>
void GQR
( AbstractDistMatrix<F>& A,
  AbstractDistMatrix<F>& householderScalarsA,
  AbstractDistMatrix<Base<F>>& signatureA,
  AbstractDistMatrix<F>& B,
  AbstractDistMatrix<F>& householderScalarsB,
  AbstractDistMatrix<Base<F>>& signatureB );

namespace gqr {

template<typename F>
void ExplicitTriang( Matrix<F>& A, Matrix<F>& B );
template<typename F>
void ExplicitTriang( AbstractDistMatrix<F>& A, AbstractDistMatrix<F>& B );

} // namespace gqr

// Generalized RQ
// ==============
template<typename F>
void GRQ
( Matrix<F>& A,
  Matrix<F>& householderScalarsA,
  Matrix<Base<F>>& signatureA,
  Matrix<F>& B,
  Matrix<F>& householderScalarsB,
  Matrix<Base<F>>& signatureB );
template<typename F>
void GRQ
( AbstractDistMatrix<F>& A,
  AbstractDistMatrix<F>& householderScalarsA,
  AbstractDistMatrix<Base<F>>& signatureA,
  AbstractDistMatrix<F>& B,
  AbstractDistMatrix<F>& householderScalarsB,
  AbstractDistMatrix<Base<F>>& signatureB );

namespace grq {

template<typename F>
void ExplicitTriang( Matrix<F>& A, Matrix<F>& B );
template<typename F>
void ExplicitTriang( AbstractDistMatrix<F>& A, AbstractDistMatrix<F>& B );

} // namespace grq

// Interpolative Decomposition
// ===========================
template<typename F>
void ID
( const Matrix<F>& A,
        Permutation& P,
        Matrix<F>& Z,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );
template<typename F>
void ID
( const AbstractDistMatrix<F>& A,
        DistPermutation& P,
        AbstractDistMatrix<F>& Z,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );

template<typename F>
void ID
( Matrix<F>& A,
  Permutation& P,
  Matrix<F>& Z,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>(),
  bool canOverwrite=false );
template<typename F>
void ID
( AbstractDistMatrix<F>& A,
  DistPermutation& P,
  AbstractDistMatrix<F>& Z,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>(),
  bool canOverwrite=false );

// Skeleton
// ========
template<typename F>
void Skeleton
( const Matrix<F>& A,
        Permutation& PR,
        Permutation& PC,
        Matrix<F>& Z,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );
template<typename F>
void Skeleton
( const AbstractDistMatrix<F>& A,
        DistPermutation& PR,
        DistPermutation& PC,
        AbstractDistMatrix<F>& Z,
  const QRCtrl<Base<F>>& ctrl=QRCtrl<Base<F>>() );

} // namespace El

#include <El/lapack_like/factor/qr/ProxyHouseholder.hpp>

#endif // ifndef EL_FACTOR_HPP
