#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <stdexcept>

struct Node {
	uint64_t key;
	Node* next;

	Node(uint64_t key, Node* next) : key(key), next(next) {}	
};

class ChainingHashTable {
private:
	std::vector<Node*> buckets;

	static uint64_t hash(uint64_t x) {
		x += 0x9e3779b97f4a7c15ULL;
		x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
		x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
		return x ^ (x >> 31);
	}

public:
	explicit ChainingHashTable(std::size_t bucket_count) : buckets(bucket_count, nullptr) {}
	
	~ChainingHashTable() {
		for (Node* head : buckets) {
			while (head != nullptr) {
				Node* next = head->next;
				delete head;
				head = next;
			}
		}
	}

	void insert(uint64_t key) {
		std::size_t index = hash(key) % buckets.size();
		buckets[index] = new Node(key, buckets[index]);
	}

	bool find(uint64_t key) const {
		std::size_t index = hash(key) % buckets.size();

		Node* current = buckets[index];

		while (current != nullptr) {
			if (current->key == key)
				return true;

			current = current->next;
		}

		return false;
	}

	std::size_t bucket_count() const {
		return buckets.size();
	}
};

class LinearProbingHashTable {
private:
	static constexpr uint64_t EMPTY = UINT64_MAX;

	std::vector<uint64_t> slots;

	static uint64_t hash(uint64_t x) {
	        x += 0x9e3779b97f4a7c15ULL;
        	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
	        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        	return x ^ (x >> 31);
	};

public:
	explicit LinearProbingHashTable(std::size_t slot_count) : slots(slot_count, EMPTY) {}

	void insert(uint64_t key) {
		std::size_t index = hash(key) % slots.size();

		while (slots[index] != EMPTY) {
			++index;

			if (index == slots.size())
				index = 0;
		}

		slots[index] = key;
	}

	bool find(uint64_t key) const {
		std::size_t index = hash(key) % slots.size();

		while (slots[index] != EMPTY) {
			if (slots[index] == key)
				return true;

			++index;

			if (index == slots.size())
				index = 0;
		}

		return false;
	}

	std::size_t slot_count() const {
		return slots.size();
	}
};

struct RobinHoodSlot {
	uint64_t key;
	uint32_t distance;
}; 

class RobinHoodHashTable {
private:
	static constexpr uint64_t EMPTY = UINT64_MAX;

	std::vector<RobinHoodSlot> slots;

	static uint64_t hash(uint64_t x) {
		x += 0x9e3779b97f4a7c15ULL;
		x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
		x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
		return x ^ (x >> 31);
	}	

public:
	explicit RobinHoodHashTable(std::size_t slot_count) : slots(slot_count, {EMPTY, 0}) {}

	void insert(uint64_t key) {
		std::size_t index = hash(key) % slots.size();

		RobinHoodSlot incoming {key, 0};

		while (true) {
			RobinHoodSlot& current = slots[index];

			if (current.key == EMPTY) {
				current = incoming;
				return;
			}

			if (current.key == key)
				return;
				
			if (current.distance < incoming.distance)
				std::swap(current, incoming);

			++incoming.distance;
			++index;

			if (index == slots.size())
				index = 0;
		}
	}

	bool find(uint64_t key) const {
		std::size_t index = hash(key) % slots.size();
		uint32_t distance = 0;

		while (true) {
			const RobinHoodSlot& current = slots[index];

			if (current.key == EMPTY)
				return false;

			if (distance > current.distance)
				return false;

			if (current.key == key)
				return true;
			
			++distance;
			++index;

			if (index == slots.size())
				index = 0;
		}
	}

	std::size_t slot_count() const {
		return slots.size();
	}
};

std::vector<uint64_t> generate_queries(std::size_t query_count, std::size_t n, const std::string& query_type) {
	std::vector<uint64_t> queries;
	queries.reserve(query_count);

	if (query_type == "hit") {
		for (std::size_t i = 0; i < query_count; ++i)
			queries.push_back(2 * (i % n));	
	}
	else if (query_type == "miss") {
		for (std::size_t i = 0; i < query_count; ++i)
			queries.push_back(2 * (i % n) + 1);
	}
	else if (query_type == "mixed") {
		std::size_t hits = query_count / 2;

		for (std::size_t i = 0; i < hits; ++i)
			queries.push_back(2 * (i % n));

		for (std::size_t i = hits; i < query_count; ++i)
			queries.push_back(2 * ((i - hits) % n) + 1);
	}
	else {
		throw std::invalid_argument("Unknown query type");
	}

	std::mt19937_64 rng(42);
	std::shuffle(queries.begin(), queries.end() , rng);

	return queries;
}

int main(int argc, char* argv[]) {
	if (argc != 6) {
		std::cerr << "Usage: " << argv[0] << " implementation N queries load_factor query_type\n";
		return 1;
	}

	std::string implementation = argv[1];
	std::size_t n = std::stoull(argv[2]);
	std::size_t query_count = std::stoull(argv[3]);
	double load_factor = std::stod(argv[4]);
	std::string query_type = argv[5];

	std::size_t capacity = static_cast<std::size_t>(n / load_factor) + 1;

	if (implementation == "chaining") {
		ChainingHashTable table(capacity);
			
		for (std::size_t i = 0; i < n; ++i)
			table.insert(2 * i);

		std::vector<uint64_t> queries = generate_queries(query_count, n, query_type);

		std::size_t found = 0;
			
		auto start = std::chrono::steady_clock::now();

		for (uint64_t key : queries)
			found += table.find(key);
		
		auto end = std::chrono::steady_clock::now();

		double seconds = std::chrono::duration<double>(end - start).count();
		double ns_per_query = seconds * 1e9 / query_count;

		std::cout << "implementation: chaining\n";
		std::cout << "N: " << n << '\n';
		std::cout << "capacity: " << table.bucket_count() << '\n';
		std::cout << "load factor: " << static_cast<double>(n) / table.bucket_count() << '\n';
		std::cout << "queries: " << query_count << '\n';
		std::cout << "query type: " << query_type << '\n';
		std::cout << "time: " << seconds << " s\n";
		std::cout << "ns/query: " << ns_per_query << '\n';
		std::cout << "found: " << found << '\n';
	}
	else if (implementation == "linear") {
		LinearProbingHashTable table(capacity);
			
		for (std::size_t i = 0; i < n; ++i)
			table.insert(2 * i);

		std::vector<uint64_t> queries = generate_queries(query_count, n, query_type);

		std::size_t found = 0;
			
		auto start = std::chrono::steady_clock::now();

		for (uint64_t key : queries)
			found += table.find(key);
		
		auto end = std::chrono::steady_clock::now();

		double seconds = std::chrono::duration<double>(end - start).count();
		double ns_per_query = seconds * 1e9 / query_count;

		std::cout << "implementation: linear\n";
		std::cout << "N: " << n << '\n';
		std::cout << "capacity: " << table.slot_count() << '\n';
		std::cout << "load factor: " << static_cast<double>(n) / table.slot_count() << '\n';
		std::cout << "queries: " << query_count << '\n';
		std::cout << "query type: " << query_type << '\n';
		std::cout << "time: " << seconds << " s\n";
		std::cout << "ns/query: " << ns_per_query << '\n';
		std::cout << "found: " << found << '\n';

	}
	else if (implementation == "robin_hood") {
		RobinHoodHashTable table(capacity);

		for (std::size_t i = 0; i < n; ++i)
			table.insert(2 * i);
		
		std::vector<uint64_t> queries = generate_queries(query_count, n, query_type);	

		std::size_t found = 0;

		auto start = std::chrono::steady_clock::now();

		for (uint64_t key : queries)
			found += table.find(key);

		auto end = std::chrono::steady_clock::now();

		double seconds = std::chrono::duration<double>(end - start).count();
		double ns_per_query = seconds * 1e9 / query_count;

		std::cout << "implementation: robin_hood\n";
		std::cout << "N: " << n << '\n';
		std::cout << "capacity: " << table.slot_count() << '\n';
		std::cout << "load factor: " << static_cast<double>(n) / table.slot_count() << '\n';
		std::cout << "queries: " << query_count << '\n';
		std::cout << "query type: " << query_type << '\n';
		std::cout << "time: " << seconds << " s\n";
		std::cout << "ns/query: " << ns_per_query << '\n';
		std::cout << "found: " << found << '\n';		
	}
	else {
		std::cerr << "Unknown implementation\n";
		return 1;
	}
}























