/*
   Copyright (c) 2009-2010, Jack Poulson
   All rights reserved.

   This file is part of Elemental.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    - Neither the name of the owner nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/
#include "elemental/blas_internal.hpp"
#include "elemental/lapack_internal.hpp"
using namespace std;
using namespace elemental;
using namespace elemental::wrappers::mpi;

template<typename R>
void
elemental::lapack::internal::PanelTridiagU
( DistMatrix<R,MC,MR  >& A,
  DistMatrix<R,MC,MR  >& W,
  DistMatrix<R,MD,Star>& e )
{
#ifndef RELEASE
    PushCallStack("lapack::internal::PanelTridiagU");
    if( A.Grid() != W.Grid() || W.Grid() != e.Grid() )
        throw logic_error
        ( "A, W, and e must be distributed over the same grid." );
    if( A.Height() != A.Width() )
        throw logic_error( "A must be square." );
    if( A.Height() != W.Height() )
        throw logic_error( "A and W must be the same height." );
    if( W.Width() > W.Height() )
        throw logic_error( "W must be a column panel." );
    if( W.ColAlignment() != A.ColAlignment() || 
        W.RowAlignment() != 
          ((A.RowAlignment()+A.Width()-W.Width())%A.Grid().Width()) )
        throw logic_error( "W and A must be aligned." );
    if( e.Height() != W.Width() || e.Width() != 1 )
        throw logic_error
        ( "e must be a column vector of the same length as W's width." );
    {
        DistMatrix<R,MC,MR> A11Expanded(A.Grid());
        A11Expanded.View
        ( A, A.Height()-W.Width()-1, A.Width()-W.Width()-1,
             W.Width()+1, W.Width()+1 );
        if( !e.AlignedWithDiag( A11Expanded, 1 ) )
            throw logic_error( "e is not correctly aligned with A." );
    }
#endif
    const Grid& g = A.Grid();

    // Matrix views 
    DistMatrix<R,MC,MR> 
        ATL(g), ATR(g),  A00(g), a01(g),     A02(g),  ACol(g),  a01T(g),
        ABL(g), ABR(g),  a10(g), alpha11(g), a12(g),            alpha01B(g),
                         A20(g), a21(g),     A22(g);
    DistMatrix<R,MC,MR> 
        WTL(g), WTR(g),  W00(g), w01(g),     W02(g),  WCol(g),
        WBL(g), WBR(g),  w10(g), omega11(g), w12(g),
                         W20(g), w21(g),     W22(g);
    DistMatrix<R,MD,Star> eT(g),  e0(g),
                          eB(g),  epsilon1(g),
                                  e2(g);

    // Temporary distributions
    DistMatrix<R,MC,  Star> a01_MC_Star(g);
    DistMatrix<R,MR,  Star> a01_MR_Star(g);
    DistMatrix<R,Star,MR  > z12_Star_MR(g);
    DistMatrix<R,MC,  MR  > z01(g);
    DistMatrix<R,MC,  Star> z01_MC_Star(g);
    DistMatrix<R,MR,  Star> z01_MR_Star(g);
    DistMatrix<R,MR,  MC  > z01_MR_MC(g);

    // Push to the blocksize of 1, then pop at the end of the routine
    PushBlocksizeStack( 1 );

    PartitionUpRightDiagonal
    ( A, ATL, ATR,
         ABL, ABR, 0 );
    PartitionUpRightDiagonal
    ( W, WTL, WTR,
         WBL, WBR, 0 );
    PartitionUp
    ( e, eT,
         eB, 0 );
    while( WBR.Width() < W.Width() )
    {
        RepartitionUpDiagonal
        ( ATL, /**/ ATR,  A00, a01,     /**/ A02,
               /**/       a10, alpha11, /**/ a12, 
         /*************/ /**********************/
          ABL, /**/ ABR,  A20, a21,     /**/ A22 );
        
        RepartitionUpDiagonal
        ( WTL, /**/ WTR,  W00, w01,     /**/ W02,
               /**/       w10, omega11, /**/ w12,
         /*************/ /**********************/
          WBL, /**/ WBR,  W20, w21,     /**/ W22 );

        RepartitionUp
        ( eT,  e0,
               epsilon1,
         /**/ /********/
          eB,  e2 );

        ACol.View2x1( a01,
                      alpha11 );

        WCol.View2x1( w01,
                      omega11 );
        
        PartitionUp
        ( a01, a01T,
               alpha01B, 1 );

        a01_MC_Star.AlignWith( A00 );
        a01_MR_Star.AlignWith( A00 );
        z12_Star_MR.AlignWith( W02 );
        z01.AlignWith( w01 );
        z01_MC_Star.AlignWith( A00 );
        z01_MR_Star.AlignWith( A00 );
        z01_MR_MC.AlignColsWith( A00 );
        z01_MC_Star.ResizeTo( w01.Height(), 1 );
        z01_MR_Star.ResizeTo( w01.Height(), 1 );
        z12_Star_MR.ResizeTo( 1, w12.Width() );
        z01_MC_Star.SetToZero();
        z01_MR_Star.SetToZero();
        //--------------------------------------------------------------------//
        blas::Gemv( Normal, (R)-1, ATR, w12, (R)1, ACol );
        blas::Gemv( Normal, (R)-1, WTR, a12, (R)1, ACol );

        R tau = 0; // Initializing avoids false compiler warnings
        const bool thisIsMyColumn = ( g.MRRank() == a01.RowAlignment() );
        if( thisIsMyColumn )
            tau = lapack::internal::ColReflector( alpha01B, a01T );
            
        alpha01B.GetDiagonal( epsilon1 );
        alpha01B.Set( 0, 0, (R)1 );

        a01_MR_Star = a01_MC_Star = a01;
        
        PopBlocksizeStack();
        blas::internal::LocalSymvColAccumulateU
        ( (R)1, A00, a01_MC_Star, a01_MR_Star, z01_MC_Star, z01_MR_Star );
        PushBlocksizeStack( 1 );

        blas::Gemv
        ( Transpose, 
          (R)1, W02.LockedLocalMatrix(),
                a01_MC_Star.LockedLocalMatrix(),
          (R)0, z12_Star_MR.LocalMatrix() );
        z12_Star_MR.SumOverCol();

        blas::Gemv
        ( Normal,
          (R)-1, A02.LockedLocalMatrix(),
                 z12_Star_MR.LockedLocalMatrix(),
          (R)+1, z01_MC_Star.LocalMatrix() );

        blas::Gemv
        ( Transpose,
          (R)1, A02.LockedLocalMatrix(),
                a01_MC_Star.LockedLocalMatrix(),
          (R)0, z12_Star_MR.LocalMatrix() );
        z12_Star_MR.SumOverCol();

        blas::Gemv
        ( Normal,
          (R)-1, W02.LockedLocalMatrix(),
                 z12_Star_MR.LockedLocalMatrix(),
          (R)+1, z01_MC_Star.LocalMatrix() );

        w01.SumScatterFrom( z01_MC_Star );
        z01_MR_MC.SumScatterFrom( z01_MR_Star );
        z01 = z01_MR_MC;

        if( thisIsMyColumn )
        {
            blas::Axpy( (R)1, z01, w01 );
            blas::Scal( tau, w01 );

            R alpha;
            R myAlpha = -static_cast<R>(0.5)*tau*
                        blas::Dot( w01.LockedLocalMatrix(),
                                   a01.LockedLocalMatrix() );
            AllReduce( &myAlpha, &alpha, 1, MPI_SUM, g.MCComm() );
            blas::Axpy( alpha, a01, w01 );
        }
        //--------------------------------------------------------------------//
        a01_MC_Star.FreeAlignments();
        a01_MR_Star.FreeAlignments();
        z12_Star_MR.FreeAlignments();
        z01.FreeAlignments();
        z01_MC_Star.FreeAlignments();
        z01_MR_Star.FreeAlignments();
        z01_MR_MC.FreeAlignments();

        SlidePartitionUpDiagonal
        ( ATL, /**/ ATR,  A00, /**/ a01,     A02,
         /*************/ /**********************/
               /**/       a10, /**/ alpha11, a12,
          ABL, /**/ ABR,  A20, /**/ a21,     A22 );

        SlidePartitionUpDiagonal
        ( WTL, /**/ WTR,  W00, /**/ w01,     W02,
         /*************/ /**********************/
               /**/       w10, /**/ omega11, w12,
          WBL, /**/ WBR,  W20, /**/ w21,     W22 );
        
        SlidePartitionUp
        ( eT,  e0,
         /**/ /********/
               epsilon1,
          eB,  e2 );
    }
    PopBlocksizeStack();
#ifndef RELEASE
    PopCallStack();
#endif
}

#ifndef WITHOUT_COMPLEX
template<typename R>
void
elemental::lapack::internal::PanelTridiagU
( DistMatrix<complex<R>,MC,  MR  >& A,
  DistMatrix<complex<R>,MC,  MR  >& W,
  DistMatrix<R,MD,Star>& e,
  DistMatrix<complex<R>,MD,  Star>& t )
{
#ifndef RELEASE
    PushCallStack("lapack::internal::PanelTridiagU");
    if( A.Grid() != W.Grid() || 
        W.Grid() != e.Grid() ||
        e.Grid() != t.Grid() )
        throw logic_error
        ( "A, W, t, and e must be distributed over the same grid." );
    if( A.Height() != A.Width() )
        throw logic_error( "A must be square." );
    if( A.Height() != W.Height() )
        throw logic_error( "A and W must be the same height." );
    if( W.Width() > W.Height() )
        throw logic_error( "W must be a column panel." );
    if( W.ColAlignment() != A.ColAlignment() || 
        W.RowAlignment() != 
          ((A.RowAlignment()+A.Width()-W.Width())%A.Grid().Width()) )
        throw logic_error( "W and A must be aligned." );
    if( e.Height() != W.Width() || e.Width() != 1 )
        throw logic_error
        ( "e must be a column vector of the same length as W's width." );    
    if( t.Height() != W.Width() || t.Width() != 1 )
        throw logic_error
              ( "t must be a column vector of the same length as W's width." );
    {
        DistMatrix<complex<R>,MC,MR> A11Expanded(A.Grid());
        A11Expanded.View
        ( A, A.Height()-W.Width()-1, A.Width()-W.Width()-1,
             W.Width()+1, W.Width()+1 );
        if( !e.AlignedWithDiag( A11Expanded, 1 ) )
            throw logic_error( "e is not correctly aligned with A." );
        if( !t.AlignedWithDiag( A11Expanded, 1 ) )
            throw logic_error( "t is not correctly aligned with A." );
    }
#endif
    typedef complex<R> C;

    const Grid& g = A.Grid();

    // Matrix views 
    DistMatrix<C,MC,MR> 
        ATL(g), ATR(g),  A00(g), a01(g),     A02(g),  ACol(g),  a01T(g),
        ABL(g), ABR(g),  a10(g), alpha11(g), a12(g),            alpha01B(g),
                         A20(g), a21(g),     A22(g);
    DistMatrix<C,MC,MR> 
        WTL(g), WTR(g),  W00(g), w01(g),     W02(g),  WCol(g),
        WBL(g), WBR(g),  w10(g), omega11(g), w12(g),
                         W20(g), w21(g),     W22(g);
    DistMatrix<R,MD,Star> eT(g),  e0(g),
                          eB(g),  epsilon1(g),
                                  e2(g);
    DistMatrix<C,MD,Star>
        tT(g),  t0(g),
        tB(g),  tau1(g),
                t2(g);

    // Temporary distributions
    DistMatrix<C,MC,  MR  > a12Conj(g);
    DistMatrix<C,MC,  MR  > w12Conj(g);
    DistMatrix<C,MC,  Star> a01_MC_Star(g);
    DistMatrix<C,MR,  Star> a01_MR_Star(g);
    DistMatrix<C,Star,MR  > z12_Star_MR(g);
    DistMatrix<C,MC,  MR  > z01(g);
    DistMatrix<C,MC,  Star> z01_MC_Star(g);
    DistMatrix<C,MR,  Star> z01_MR_Star(g);
    DistMatrix<C,MR,  MC  > z01_MR_MC(g);

    // Push to the blocksize of 1, then pop at the end of the routine
    PushBlocksizeStack( 1 );

    PartitionUpRightDiagonal
    ( A, ATL, ATR,
         ABL, ABR, 0 );
    PartitionUpRightDiagonal
    ( W, WTL, WTR,
         WBL, WBR, 0 );
    PartitionUp
    ( e, eT,
         eB, 0 );
    PartitionUp
    ( t, tT, 
         tB, 0 );
    while( WBR.Width() < W.Width() )
    {
        RepartitionUpDiagonal
        ( ATL, /**/ ATR,  A00, a01,     /**/ A02,
               /**/       a10, alpha11, /**/ a12, 
         /*************/ /**********************/
          ABL, /**/ ABR,  A20, a21,     /**/ A22 );
        
        RepartitionUpDiagonal
        ( WTL, /**/ WTR,  W00, w01,     /**/ W02,
               /**/       w10, omega11, /**/ w12,
         /*************/ /**********************/
          WBL, /**/ WBR,  W20, w21,     /**/ W22 );

        RepartitionUp
        ( eT,  e0,
               epsilon1,
         /**/ /********/
          eB,  e2 );

        RepartitionUp
        ( tT,  t0,
               tau1,
         /**/ /****/
          tB,  t2 );

        ACol.View2x1( a01,
                      alpha11 );

        WCol.View2x1( w01,
                      omega11 );

        PartitionUp
        ( a01, a01T,
               alpha01B, 1 );

        a01_MC_Star.AlignWith( A00 );
        a01_MR_Star.AlignWith( A00 );
        z12_Star_MR.AlignWith( W02 );
        z01.AlignWith( w01 );
        z01_MC_Star.AlignWith( A00 );
        z01_MR_Star.AlignWith( A00 );
        z01_MR_MC.AlignColsWith( A00 );
        z01_MC_Star.ResizeTo( w01.Height(), 1 );
        z01_MR_Star.ResizeTo( w01.Height(), 1 );
        z12_Star_MR.ResizeTo( 1, w12.Width() );
        z01_MC_Star.SetToZero();
        z01_MR_Star.SetToZero();
        //--------------------------------------------------------------------//
        alpha11.SetImag( 0, 0, (R)0 );
        blas::Conj( w12, w12Conj );
        blas::Gemv( Normal, (C)-1, ATR, w12Conj, (C)1, ACol );
        blas::Conj( a12, a12Conj );
        blas::Gemv( Normal, (C)-1, WTR, a12Conj, (C)1, ACol );
        alpha11.SetImag( 0, 0, (R)0 );

        C tau = 0; // Initializing avoids false compiler warnings
        const bool thisIsMyColumn = ( g.MRRank() == a01.RowAlignment() );
        if( thisIsMyColumn )
        {
            tau = lapack::internal::ColReflector( alpha01B, a01T );
            const bool thisIsMyRow = ( g.MCRank() == alpha01B.ColAlignment() );
            if( thisIsMyRow )
                tau1.SetLocalEntry(0,0,tau);
        }
            
        alpha01B.GetRealDiagonal( epsilon1 );
        alpha01B.Set( 0, 0, (C)1 );

        a01_MR_Star = a01_MC_Star = a01;

        PopBlocksizeStack();
        blas::internal::LocalHemvColAccumulateU
        ( (C)1, A00, a01_MC_Star, a01_MR_Star, z01_MC_Star, z01_MR_Star );
        PushBlocksizeStack( 1 );

        blas::Gemv
        ( ConjugateTranspose, 
          (C)1, W02.LockedLocalMatrix(),
                a01_MC_Star.LockedLocalMatrix(),
          (C)0, z12_Star_MR.LocalMatrix() );
        z12_Star_MR.SumOverCol();

        blas::Gemv
        ( Normal,
          (C)-1, A02.LockedLocalMatrix(),
                 z12_Star_MR.LockedLocalMatrix(),
          (C)+1, z01_MC_Star.LocalMatrix() );

        blas::Gemv
        ( ConjugateTranspose,
          (C)1, A02.LockedLocalMatrix(),
                a01_MC_Star.LockedLocalMatrix(),
          (C)0, z12_Star_MR.LocalMatrix() );
        z12_Star_MR.SumOverCol();

        blas::Gemv
        ( Normal,
          (C)-1, W02.LockedLocalMatrix(),
                 z12_Star_MR.LockedLocalMatrix(),
          (C)+1, z01_MC_Star.LocalMatrix() );

        w01.SumScatterFrom( z01_MC_Star );
        z01_MR_MC.SumScatterFrom( z01_MR_Star );
        z01 = z01_MR_MC;

        if( thisIsMyColumn )
        {
            blas::Axpy( (C)1, z01, w01 );
            blas::Scal( tau, w01 );

            C alpha;
            C myAlpha = -static_cast<R>(0.5)*tau*
                        blas::Dot( w01.LockedLocalMatrix(),
                                   a01.LockedLocalMatrix() );
            AllReduce( &myAlpha, &alpha, 1, MPI_SUM, g.MCComm() );
            blas::Axpy( alpha, a01, w01 );
        }
        //--------------------------------------------------------------------//
        a01_MC_Star.FreeAlignments();
        a01_MR_Star.FreeAlignments();
        z12_Star_MR.FreeAlignments();
        z01.FreeAlignments();
        z01_MC_Star.FreeAlignments();
        z01_MR_Star.FreeAlignments();
        z01_MR_MC.FreeAlignments();

        SlidePartitionUpDiagonal
        ( ATL, /**/ ATR,  A00, /**/ a01,     A02,
         /*************/ /**********************/
               /**/       a10, /**/ alpha11, a12,
          ABL, /**/ ABR,  A20, /**/ a21,     A22 );

        SlidePartitionUpDiagonal
        ( WTL, /**/ WTR,  W00, /**/ w01,     W02,
         /*************/ /**********************/
               /**/       w10, /**/ omega11, w12,
          WBL, /**/ WBR,  W20, /**/ w21,     W22 );
        
        SlidePartitionUp
        ( eT,  e0,
         /**/ /********/
               epsilon1,
          eB,  e2 );

        SlidePartitionUp
        ( tT,  t0,
         /**/ /****/
               tau1,
          tB,  t2 );
    }
    PopBlocksizeStack();
#ifndef RELEASE
    PopCallStack();
#endif
}
#endif // WITHOUT_COMPLEX

template void elemental::lapack::internal::PanelTridiagU
( DistMatrix<float,MC,MR  >& A,
  DistMatrix<float,MC,MR  >& W,
  DistMatrix<float,MD,Star>& e );

template void elemental::lapack::internal::PanelTridiagU
( DistMatrix<double,MC,MR  >& A,
  DistMatrix<double,MC,MR  >& W,
  DistMatrix<double,MD,Star>& e );

#ifndef WITHOUT_COMPLEX
template void elemental::lapack::internal::PanelTridiagU
( DistMatrix<scomplex,MC,MR  >& A,
  DistMatrix<scomplex,MC,MR  >& W,
  DistMatrix<float,   MD,Star>& e,
  DistMatrix<scomplex,MD,Star>& t );

template void elemental::lapack::internal::PanelTridiagU
( DistMatrix<dcomplex,MC,MR  >& A,
  DistMatrix<dcomplex,MC,MR  >& W,
  DistMatrix<double,  MD,Star>& e,
  DistMatrix<dcomplex,MD,Star>& t );
#endif

