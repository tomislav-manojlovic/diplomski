#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <stride>\n";
		return 1;
	}

	const size_t n = 1ULL << 26;
	const size_t target_accesses = 1ULL << 28;
	const size_t stride = std::stoull(argv[1]);

	std::vector<int> a(n, 1);

	const size_t accesses_per_pass = (n + stride - 1) / stride;
	const size_t repetitions = (target_accesses + accesses_per_pass -1) / accesses_per_pass;

	long long sum = 0;
	size_t accesses = 0;

	auto start = std::chrono::steady_clock::now();

	for (size_t r = 0; r < repetitions; ++r) {
		for (size_t i = 0; i < n; i += stride) {
			sum += a[i];
			++accesses;
		}
	}

	auto end = std::chrono::steady_clock::now();

	double seconds = std::chrono::duration<double>(end - start).count();

	std::cout << "stride: " << stride << '\n';
	std::cout << "accesses: " << accesses << '\n';
	std::cout << "time: " << seconds << '\n';
	std::cout << "ns/access: " << seconds * 1e9 / accesses << '\n';
	std::cout << "sum: " << sum << '\n';
}
