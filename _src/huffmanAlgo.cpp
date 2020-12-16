#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <queue>
using namespace std;

struct node {
	char data;
	int frequency;
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

	node* root;
	unordered_map<char, string> encodingTable;
	int height = 0;

	void buildHuffmanTree(priority_queue<node*, vector<node*>, CompareFrequencies>& minFreq) {
		if (minFreq.size() == 0)
			return;

		while (minFreq.size() != 1) {
			node* first = minFreq.top();
			minFreq.pop();

			node* second = minFreq.top();
			minFreq.pop();

			node* combined = new node(first->frequency + second->frequency);
			combined->left = first;
			combined->right = second;

			minFreq.push(combined);
			height++;
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

	// TODO: fix code
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
	}

	void buildEncodingTable() {
		string code = "";
		code.reserve(height + 1);
		buildEncodingTableDFSTraversal(root, code);
	}

public:

	HuffmanTree(priority_queue<node*, vector<node*>, CompareFrequencies> minFreq) {
		root = nullptr;
		buildHuffmanTree(minFreq);
		buildEncodingTable();
	}

	void print() {
		dfsPrint(root);
	}

	void printTable() {
		for (auto kvp : encodingTable) {
			cout << kvp.first << "->" << kvp.second << endl;
		}
	}

	// TODO: append 0s if last byte is incomplete
	string compressString(string& toCompress) {
		string compressed = "";
		compressed.reserve(toCompress.size() * height);

		for (auto ch : toCompress) {
			compressed.append(encodingTable[ch]);
		}

		return compressed;
	}

	// TODO: fix function...problem might be in the above function...input string does not appear to contain what is expected
	string convertByteOutputToNumbers(string& toConvert) {
		string converted = "";
		int optimalSize = (toConvert.size() % 8) * 4;
		converted.reserve(optimalSize);

		int counter = 0;
		int byteNumbers[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
		int currNum = 1;
		for (char ch : toConvert) {
			if (ch == '1') {
				currNum += byteNumbers[counter];
			}

			counter++;
			if (counter == 8) {
				converted += to_string(currNum) + " ";
				currNum = 1;
				counter = 0;
			}
		}

		return converted;
	}
};

int main() {

	string abc = "ABRACADABRA";
	unordered_map<char, int> freq;
	priority_queue<node*, vector<node*>, CompareFrequencies> minFreq;
	calculateFrequenciesFromString(abc, freq);
	getFreq(freq, minFreq);

	HuffmanTree h(minFreq);

	h.print();
	cout << endl;
	h.printTable();

	string compressedString = h.compressString(abc);
	cout << compressedString << endl;

	string convrtedString = h.convertByteOutputToNumbers(compressedString);
	cout << convrtedString << endl;

	return 0;
}
