#include <iostream>
#include <vector>

int main() {
	const size_t n = 1 << 26;
	const size_t stride = 16;
	
	std::vector<int> a(n, 1);

	long long sum = 0;

	for (size_t i = 0; i < n; i += stride)
		sum += a[i];

	std::cout << sum << '\n';
}
