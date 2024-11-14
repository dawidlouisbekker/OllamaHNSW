#pragma once
#define SKIPLIST
#ifdef SKIPLIST


#include <thread>
#include <future>
#include <iomanip>
#include <random>
#include <mutex>
#include <queue>
#include <cmath>
#include <random>
#include <unordered_set>
#include <limits>

using namespace std;

class HNSW {
private:

	int max_level;
	struct Node {
		int id;
		float* vectorStart;
		int textLength;
		char* text;
		vector<vector<int>> neighbors;
		vector<vector<float>> distances;
		// Cleanup function
		void CleanUp() {
			// Free the dynamically allocated text
			if (text) {
				delete[] text;
				text = nullptr;
			}
		}

		Node(float* VecStart, int ID, int max_level) :  vectorStart(nullptr), textLength(0), text(nullptr), neighbors(max_level + 1) {      //NeighbourAddresses(nullptr)
			vectorStart = VecStart;
			id = ID;
		}

	};

	int m;
	std::mt19937 rng;
	int efConstruction;

	std::shared_ptr<Node> entry_point = nullptr;

	int current_level;

	std::uniform_real_distribution<> level_dist;
	int vecSize;
	int key = 0;
	std::atomic<bool> stopFlag{ false };
	std::vector<std::shared_ptr<Node>> nodes;
	std::vector<std::thread> threads;
	



	std::future<float> CalculateEuclideanNormsAsync(const float* vec, int vecSize) {
		return std::async(std::launch::async, [vec, vecSize]() {
			float result = 0.0f;
			for (int i = 0; i < vecSize; i++) {
				result += vec[i] * vec[i];
			}
			result = std::sqrt(result);  // Take the square root of the sum

			std::cout << "Computed result inside async: " << std::setprecision(50) << result << std::endl;
			cout << result;
			return result;
			});
	}

	float CalculateNode(float* query, int neighbourPos) { //Calculate cosine similarity aswell as organize level 0
		__m128 result = _mm_setzero_ps();
		cout << endl << endl;
		auto start = std::chrono::high_resolution_clock::now();
		int i = 0;
		std::future<float> vec1Future = CalculateEuclideanNormsAsync(query, vecSize);
		std::future<float> vec2Future = CalculateEuclideanNormsAsync(query, vecSize);


		for (; i + 3 < vecSize; i += 4) {
			__m128 v1 = _mm_loadu_ps(query + i);
			__m128 v2 = _mm_loadu_ps(nodes[neighbourPos]->vectorStart + i);
			result = _mm_add_ps(result, _mm_dp_ps(v1, v2, 0xF1));
		}
		float dotProduct = _mm_cvtss_f32(result);
		//cout << "dot product: " << dotProduct << endl;
		float finalRes = (dotProduct / (vec1Future.get() * vec2Future.get()));
		//cout << finalRes << endl;
		auto end = std::chrono::high_resolution_clock::now();
		cout << "result: " << finalRes << endl;
		std::chrono::duration<double> elapsed = end - start;
		return finalRes;
		//	SearchLevel()
	}



	//const std::vector<float>& query
	std::shared_ptr<Node> searchLayer(float* query, std::shared_ptr<Node> curNode, int level) {
		std::cout << "Searching layer " << level << " for node ID: " << curNode->id << std::endl;

		// Ensure the current node has the given level
		if (level >= curNode->neighbors.size()) {
			std::cerr << "Error: Level " << level << " is out of range for node ID: " << curNode->id << std::endl;
			return curNode;  // If the level is invalid, return the current node
		}

		bool improved = true;
		while (improved) {
			improved = false;
			float dist_to_current = 0;
			// Iterate over all neighbors at the current level
			for (int neighbor_id : curNode->neighbors[level]) {
				if (neighbor_id < 0 || neighbor_id >= nodes.size()) {
					std::cerr << "Invalid neighbor ID: " << neighbor_id << std::endl;
					continue;
				}

				// Compare distances to the query
				float dist_to_neighbor = CalculateNode(query, neighbor_id);   
				if (dist_to_current == 0) {
					dist_to_current = CalculateNode(query, neighbor_id);
				}
				

				// If the neighbor is closer, move to the neighbor node
				if (dist_to_neighbor < dist_to_current) {
					curNode = nodes[neighbor_id];  // Update the current node
					improved = true;
					break;  // Start from the new node
				}
			}
		}

		std::cout << "Found node ID: " << curNode->id << " at level " << level << std::endl;
		return curNode;
	}


	void searchAndConnect(std::shared_ptr<Node>& newNode, int level) {
		// Perform greedy search to find the closest node in the current level
		auto nearest = searchLayer(newNode->vectorStart, entry_point, level);
		// Connect neighbors
		auto neighbors = selectNeighbors(reinterpret_cast<uintptr_t>(nearest.get()), newNode, level, m);
		for (int n : neighbors) {
			newNode->neighbors[level].push_back(n);
			nodes[n]->neighbors[level].push_back(newNode->id);
		}
	}


	std::vector<int> selectNeighbors(int nearest, std::shared_ptr<Node> newNode, int level, int M) {
		// Select top-M nearest neighbors (pruned graph logic)
		std::vector<int> result;
		// Implement logic to select top-M closest nodes
		return result;
	}


	int getRandomLevel() {
		int level = 0;
		while (level_dist(rng) < 1.0 / log2(m) && level < max_level) {
			level++;
		}
		return level;
	}


public:



	HNSW(LPVOID* &pMap,int VecSize,int EfConstruction ,size_t fileSize, int M, int maxLevel) : m(M), efConstruction(EfConstruction), vecSize(VecSize), max_level(maxLevel), current_level(0) {
		rng.seed(std::random_device{}());
		level_dist = std::uniform_real_distribution<>(0.0, 1.0);
		cout << "vec size: " << vecSize << endl;

        int count = 0;
		bool first = false;
        //
		int pos = 0;
        cout << "file size:" << fileSize << endl;
        while (count < fileSize) {
			int level = getRandomLevel();
			cout << "generated level: " << level << endl;
			key++;
			float* vec = reinterpret_cast<float*>(reinterpret_cast<char*>(*pMap) + count);
			auto newNode = std::make_shared<Node>(vec, key, max_level);
			count = count + sizeof(float) * VecSize;
			newNode->textLength = *reinterpret_cast<int*>(reinterpret_cast<char*>(*pMap) + count);
			count = count + sizeof(int);
			newNode->text = reinterpret_cast<char*>(reinterpret_cast<char*>(*pMap) + count);
			newNode->id = key;
			count = count + sizeof(char) * newNode->textLength;

			nodes.push_back(newNode);
			cout << "round: " << key << endl;
			if (entry_point == nullptr) {
				entry_point = newNode;
				continue;
			}
			for (int l = level; l >= 0; --l) {
				searchAndConnect(newNode, l);
			}

			// Link the new node into the graph


			pos++;
        }
		return;
	}

	void DisplayNeighbors() {
		// Iterate through each node in the vector of shared_ptrs
		for (const auto& node_ptr : nodes) {
			std::cout << "Node ID: " << node_ptr->id << std::endl;

			// Iterate through each level of neighbors
			for (int level = 0; level < node_ptr->neighbors.size(); ++level) {
				std::cout << "  Level " << level << " neighbors: ";
				if (node_ptr->neighbors[level].empty()) {
					std::cout << "No neighbors at this level." << std::endl;
				}
				else {
					for (int neighbor : node_ptr->neighbors[level]) {
						std::cout << neighbor << " ";
					}
					std::cout << std::endl;
				}
			}
		}
	}

	std::vector<string> searchKNN(float* query, int k) {
		auto curNode = entry_point;

		for (int l = current_level; l >= 0; --l) {

			curNode = searchLayer(query, curNode, l);
		}
		vector<int> ids = searchLayerWithHeap(query, curNode, 0, k);
		
		vector<string> answers;
		for (int id : ids) {
			int textSize = nodes[id]->textLength;
			for (int i = 0; i < textSize; i++) {
				answers[id][i] = nodes[id]->text[i];
			}
		}
	//	vector<string> answers = {"test"};

		return answers;
	}
	

	// Function to display all neighbors for a list of shared pointers to nodes


	std::vector<int> searchLayerWithHeap(float* query, std::shared_ptr<Node> startNode, int level, int k) {
		// Multi-layer search with a priority queue
		std::priority_queue<std::pair<float, int>> candidates;
		std::unordered_set<int> visited;
		candidates.emplace(std::numeric_limits<float>::infinity(), startNode->id);

		while (!candidates.empty()) {
			auto top = candidates.top();
			candidates.pop();

			int cur = top.second;
			visited.insert(cur);

			for (int neighbor : nodes[cur]->neighbors[level]) {
				if (visited.count(neighbor) == 0) {
					float d = CalculateNode(query, neighbor);
					candidates.emplace(d, neighbor);
				}
			}
		}

		std::vector<int> result;
		for (int i = 0; i < k && !candidates.empty(); ++i) {
			result.push_back(candidates.top().second);
			candidates.pop();
		}
		return result;
	}

	void stopAllThreads() {
		stopFlag.store(true); // Optionally wait for threads to complete any final tasks 
		for (auto& t : threads) { 
			if (t.joinable()) {
				t.join(); 
			} 
		} 
	}

	~HNSW() {
		stopAllThreads();
		for (shared_ptr<Node> node : nodes) {
			node->CleanUp();
		}
		return;
	}

//	void ShowNeighbours0() {
//		for (Node* node : hnswList) {
//			node->ShowList0();
//		}
//	}

}; 

#endif 