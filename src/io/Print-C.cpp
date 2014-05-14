/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El-lite.hpp"
#include "El-C.h"
using namespace El;

#define RCM_s_const(AHandle) reinterpret_cast<const Matrix<float>*>(AHandle)
#define RCM_d_const(AHandle) reinterpret_cast<const Matrix<double>*>(AHandle)
#define RCM_c_const(AHandle) \
    reinterpret_cast<const Matrix<Complex<float>>*>(AHandle)
#define RCM_z_const(AHandle) \
    reinterpret_cast<const Matrix<Complex<double>>*>(AHandle)

#define CATCH catch( std::exception& e ) { ReportException(e); }

extern "C" {

void ElPrintMatrix_s( const ElMatrix_s* AHandle, const char* title )
{
    try { Print( *RCM_s_const(AHandle), std::string(title) ); }
    CATCH
}

void ElPrintMatrix_d( const ElMatrix_d* AHandle, const char* title )
{
    try { Print( *RCM_d_const(AHandle), std::string(title) ); }
    CATCH
}

void ElPrintMatrix_c( const ElMatrix_c* AHandle, const char* title )
{
    try { Print( *RCM_c_const(AHandle), std::string(title) ); }
    CATCH
}

void ElPrintMatrix_z( const ElMatrix_z* AHandle, const char* title )
{
    try { Print( *RCM_z_const(AHandle), std::string(title) ); }
    CATCH
}

} // extern "C"