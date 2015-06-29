/*
 *  Header file for cuRNN math functions.
 *
 *  Copyright (C) 2015 Rob Clucas robclu1818@gmail.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT AN_size.y WARRANTY; without even the implied warranty of
 *  MERCHANTABILIT_size.y or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation,
 *	Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _CURNN_MATH_
#define	_CURNN_MATH_

#include <cuda_runtime.h>
#include <cublas_v2.h>

#include <vector>

#include "../util/errors.h"
#include "../curnn/curnn.h"
#include "math.cuh"

namespace curnn {
	namespace math  {
		
		/*
		 * ==================================================================================================     
		 * Function		: axpy
		 *
		 * Description	: Performs simgle precision a*X + Y
		 *
		 * Inputs		: error		: cuRNN error type for result of operations
		 *				: a			: Constant for multiplication 
		 *              : x			: Vector to multiply with a
		 * 
		 * Outputs/(I)	: y			: Vector used in a*X + Y, and where the result of a*X + Y is stored
		 * ==================================================================================================
		 */
		void axpy( curnnError& error   , const float a, 
				   const std::vector<float>& x, std::vector<float>& y );	

		/*
		 * ==================================================================================================     
		 * Function		: axpy
		 *
		 * Description	: Performs double precision a*X + Y
		 *
		 * Inputs		: error		: cuRNN error type for result of operations
		 *				: a			: Constant for multiplication 
		 *              : x			: Vector to multiply with a
		 * 
		 * Outputs/(I)	: y			: Vector used in a*X + Y, and where the result of a*X + Y is stored
		 * ==================================================================================================
		 */
		void axpy( curnnError& error    , const double a, 
				   const std::vector<double>& x, std::vector<double>& y );	


		/*
		 * ==================================================================================================     
		 * Function		: sum 
		 *
		 * Description	: Performs the sum of the elements in a vector
		 *					
		 * Inputs		: error		: cuRNN error type for results of operations
		 *				: x			: The vector, araary etc.. (data) to comupte the sum of
		 *        
		 * Outputs		: val		: The result of the sum of the array 
		 *
		 * Params		: dType		: The data type of the array elements
		 * ==================================================================================================
		 */	 
		template <typename dType>
		dType sum( curnnError& error, const std::vector<dType>& x );
		
		/*
		 * ==================================================================================================     
		 * Function		: softmax
		 *
		 * Description	: Performs the softmax function of a vector of floats x, which is 
		 *					
		 *				  softmax( x_i ) = exp( x_i ) / sum[ j=1 to J ]( exp( x_j )
		 *
		 * Inputs		: status	: Cublas status for determining correct completion of operation
		 *        
		 * Outputs/(I)  : x			: Vector to compute the softmax of, and to store the result in
		 * ==================================================================================================
		 */	 
		void softmax( cublasStatus_t& status, std::vector<float>& x );

		/* ===================== Implementations for templated functions ================================== */

		template <typename dType>
		dType sum( curnnError& error, const std::vector<dType>& x ) {

			// Declare device pointers, and (non-pointer) result val
			dType* in = 0, *out = 0, val = 0;

			// Alllocate memory on the device
			if ( cudaMalloc( (void**)&in, x.size() * sizeof( dType ) ) != cudaSuccess ) {
				curnn::err::allocError( error, stringify( in ) );
			}
			if ( cudaMalloc( (void**)&out, sizeof( dType) ) != cudaSuccess ) {
				curnn::err::allocError( error, stringify( out ) );
			}

			// Copy data from x to in
			if ( cudaMemcpy( in, &x[0], x.size() * sizeof( dType ), cudaMemcpyHostToDevice ) != cudaSuccess ) {
				curnn::err::copyError( error, stringify( in ) );
			}
			// Set out to 0 on the device
			if ( cudaMemsetAsync( out, 0, sizeof( dType) ) != cudaSuccess ) {
				curnn::err::copyError( error, stringify( out ) );
			}

			// Determine the size of the grids for the kernel
			int threads = 256;
			int blocks  = min( ( ( x.size() / 2 ) + threads - 1 ) / threads, MAX_BLOCKS );

			// Execute kernel
			blockReduceAtomicVectorized<<<blocks, threads>>>( in, out, x.size() );

			// copy result from out to val 
			if ( cudaMemcpy( &val, out, sizeof( dType ), cudaMemcpyDeviceToHost ) != cudaSuccess ) {
				curnn::err::copyError( error, stringify( out ) );
			}

			// Free device memory
			cudaFree( in ); cudaFree( out );

			return val;
		}
	}
}

#endif
