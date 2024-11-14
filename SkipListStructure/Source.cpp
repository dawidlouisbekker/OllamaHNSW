#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <string>
#include <climits>

class SkipList {
    struct Node {
        int value;
        std::vector<Node*> forward; // Pointers for each level

        Node(int value, int level) : value(value), forward(level + 1, nullptr) {}

       void display() {
            std::cout << "Forward pointers: ";
            for (auto& ptr : forward) {
                std::cout << ptr << " ";
            }
            std::cout << std::endl;
       }



    };

    int maxLevel;               // Maximum level of skip list
    float probability;          // Probability to increase level
    int currentLevel;           // Current highest level in the list
                     // Pointer to the head node

public:
    Node* head;
    SkipList(int maxLevel, float probability)
        : maxLevel(maxLevel), probability(probability), currentLevel(0) {
        head = new Node(INT_MIN, maxLevel); // Smallest possible value at the head//std::numeric_limits<int>::min()
        std::srand(std::time(nullptr)); // Seed random number generator
    }

    ~SkipList() {
        Node* current = head;
        while (current) {
            Node* next = current->forward[0];
            delete current;
            current = next;
        }
    }

    int randomLevel() {
        int level = 0;
        while (static_cast<float>(std::rand()) / RAND_MAX < probability && level < maxLevel) {
            level++;
        }
        return level;
    }

    void insert(int value) {
        std::vector<Node*> update(maxLevel + 1, nullptr); // Track nodes that need updating
        Node* current = head;

        // Move forward through each level, updating nodes that need to point to the new node
        for (int i = currentLevel; i >= 0; i--) {
            while (current->forward[i] && current->forward[i]->value < value) {
                current = current->forward[i];
            }
            update[i] = current;
        }

        // Get random level for the new node
        int newLevel = randomLevel();

        // If new node's level is greater than the current highest level, update the head's forward pointers
        if (newLevel > currentLevel) {
            for (int i = currentLevel + 1; i <= newLevel; i++) {
                update[i] = head;
            }
            currentLevel = newLevel;
        }

        // Create the new node and adjust pointers
        Node* newNode = new Node(value, newLevel);
        for (int i = 0; i <= newLevel; i++) {
            newNode->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = newNode;
        }
    }

    bool search(int value) {
        Node* current = head;
        for (int i = currentLevel; i >= 0; i--) {
            while (current->forward[i] && current->forward[i]->value < value) {
                current = current->forward[i];
            }
        }
        current = current->forward[0];
        return current && current->value == value;
    }

    void remove(int value) {
        std::vector<Node*> update(maxLevel + 1, nullptr);
        Node* current = head;

        for (int i = currentLevel; i >= 0; i--) {
            while (current->forward[i] && current->forward[i]->value < value) {
                current = current->forward[i];
            }
            update[i] = current;
        }

        current = current->forward[0];
        if (current && current->value == value) {
            for (int i = 0; i <= currentLevel; i++) {
                if (update[i]->forward[i] != current) break;
                update[i]->forward[i] = current->forward[i];
            }
            delete current;

            // Adjust current level of the list if necessary
            while (currentLevel > 0 && head->forward[currentLevel] == nullptr) {
                currentLevel--;
            }
        }
    }

    void display() const {
        for (int i = currentLevel; i >= 0; i--) {
            Node* current = head->forward[i];
            std::cout << "Level " << i << ": ";
            while (current) {
                std::cout << current->value << " ";
                current = current->forward[i];
            }
            std::cout << "\n";
        }
    }
};

int main() {
    std::string input;
    while (true) {
        std::cout << " enter : ";
      std::getline(std::cin,input);

        SkipList* list = new SkipList(4, 0.6);  // Create a skip list with max level 4 and probability 0.5



    list->insert(3);
    list->insert(8);
    list->insert(7);
    list->insert(9);
    list->insert(12);
    list->insert(19);
    list->insert(17);
    list->insert(26);
    list->insert(21);

    list->head->display();

    std::cout << "Skip List contents:\n";
    list->display();
    for (int i = 30; i < 40; i++) {
        list->insert(i);
    }
    std::cout << std::endl;
    list->display();
    std::cout << "Enter search value: ";
    int searchValue;
    std::cin >> searchValue;
    std::cout << "Searching for " << searchValue << ": " << (list->search(searchValue) ? "Found" : "Not Found") << "\n";

    int removeValue = 19;
    std::cout << "Removing " << removeValue << " from the list.\n";
    list->remove(removeValue);

    std::cout << "Skip List after removal:\n";
    list->display();
    delete list;
    }

    return 0;
}