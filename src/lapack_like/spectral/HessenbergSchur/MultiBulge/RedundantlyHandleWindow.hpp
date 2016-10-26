/*
   Copyright (c) 2009-2016, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#ifndef EL_HESS_SCHUR_MULTIBULGE_REDUNDANTLY_HANDLE_WINDOW_HPP
#define EL_HESS_SCHUR_MULTIBULGE_REDUNDANTLY_HANDLE_WINDOW_HPP

#include "./Transform.hpp"

namespace El {
namespace hess_schur {
namespace multibulge {

template<typename F>
HessenbergSchurInfo
RedundantlyHandleWindow
( DistMatrix<F,MC,MR,BLOCK>& H,
  DistMatrix<Complex<Base<F>>,STAR,STAR>& w,
  DistMatrix<F,MC,MR,BLOCK>& Z,
  const HessenbergSchurCtrl& ctrl )
{
    DEBUG_CSE
    const Int n = H.Height();
    const Int winBeg = ( ctrl.winBeg==END ? n : ctrl.winBeg );
    const Int winEnd = ( ctrl.winEnd==END ? n : ctrl.winEnd );
    const Int winSize = winEnd - winBeg;

    const auto winInd = IR(winBeg,winEnd);
    auto HWin = H(winInd,winInd);
    auto& wLoc = w.Matrix();

    // Compute the Schur decomposition HWin = ZWin TWin ZWin',
    // where HWin is overwritten by TWin, and wWin by diag(TWin).
    DistMatrix<F,STAR,STAR> HWinFull( HWin );
    TestConsistency( HWinFull, "HWinRedundant" );
    auto wWin = wLoc(winInd,ALL);
    Matrix<F> ZWin;
    Identity( ZWin, winSize, winSize );
    auto info = HessenbergSchur( HWinFull.Matrix(), wWin, ZWin );
    TestConsistency( HWinFull, "HWinRedundantAfter" );
    Output("redundant window: [",winBeg,",",winEnd,")");
    Log("redundant window: [",winBeg,",",winEnd,")");
    /*
    Print( HWinFull, "HWin" );
    Print( HWinFull.Matrix(), "HWin", LogOS() );
    Print( ZWin, "ZWin", LogOS() );
    Print( wWin, "wWin", LogOS() );
    if( H.Grid().Rank() == 0 )
        Print( ZWin, "ZWin" );
    */
    HWin = HWinFull;

    if( ctrl.fullTriangle )
    {
        if( n > winEnd )
        {
            // Overwrite H(winInd,winEnd:n) *= ZWin'
            // (applied from the left)
            auto HRight = H( winInd, IR(winEnd,n) );
            //Print( HRight, "HRight" );
            TransformRows( ZWin, HRight );
            //Print( HRight, "HRightAfter" );
        }

        // Overwrite H(0:winBeg,winInd) *= ZWin
        auto HTop = H( IR(0,winBeg), winInd );
        //Print( HTop, "HTop" );
        TransformColumns( ZWin, HTop );
        //Print( HTop, "HTopAfter" );
    }
    if( ctrl.wantSchurVecs )
    {
        // Overwrite Z(:,winInd) *= ZWin
        auto ZBlock = Z( ALL, winInd );
        //Print( ZBlock, "ZBlock" );
        TransformColumns( ZWin, ZBlock );
        //Print( ZBlock, "ZBlockAfter" );
    }

    return info;
}

} // namespace multibulge
} // namespace hess_schur
} // namespace El

#endif // ifndef EL_HESS_SCHUR_MULTIBULGE_REDUNDANTLY_HANDLE_WINDOW_HPP
