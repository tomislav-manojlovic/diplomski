#include <linux/prctl.h>
#include <sys/prctl.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using AdjList = std::vector<std::vector<int>>;

struct CSRGraph {
	std::vector<int> offsets;
	std::vector<int> edges;
};

struct BFSResult {
	std::size_t visited_vertices;
	std::size_t examined_edges;
};

AdjList generate_graph(int n, int degree) {
	AdjList graph(n);

	std::mt19937 rng(42);
	std::uniform_int_distribution<int> dist(0, n - 1);
	
	for (int u = 0; u < n; ++u) {
		graph[u].reserve(degree);
		
		for (int j = 0; j < degree; ++j) {
			int v = dist(rng);

			if (v != u)
				graph[u].push_back(v);
		}
	}

	return graph;
}

CSRGraph to_csr(const AdjList& graph) {
	CSRGraph csr;
	int n = graph.size();

	csr.offsets.resize(n + 1);

	std::size_t total_edges = 0;
	for(const auto& neighbours : graph)
		total_edges += neighbours.size();

	csr.edges.reserve(total_edges);

	for (int u = 0; u < n; ++u) {
		csr.offsets[u] = static_cast<int>(csr.edges.size());

		for (int v : graph[u])
			csr.edges.push_back(v);		
	}

	csr.offsets[n] = static_cast<int>(csr.edges.size());

	return csr;
}

struct BFSWorkspace {
	std::vector<uint8_t> visited;
	std::vector<int> queue;

	explicit BFSWorkspace(std::size_t n) : visited(n), queue(n) {}
};

BFSResult bfs_adj(const AdjList& graph, int source, BFSWorkspace& workspace) {
	std::fill(workspace.visited.begin(), workspace.visited.end(), 0);

	auto& visited = workspace.visited;
	auto& queue = workspace.queue;

	std::size_t head = 0;
	std::size_t tail = 0;

	queue[tail++] = source;
	visited[source] = 1;

	std::size_t visited_vertices = 0;
	std::size_t examined_edges = 0;

	while (head < tail) {
		int u = queue[head++];
		++visited_vertices;

		for (int v : graph[u]) {
			++examined_edges;
			
			if (!visited[v]) {
				visited[v] = 1;
				queue[tail++] = v;
			}
		}
	}

	return {visited_vertices, examined_edges};
}

BFSResult bfs_csr(const CSRGraph& graph, int source, BFSWorkspace& workspace) {
	std::fill(workspace.visited.begin(), workspace.visited.end(), 0);

	auto& visited = workspace.visited;
	auto& queue = workspace.queue;

	std::size_t head = 0;
	std::size_t tail = 0;

	queue[tail++] = source;
	visited[source] = 1;
	
	std::size_t visited_vertices = 0;
	std::size_t examined_edges = 0;

	while (head < tail) {
		int u = queue[head++];
		++visited_vertices;

		for (int i = graph.offsets[u]; i < graph.offsets[u + 1]; ++i) {
			++examined_edges;

			int v = graph.edges[i];
			if (!visited[v]) {
				visited[v] = 1;
				queue[tail++] = v;
			}
		}
	}

	return {visited_vertices, examined_edges};
}

std::vector<int> bfs_reordering(const CSRGraph& graph, int source) {
	int n = static_cast<int>(graph.offsets.size()) - 1;

	std::vector<uint8_t> visited(n, 0);
	std::vector<int> queue(n);
	std::vector<int> old_to_new(n, -1);

	std::size_t head = 0;
	std::size_t tail = 0;
	
	queue[tail++] = source;
	visited[source] = 1;
	
	int next_id = 0;

	while (head < tail) {
		int u = queue[head++];

		old_to_new[u] = next_id++;

		for (int i = graph.offsets[u]; i < graph.offsets[u + 1]; ++i) {
			int v = graph.edges[i];

			if (!visited[v]) {
				visited[v] = 1;
				queue[tail++] = v;
			}
		}
	}	

	for (int u = 0; u < n; ++u) {
		if (old_to_new[u] == -1)
			old_to_new[u] = next_id++;
	}

	return old_to_new;
}

CSRGraph reorder_csr(const CSRGraph& graph, const std::vector<int>& old_to_new) {
	int n = static_cast<int>(graph.offsets.size()) - 1;

	std::vector<int> new_to_old(n);

	for (int old_u = 0; old_u < n; ++old_u)
		new_to_old[old_to_new[old_u]] = old_u;

	CSRGraph reordered;
	reordered.offsets.resize(n + 1);

	reordered.edges.reserve(graph.edges.size());

	for (int new_u = 0; new_u < n; ++new_u) {
		int old_u = new_to_old[new_u];

		reordered.offsets[new_u] = static_cast<int>(reordered.edges.size());

		for (int i = graph.offsets[old_u]; i < graph.offsets[old_u + 1]; ++i) {
			int old_v = graph.edges[i];
			int new_v = old_to_new[old_v];

			reordered.edges.push_back(new_v);
		}
	}

	reordered.offsets[n] = static_cast<int>(reordered.edges.size());

	return reordered;
}

int main(int argc, char* argv[]) {
	if (argc != 6) {
		std::cerr << "Usage: " << argv[0] << " <adj|csr|csr_reordered> <vertices> <degree> <repetitions> <source>\n";
		return 1;
	}
	
	std::string implementation = argv[1];
	int n = std::stoi(argv[2]);
	int degree = std::stoi(argv[3]);
	int repetitions = std::stoi(argv[4]);
	int source = std::stoi(argv[5]);

	if (n <= 0 || degree <= 0 || repetitions <= 0 || source < 0 || source >= n) {
		std::cerr << "vertices, degree and repetitions must be positive\n";
		return 1;
	}

	prctl(PR_TASK_PERF_EVENTS_DISABLE);

	AdjList graph = generate_graph(n, degree);
	BFSWorkspace workspace(n);
	
	BFSResult result {};
	double seconds = 0.0;

	if (implementation == "adj") {
		bfs_adj(graph, source, workspace);

		auto start = std::chrono::steady_clock::now();

		prctl(PR_TASK_PERF_EVENTS_ENABLE);

		for (int r = 0; r < repetitions; ++r)
			result = bfs_adj(graph, source, workspace);

		prctl(PR_TASK_PERF_EVENTS_DISABLE);

		auto end = std::chrono::steady_clock::now();

		seconds = std::chrono::duration<double>(end - start).count();
	}
	else if (implementation == "csr") {
		CSRGraph csr = to_csr(graph);
		
		graph.clear();
		graph.shrink_to_fit();

		bfs_csr(csr, source, workspace);

		auto start = std::chrono::steady_clock::now();

		prctl(PR_TASK_PERF_EVENTS_ENABLE);

		for (int r = 0; r < repetitions; ++r)
			result = bfs_csr(csr, source, workspace);

		prctl(PR_TASK_PERF_EVENTS_DISABLE);

		auto end = std::chrono::steady_clock::now();

		seconds = std::chrono::duration<double>(end - start).count();
	}
	else if (implementation == "csr_reordered") {
		CSRGraph csr = to_csr(graph);

		graph.clear();
		graph.shrink_to_fit();

		std::vector<int> old_to_new = bfs_reordering(csr, 0);

		CSRGraph reordered = reorder_csr(csr, old_to_new);

		int reordered_source = old_to_new[source];

		csr.offsets.clear();
		csr.offsets.shrink_to_fit();

		csr.edges.clear();
		csr.edges.shrink_to_fit();

		bfs_csr(reordered, reordered_source, workspace);
		
		auto start = std::chrono::steady_clock::now();

		prctl(PR_TASK_PERF_EVENTS_ENABLE);
		
		for (int r = 0; r < repetitions; ++r)
			result = bfs_csr(reordered, reordered_source, workspace);

		prctl(PR_TASK_PERF_EVENTS_DISABLE);

		auto end = std::chrono::steady_clock::now();

		seconds = std::chrono::duration<double>(end - start).count();	
	}
	else {
		std::cerr << "Unknown implementation: " << implementation << '\n';
		return 1;
	}

	std::size_t total_examined_edges = result.examined_edges * static_cast<std::size_t>(repetitions);

	double ns_per_edge = total_examined_edges == 0 ? 0.0 : seconds * 1e9 / static_cast<double>(total_examined_edges);

	double time_per_run = seconds / static_cast<double>(repetitions);

	std::cout << "implementation: " << implementation << '\n';
	std::cout << "vertices: " << n << '\n';
	std::cout << "degree: " << degree << '\n';
	std::cout << "repetitions: " << repetitions << '\n';
	std::cout << "visited vertices: " << result.visited_vertices << '\n';
	std::cout << "examined edges/run: " << result.examined_edges << '\n';
	std::cout << "time: " << seconds << " s\n";
	std::cout << "time/run: " << time_per_run << " s\n";
	std::cout << "ns/edge: " << ns_per_edge << '\n';

	return 0;
}























