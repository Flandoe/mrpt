/* +---------------------------------------------------------------------------+
   |                 The Mobile Robot Programming Toolkit (MRPT)               |
   |                                                                           |
   |                          http://www.mrpt.org/                             |
   |                                                                           |
   | Copyright (c) 2005-2013, Individual contributors, see AUTHORS file        |
   | Copyright (c) 2005-2013, MAPIR group, University of Malaga                |
   | Copyright (c) 2012-2013, University of Almeria                                 |
   | All rights reserved.                                                      |
   |                                                                           |
   | Redistribution and use in source and binary forms, with or without        |
   | modification, are permitted provided that the following conditions are    |
   | met:                                                                      |
   |    * Redistributions of source code must retain the above copyright       |
   |      notice, this list of conditions and the following disclaimer.        |
   |    * Redistributions in binary form must reproduce the above copyright    |
   |      notice, this list of conditions and the following disclaimer in the  |
   |      documentation and/or other materials provided with the distribution. |
   |    * Neither the name of the copyright holders nor the                    |
   |      names of its contributors may be used to endorse or promote products |
   |      derived from this software without specific prior written permission.|
   |                                                                           |
   | THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       |
   | 'AS IS' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED |
   | TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR|
   | PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE |
   | FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL|
   | DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR|
   |  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)       |
   | HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,       |
   | STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN  |
   | ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           |
   | POSSIBILITY OF SUCH DAMAGE.                                               |
   +---------------------------------------------------------------------------+ */


#include <mrpt/math.h>
#include <gtest/gtest.h>

using namespace mrpt;
using namespace mrpt::utils;
using namespace mrpt::math;
using namespace std;


void generateRandomSparseMatrix(size_t N, size_t M, size_t nEntries,  CSparseMatrix &MAT)
{
	MAT.clear();

	MAT.setRowCount(N);
	MAT.setColCount(M);

	for (size_t i=0;i<nEntries;i++)
	{
		MAT.insert_entry( 
			mrpt::random::randomGenerator.drawUniform32bit() % N,
			mrpt::random::randomGenerator.drawUniform32bit() % M,
			mrpt::random::randomGenerator.drawGaussian1D(0,1) );
	}

	// Return already compressed:
	MAT.compressFromTriplet();
}


void do_test_init_to_unit(size_t N)
{
	CMatrixDouble	 dense1;
	dense1.unit(N,1.0);

	CSparseMatrix SM(dense1);

	CMatrixDouble    dense_out;
	SM.get_dense(dense_out);

	EXPECT_TRUE( dense_out==dense1 ) <<
		"Failed with N=" << N << "\n";
}

TEST(SparseMatrix, InitFromDenseUnit)
{
	do_test_init_to_unit(1);
	do_test_init_to_unit(10);
	do_test_init_to_unit(100);
}


void do_test_init_random(size_t N)
{
	CMatrixDouble	 dense1;
	mrpt::random::randomGenerator.drawGaussian1DMatrix(dense1);
	CSparseMatrix SM(dense1);
	CMatrixDouble    dense_out;
	SM.get_dense(dense_out);
	EXPECT_TRUE( dense_out==dense1 ) <<
		"Failed with N=" << N << "\n";
}

TEST(SparseMatrix, InitFromDenseRandom)
{
	do_test_init_random(1);
	do_test_init_random(10);
	do_test_init_random(100);
}


TEST(SparseMatrix, InitFromTriplet)
{
	CSparseMatrix SM;
	CMatrixDouble  D(10,20);

	SM.insert_entry(2,2, 4.0);  D(2,2) = 4.0;
	SM.insert_entry(6,8, -2.0);  D(6,8) = -2.0;

	SM.setRowCount(10);
	SM.setColCount(20);

	CMatrixDouble    dense_out1;
	SM.get_dense(dense_out1);

	SM.compressFromTriplet();

	CMatrixDouble    dense_out2;
	SM.get_dense(dense_out2);

	EXPECT_TRUE(dense_out1==dense_out2);
}


TEST(SparseMatrix, InitFromSparse)
{
	CMatrixDouble  D(4,5);
	mrpt::math::CSparseMatrixTemplate<double>  S(4,5);
	D(1,2) = 2.0;
	S(1,2) = 2.0;

	D(3,1) = -7.0;
	S(3,1) = -7.0;

	CSparseMatrix SM(S);
	CMatrixDouble    dense_out;
	SM.get_dense(dense_out);
	EXPECT_TRUE( dense_out==D ) 
		<< "Dense: \n" << D 
		<< "Sparse:\n" << dense_out << endl;
}

TEST(SparseMatrix, InitFromRandom)
{
	CSparseMatrix SM;
	generateRandomSparseMatrix(100,100, 25, SM);
	generateRandomSparseMatrix(20,10, 15, SM);
}

typedef void (*TMatrixSMOperator)(const CSparseMatrix &M1, const CSparseMatrix &M2, CSparseMatrix &res);
typedef void (*TMatrixDenseOperator)(const CMatrixDouble &M1, const CMatrixDouble &M2, CMatrixDouble &res);

void do_matrix_op_test(
	size_t nRows1, size_t nCols1, size_t nNonZeros1,
	size_t nRows2, size_t nCols2, size_t nNonZeros2, 
	TMatrixSMOperator op1, TMatrixDenseOperator op2)
{
	CSparseMatrix SM1, SM2;
	generateRandomSparseMatrix(nRows1,nCols1,nNonZeros1, SM1);
	generateRandomSparseMatrix(nRows2,nCols2,nNonZeros2, SM2);

	CSparseMatrix SM_res;
	(*op1)(SM1,SM2,SM_res);

	// Check:
	CMatrixDouble    D1,D2,Dres;
	SM1.get_dense(D1);
	SM2.get_dense(D2);
	SM_res.get_dense(Dres);

	CMatrixDouble  RES;
	(*op2)(D1,D2,RES);

	const double err = (RES-Dres).Abs().maximum();

	EXPECT_TRUE(err<1e-10)
		<< "M1:\n" << D1 
		<< "M2:\n" << D2
		<< "Real op result:\n" << RES 
		<< "SM result:\n" << Dres 
		<< "ERR:\n" << (RES-Dres);	 
}

void op_sparse_add(const CSparseMatrix &M1, const CSparseMatrix &M2, CSparseMatrix &res) { res = M1+M2; }
void op_dense_add(const CMatrixDouble &M1, const CMatrixDouble &M2, CMatrixDouble &res) { res = M1+M2; }


TEST(SparseMatrix, Op_Add)
{
	do_matrix_op_test(1,1,0, 1,1,0, &op_sparse_add, &op_dense_add);
	do_matrix_op_test(1,1,1, 1,1,1, &op_sparse_add, &op_dense_add);
	do_matrix_op_test(2,2,1, 2,2,1, &op_sparse_add, &op_dense_add);
	do_matrix_op_test(10,20,33, 10,20,33, &op_sparse_add, &op_dense_add);
	do_matrix_op_test(11,21,34, 11,21,34, &op_sparse_add, &op_dense_add);
}

void op_sparse_multiply_AB(const CSparseMatrix &M1, const CSparseMatrix &M2, CSparseMatrix &res) { res = M1*M2; }
void op_dense_multiply_AB(const CMatrixDouble &M1, const CMatrixDouble &M2, CMatrixDouble &res) { res = M1*M2; }


TEST(SparseMatrix, Op_Multiply_AB)
{
	do_matrix_op_test(1,1,0, 1,1,0, &op_sparse_multiply_AB, &op_dense_multiply_AB);
	do_matrix_op_test(1,1,1, 1,1,1, &op_sparse_multiply_AB, &op_dense_multiply_AB);
	do_matrix_op_test(2,2,1, 2,2,1, &op_sparse_multiply_AB, &op_dense_multiply_AB);
	do_matrix_op_test(10,20,33, 20,15,33, &op_sparse_multiply_AB, &op_dense_multiply_AB);
	do_matrix_op_test(8,34,100, 34,3,100, &op_sparse_multiply_AB, &op_dense_multiply_AB);
}


TEST(SparseMatrix, CholeskyDecomp)
{
	CSparseMatrix SM(10,10);
	const CMatrixDouble COV1 = mrpt::random::randomGenerator.drawDefinitePositiveMatrix(6, 0.2);
	const CMatrixDouble COV2 = mrpt::random::randomGenerator.drawDefinitePositiveMatrix(4, 0.2);

	SM.insert_submatrix(0,0, COV1);
	SM.insert_submatrix(6,6, COV2);
	SM.compressFromTriplet();

	CSparseMatrix::CholeskyDecomp  Chol(SM);

	const CMatrixDouble L = Chol.get_L();  // lower triangle

	// Compare with the dense matrix implementation:
	CMatrixDouble D;
	SM.get_dense(D);

	CMatrixDouble Ud; // Upper triangle
	D.chol(Ud);

	const double err = ((Ud.transpose())-L).Abs().mean();
	EXPECT_TRUE(err<1e-8);
}

