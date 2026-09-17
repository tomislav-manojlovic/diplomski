#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <size_kib>\n";
		return 1;
	}

	const size_t size_kib = std::stoull(argv[1]);
	const size_t bytes = size_kib * 1024;
	const size_t n = bytes / sizeof(uint32_t);

	std::vector<uint32_t> next(n);
	std::vector<uint32_t> permutation(n);

	std::iota(permutation.begin(), permutation.end(), 0);

	std::mt19937 rng(42);
	std::shuffle(permutation.begin(), permutation.end(), rng);

	for (size_t i = 0; i < n; ++i)
		next[permutation[i]] = permutation[(i + 1) % n];

	const size_t accesses = 20'000'000;

	uint32_t index = 0;

	for (size_t i = 0; i < n; ++i)
		index = next[index];

	auto start = std::chrono::steady_clock::now();

	for (size_t i = 0; i < accesses; ++i)
		index = next[index];

	auto end = std::chrono::steady_clock::now();

	double seconds = std::chrono::duration<double>(end - start).count();

	std::cout << "working set: " << size_kib << " KiB\n";
	std::cout << "time: " << seconds << " s\n";
	std::cout << "ns/access: " << seconds * 1e9 / accesses << '\n';
	std::cout << "final index: " << index << '\n';
}
