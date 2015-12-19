/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef EL_LATTICE_HPP
#define EL_LATTICE_HPP

namespace El {

// TODO: Maintain B in BigInt form

struct LLLInfo
{
    Int nullity; 
    Int numBacktracks;
};

template<typename F>
LLLInfo LLL
( Matrix<F>& B,
  Matrix<F>& QR,
  Base<F> delta,
  Base<F> innerTol=0,
  bool weak=false,
  bool presort=false,
  bool smallestFirst=true,
  bool progress=false,
  bool time=false );

template<typename F>
LLLInfo LLL
( Matrix<F>& B,
  Matrix<F>& U,
  Matrix<F>& UInv,
  Matrix<F>& QR,
  Base<F> delta,
  Base<F> innerTol=0,
  bool weak=false,
  bool presort=false,
  bool smallestFirst=true,
  bool progress=false,
  bool time=false );

template<typename F>
Base<F> LLLDelta
( const Matrix<F>& QR,
  bool weak=false,
  Base<F> zeroTol=limits::Epsilon<Base<F>>() );

template<typename F>
void LatticeGramSchmidt( const Matrix<F>& B, Matrix<F>& G, Matrix<F>& M );

} // namespace El

#endif // ifndef EL_LATTICE_HPP
