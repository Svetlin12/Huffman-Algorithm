#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <queue>
#include <time.h>
using namespace std;

struct node {
    char data;
    int frequency, depth = 0;
    node* left, * right;

    node(int frequency, char data = '\0', node* left = nullptr, node* right = nullptr) : data(data), frequency(frequency), left(left), right(right) {}
};

class CompareFrequencies {
public:
    bool operator() (node* node1, node* node2) {
        if (node1->frequency == node2->frequency)
            return node1->data > node2->data;
        return node1->frequency > node2->frequency;
    }
};

void getFreq(unordered_map<char, int>& freq, priority_queue<node*, vector<node*>, CompareFrequencies>& minFreq) {
    for (auto kvp : freq) {
        node* newNode = new node(kvp.second, kvp.first);
        minFreq.push(newNode);
    }
}

void calculateFrequenciesFromString(string input, unordered_map<char, int>& freq) {
    for (auto ch : input) {
        freq[ch]++;
    }
}

class HuffmanTree {
private:

    string tree, serialized;
    node* root;
    unordered_map<char, string> encodingTable;

    void buildHuffmanTree(priority_queue<node*, vector<node*>, CompareFrequencies>& minFreq) {
        if (minFreq.size() == 0)
            return;

        while (minFreq.size() != 1) {
            node* first = minFreq.top();
            minFreq.pop();

            node* second = minFreq.top();
            minFreq.pop();

            node* combined = new node(first->frequency + second->frequency);
            combined->depth = max(first->depth, second->depth) + 1;
            combined->left = first;
            combined->right = second;

            minFreq.push(combined);
        }

        root = minFreq.top();
    }

    void dfsPrint(node* curr) {
        if (curr != nullptr) {
            dfsPrint(curr->left);
            cout << curr->frequency << " ";
            dfsPrint(curr->right);
        }
    }

    void buildEncodingTableDFSTraversal(node* curr, string& code) {
        if (curr == nullptr)
            return;

        if (curr->left == nullptr && curr->right == nullptr) {
            encodingTable[curr->data] = code;
            return;
        }


        code.push_back('0');
        buildEncodingTableDFSTraversal(curr->left, code);
        code.pop_back();
        code.push_back('1');
        buildEncodingTableDFSTraversal(curr->right, code);
        code.pop_back();
    }

    void buildEncodingTable() {
        string code = "";
        code.reserve(root->depth + 1);
        buildEncodingTableDFSTraversal(root, code);
    }

    void serialize() {
        queue<node*> q;
        q.push(root);
        while (!q.empty()) {
            node* curr = q.front();
            q.pop();
            if (curr == nullptr) {
                serialized.append("-1");
                serialized.push_back(' ');
                continue;
            }
            serialized.append(to_string(curr->frequency));
            serialized.push_back(' ');
            if (curr->data) {
                serialized.append("-1 -1 ");
                serialized.push_back(curr->data);
                serialized.push_back(' ');
            }
            else {
                q.push(curr->left);
                q.push(curr->right);
            }
        }
    }

    node* deserialize() {
        if (serialized.empty() || serialized[0] == 'N')
            return nullptr;

        string temp = "";
        queue<pair<int, char>> nodeVals;
        for (int i = 0; i < serialized.size(); i++) {
            if (serialized[i] == ' ') {
                nodeVals.push(pair<int, char>(stoi(temp), '\0'));
                temp = "";
                continue;
            }
            else if (i + 6 < serialized.size() && serialized[i] == '-' && serialized[i+1] == '1' && serialized[i+3] == '-' && serialized[i+4] == '1') {
                nodeVals.back().second = serialized[i + 6];
                i += 7;
                continue;
            }
            temp += serialized[i];
        }

        node* newRoot = new node(nodeVals.front().first, nodeVals.front().second);
        nodeVals.pop();
        queue<node*> q;
        q.push(newRoot);
        while (!q.empty() && !nodeVals.empty()) {
            node* curr = q.front();
            q.pop();

            pair<int, char> currChild = nodeVals.front();
            nodeVals.pop();
            if (currChild.first != -1) {
                curr->left = new node(currChild.first, currChild.second);

                if (curr->left->data == '\0') {
                    q.push(curr->left);
                }
            }

            currChild = nodeVals.front();
            nodeVals.pop();
            if (currChild.first != -1) {
                curr->right = new node(currChild.first, currChild.second);

                if (curr->right->data == '\0') {
                    q.push(curr->right);
                }
            }
        }

        return newRoot;
    }

    void restoreInitialStringHelper(string& tree, node* root, string& restored) {
        node* traverse = root;
        int i = 0;
        while (i < tree.size()) {
            if (tree[i++] == '0') {
                traverse = traverse->left;
            }
            else {
                traverse = traverse->right;
            }

            if (traverse->data != '\0') {
                restored.push_back(traverse->data);
                traverse = root;
            }
        }
    }

public:

    HuffmanTree(priority_queue<node*, vector<node*>, CompareFrequencies> minFreq) {
        root = nullptr;
        buildHuffmanTree(minFreq);
        buildEncodingTable();
    }

    void print() {
        dfsPrint(root);
        cout << "depth: " << root->depth << endl;
    }

    void printTable() {
        for (auto kvp : encodingTable) {
            cout << kvp.first << "->" << kvp.second << endl;
        }
    }

    string compressString(string& toCompress) {
        string compressed = "";
        compressed.reserve(toCompress.size() * root->depth);

        for (auto ch : toCompress) {
            compressed.append(encodingTable[ch]);
        }

        tree = compressed;

        if (compressed.size() % 8 != 0) {
            do {
                compressed.push_back('0');
            } while (compressed.size() % 8 != 0);
        }

        return compressed;
    }

    string convertByteOutputToNumbers(string& toConvert) {
        string converted = "";
        int optimalSize = (toConvert.size() % 8) * 4;
        converted.reserve(optimalSize);

        int counter = 0;
        int byteNumbers[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
        int currNum = 0;
        for (char ch : toConvert) {
            if (ch == '1') {
                currNum += byteNumbers[counter];
            }

            counter++;
            if (counter == 8) {
                converted += to_string(currNum) + " ";
                currNum = 0;
                counter = 0;
            }
        }

        return converted;
    }

    void serializeTree() {
        serialize();
        cout << serialized << endl;
    }

    node* deserializeTree() {
        return deserialize();
    }

    string restoreInitialString() {
        node* newRoot = deserializeTree();
        string restored;
        restoreInitialStringHelper(tree, newRoot, restored);
        return restored;
    }

    double getDegreeOfCompression(string& initial, string& compressed) {
        double initialBytes = initial.size() * 8;
        return (compressed.size() / initialBytes) * 100;
    }
};

// TODO: move stdin/stdout from/to file
int main() {
    clock_t tStart = clock();

    //string abc = "ABRACADABRA";
    //string abc = "A13aa-bAaB-1B3a-Aaa3b-AA3333--bbaBa---aaabbBBbab--abBaab-BBB-B--";
    //string abc = "AByEcc 11ayXEz2zbbB BBBCbdd1X 22121cdbECdzzz  22bEbCcccddCECECECECECECbdb1b1d111";
    string abc = "ACAeBbCABbAbbAA";

    unordered_map<char, int> freq;
    priority_queue<node*, vector<node*>, CompareFrequencies> minFreq;
    calculateFrequenciesFromString(abc, freq);
    getFreq(freq, minFreq);

    HuffmanTree h(minFreq);
    string compressedString = h.compressString(abc);
    cout << compressedString << endl;

    string convertedString = h.convertByteOutputToNumbers(compressedString);
    cout << convertedString << endl;

    h.serializeTree();
    h.print();

    cout << abc << endl;
    cout << h.restoreInitialString() << endl;
    cout << h.getDegreeOfCompression(abc, compressedString) << "%" << endl;

    cout << "Execution time: " << (double)(clock() - tStart)/CLOCKS_PER_SEC << endl;

    return 0;
}
