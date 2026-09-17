#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

bool binary_search_custom(const std::vector<int>& a, int x) {
	int left = 0;
	int right = static_cast<int>(a.size()) - 1;

	while (left <= right) {
		int mid = left + (right - left) / 2;

		if (a[mid] == x)
			return true;

		if (a[mid] < x)
			left = mid + 1;
		else
			right = mid - 1;
	}

	return false;
}

struct Node {
	int value;
	Node* left;
	Node* right;	
};

Node* build_balanced_tree(const std::vector<int>& data, int left, int right) {
	if (left > right)
		return nullptr;

	int mid = left + (right - left) / 2;

	Node* node = new Node {data[mid], nullptr, nullptr};
	
	node->left = build_balanced_tree(data, left, mid - 1);
	node->right = build_balanced_tree(data, mid + 1, right);

	return node;
}

bool bst_search(Node* root, int x) {
	Node* node = root;

	while (node != nullptr) {
		if (node->value == x)
			return true;
		
		if (x < node->value)
			node = node->left;
		else
			node = node->right;
	}

	return false;
}

void destroy_tree(Node* node) {
	if (node == nullptr)
		return;
	
	destroy_tree(node->left);
	destroy_tree(node->right);
	delete node;
}

void build_eytzinger_rec(const std::vector<int>& sorted, std::vector<int>& eytzinger, std::size_t i, std::size_t& pos) {
	if (i >= eytzinger.size())
		return;

	build_eytzinger_rec(sorted, eytzinger, 2 * i, pos);
	
	eytzinger[i] = sorted[pos++];

	build_eytzinger_rec(sorted, eytzinger, 2 * i + 1, pos);
}

std::vector<int> build_eytzinger(const std::vector<int>& sorted) {
	std::vector<int> result(sorted.size() + 1);

	std::size_t pos = 0;
	build_eytzinger_rec(sorted, result, 1, pos);

	return result;
}

bool eytzinger_search(const std::vector<int>& a, int x) {
	std::size_t i = 1;

	while (i < a.size()) {
		if (a[i] == x)
			return true;

		if (x < a[i])
			i = 2 * i;
		else
			i = 2 * i + 1;
	}

	return false;
}

bool eytzinger_search_prefetch(const std::vector<int>& a, int x) {
	std::size_t i = 1;

	while (i < a.size()) {
		std::size_t prefetch_i = 16 * i;

		if (prefetch_i < a.size())
			__builtin_prefetch(&a[prefetch_i], 0, 1);

		if (a[i] == x)
			return true;

		if (x < a[i])
			i = 2 * i;
		else
			i = 2 * i + 1;
	}

	return false;
}

int main(int argc, char* argv[]) {
	if (argc < 4) {
		std::cerr << "Usage: " << argv[0] << "<binary|bst|eytzinger|eytzinger_prefetch> <N> <queries>\n";
		return 1;
	}

	std::string mode = argv[1];
	std::size_t n = std::stoull(argv[2]);
	std::size_t query_count = std::stoull(argv[3]);

	std::vector<int> data(n);

	for (std::size_t i = 0; i < n; ++i)
		data[i] = static_cast<int>(2 * i);

	std::vector<int> queries(query_count);

	std::mt19937 rng(42);
	std::uniform_int_distribution<int> dist(0, static_cast<int>(2 * n - 1));

	for (auto& q : queries)
		q = dist(rng);

	Node* root = nullptr;

	if (mode == "bst") 
		root = build_balanced_tree(data, 0, static_cast<int>(data.size()) - 1);

	std::vector<int> eytzinger;

	if (mode == "eytzinger" || mode == "eytzinger_prefetch")
		eytzinger = build_eytzinger(data);

	std::uint64_t found = 0;

	auto start = std::chrono::steady_clock::now();

	if (mode == "binary") {
		for (int q : queries)
			found += binary_search_custom(data, q);
	}
	else if (mode == "bst") {
		for (int q : queries)
			found += bst_search(root, q);
	}
	else if (mode == "eytzinger") {
		for (int q : queries)
			found += eytzinger_search(eytzinger, q);
	}
	else if (mode == "eytzinger_prefetch") {
		for (int q : queries)
			found += eytzinger_search_prefetch(eytzinger, q);
	}
	else {
		std::cerr << "Unknown mode\n";
		return 1;
	}

	auto end = std::chrono::steady_clock::now();

	if (root != nullptr)
		destroy_tree(root);

	double seconds = std::chrono::duration<double>(end - start).count();
	double ns_per_query = seconds * 1e9 / static_cast<double>(query_count);

	std::cout << "N: " << n << '\n';
	std::cout << "size: " << n * sizeof(int) / 1024.0 << " KiB\n";
	std::cout << "queries: " << query_count << '\n';
	std::cout << "time: " << seconds << " s\n";
	std::cout << "ns/query: " << ns_per_query << '\n';
	std::cout << "found: " << found << '\n';
}

