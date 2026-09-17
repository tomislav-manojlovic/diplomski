#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <locations>\n";
		return 1;
	}
	
	const size_t locations = std::stoull(argv[1]);

	const size_t stride_bytes = 32 * 1024;
	const size_t stride = stride_bytes / sizeof(uint32_t);

	std::vector<uint32_t> next(locations * stride);

	for (size_t i = 0; i < locations; ++i) {
		size_t current = i * stride;
		size_t following = ((i + 1) % locations) * stride;

		next[current] = following;
	}

	uint32_t index = 0;

	for (size_t i = 0; i < locations; ++i)
		index = next[index];

	const size_t accesses = 20'000'000;

	auto start = std::chrono::steady_clock::now();

	for (size_t i = 0; i < accesses; ++i)
		index = next[index];

	auto end = std::chrono::steady_clock::now();

	double seconds = std::chrono::duration<double>(end - start).count();

	std::cout << "locations: " << locations << '\n';
	std::cout << "ns/access: " << seconds * 1e9 / accesses << '\n';
	std::cout << "final index: " << index << '\n';
}
