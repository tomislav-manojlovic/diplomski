#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <cstdlib>
#include <algorithm>

using Matrix = std::vector<double>;

void multiply_ijk(const Matrix& A, const Matrix& B, Matrix& C, int n) {
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			double sum = 0.0;

			for (int k = 0; k < n; k++)
				sum += A[i * n + k] * B[k * n + j];

			C[i * n + j] = sum;
		}
	}
}

void multiply_ikj(const Matrix& A, const Matrix& B, Matrix& C, int n) {
	for (int i = 0; i < n; i++) {
		for (int k = 0; k < n; k++) {
			double a = A[i * n + k];

			for (int j = 0; j < n; j++)
				C[i * n + j] += a * B[k * n + j];
		}
	}
}

void multiply_jik(const Matrix& A, const Matrix& B, Matrix& C, int n) {
	for (int j = 0; j < n; j++) {
		for (int i = 0; i < n; i++) {
			double sum = 0.0;

			for (int k = 0; k < n; k++)
				sum += A[i * n + k] * B[k * n + j];

			C[i * n + j] = sum;
		}
	}
}

void multiply_jki(const Matrix& A, const Matrix& B, Matrix& C, int n) {
	for (int j = 0; j < n; j++) {
		for (int k = 0; k < n; k++) {
			double b = B[k * n + j];

			for (int i = 0; i < n; i++)
				C[i * n + j] += A[i * n + k] * b;
		}
	}
}

void multiply_kij(const Matrix& A, const Matrix& B, Matrix& C, int n) {
	for (int k = 0; k < n; k++) {
		for (int i = 0; i < n; i++) {
			double a = A[i * n + k];

			for (int j = 0; j < n; j++)
				C[i * n + j] += a * B[k * n + j];
		}
	}
}

void multiply_kji(const Matrix& A, const Matrix& B, Matrix& C, int n) {
	for (int k = 0; k < n; k++) {
		for (int j = 0; j < n; j++) {
			double b = B[k * n + j];

			for (int i = 0; i < n; i++)
				C[i * n + j] += A[i * n + k] * b;
		}
	}
}

void multiply_blocked(const Matrix& A, const Matrix& B, Matrix& C, int n, int block_size) {
	for (int ii = 0; ii < n; ii += block_size) {
		for (int kk = 0; kk < n; kk += block_size) {
			for (int jj = 0; jj < n; jj += block_size) {
				int i_end = std::min(ii + block_size, n);
				int k_end = std::min(kk + block_size, n); 
				int j_end = std::min(jj + block_size, n); 

				for (int i = ii; i < i_end; i++) {
					for (int k = kk; k < k_end; k++) {
						double a = A[i * n + k];

						for (int j = jj; j < j_end; j++)
							C[i * n + j] += a * B[k * n + j];
					}
				}
			}
		}
	}	
}

void transpose(const Matrix& B, Matrix& BT, int n) {
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			BT[j * n + i] = B[i * n + j];
}

void multiply_transposed(const Matrix& A, const Matrix& BT, Matrix& C, int n) {
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			double sum = 0.0;

			for (int k = 0; k < n; k++)
				sum += A[i * n + k] * BT[j * n + k];

			C[i * n + j] = sum;
		}
	}
}

void multiply_recursive(const Matrix& A, const Matrix& B, Matrix& C, int n, int stride, int ar, int ac, int br, int bc, int cr, int cc, int cutoff) {
	if (n <= cutoff) {
		for (int i = 0; i < n; i++) {
			for (int k = 0; k < n; k++) {
				double a = A[(ar + i) * stride + (ac + k)];
				
				for (int j = 0; j < n; j++)
					C[(cr + i) * stride + (cc + j)] += a * B[(br + k) * stride + (bc + j)];
			}
		}
	
		return;
	}

	int h = n / 2;

	// C12 = A11*B12 + A12*B22
	multiply_recursive(A, B, C, h, stride, ar, ac, br, bc, cr, cc, cutoff);	
	multiply_recursive(A, B, C, h, stride, ar, ac + h, br + h, bc, cr, cc, cutoff);

	// C21 = A21*B11 + A22*B21
	multiply_recursive(A, B, C, h, stride, ar, ac, br, bc + h, cr, cc + h, cutoff);	
	multiply_recursive(A, B, C, h, stride, ar, ac + h, br + h, bc + h, cr, cc + h, cutoff);	

	// C21 = A21*B11 + A22*B21
	multiply_recursive(A, B, C, h, stride, ar + h, ac, br, bc, cr + h, cc, cutoff);	
	multiply_recursive(A, B, C, h, stride, ar + h, ac + h, br + h, bc, cr + h, cc, cutoff);	
			
	// C22 = A21*B12 + A22*B22
	multiply_recursive(A, B, C, h, stride, ar + h, ac, br, bc + h, cr + h, cc + h, cutoff);	
	multiply_recursive(A, B, C, h, stride, ar + h, ac + h, br + h, bc + h, cr + h, cc + h, cutoff);	
}

void multiply_cache_oblivious(const Matrix& A, const Matrix& B, Matrix& C, int n, int cutoff) {
	multiply_recursive(A, B, C, n, n, 0, 0, 0, 0, 0, 0, cutoff);
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cerr << "Usage:\n";
		std::cerr << argv[0] << " N order\n";
		std::cerr << argv[0] << " N blocked block_size\n";
		return 1;
	}

	int n = std::atoi(argv[1]);
	std::string order = argv[2];

	Matrix A(n * n);
	Matrix B(n * n);
	Matrix C(n * n, 0.0);
	Matrix BT(n * n);

	for (int i = 0; i < n * n; i++) {
		A[i] = (i % 100) * 0.01;
		B[i] = ((i * 3) % 100) * 0.01;
	}

	transpose(B, BT, n);

	auto start = std::chrono::steady_clock::now();

	if (order == "ijk")
		multiply_ijk(A, B, C, n);
	else if (order == "ikj")
		multiply_ikj(A, B, C, n);
	else if (order == "jik")
		multiply_jik(A, B, C, n);
	else if (order == "jki")
		multiply_jki(A, B, C, n);
	else if (order == "kij")
		multiply_kij(A, B, C, n);
	else if (order == "kji")
		multiply_kji(A, B, C, n);
	else if (order == "blocked") {
		if (argc != 4) {
			std::cerr << "block size required\n";
			return 1;
		}
		
		int block_size = std::atoi(argv[3]);
		multiply_blocked(A, B, C, n, block_size);
	}
	else if (order == "transposed")
		multiply_transposed(A, BT, C, n);
	else if (order == "recursive") {
		if (argc != 4) {
			std::cerr << "cutoff required\n";
			return 1;
		}

		int cutoff = std::atoi(argv[3]);

		if (cutoff <= 0) {
			std::cerr << "invalid cutoff\n";
			return 1;
		}

		multiply_cache_oblivious(A, B, C, n, cutoff);
	}
	else {
		std::cerr << "Invalid order\n";
		return 1;	
	}

	auto end = std::chrono::steady_clock::now();
	
	double seconds = std::chrono::duration<double>(end - start).count();

	double operations = 2.0 * static_cast<double>(n) * n * n;
	double gflops = operations / seconds / 1e9;

	std::cout << "N: " << n << '\n';
	std::cout << "order: " << order << '\n';

	if (order == "blocked")
		std::cout << "block size: " << argv[3] << '\n';

	if (order == "recursive")
		std::cout << "cutoff: " << argv[3] << '\n';

	std::cout << "time: " << seconds << '\n';
	std::cout << "GFLOP/s: " << gflops << '\n';

	// Sprecava compiler da potpuno eliminise racunanje
	std::cout << "checksum: " << C[(n / 2) * n + n / 2] << '\n';
}

