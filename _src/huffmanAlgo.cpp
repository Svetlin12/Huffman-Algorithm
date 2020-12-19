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

class HuffmanTree {
private:

    node* root;
    string initialString, compressed;
    int addedBits;
    unordered_map<char, string> encodingTable;

    void buildHuffmanTree() {  
        unordered_map<char, int> freq;
        priority_queue<node*, vector<node*>, CompareFrequencies> minFreq;

        for (auto ch : initialString) {
            freq[ch]++;
        }

        for (auto kvp : freq) {
            node* newNode = new node(kvp.second, kvp.first);
            minFreq.push(newNode);
        }

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

    void serialize(ofstream& fileStream) {
        queue<node*> q;
        q.push(root);
        while (!q.empty()) {
            node* curr = q.front();
            q.pop();
            if (curr == nullptr) {
                fileStream << "-1 ";
                continue;
            }
            fileStream << curr->frequency << " ";
            if (curr->data) {
                fileStream << "-1 -1 " << curr->data << " ";
            }
            else {
                q.push(curr->left);
                q.push(curr->right);
            }
        }
    }

    node* deserialize(string& serialized) {
        if (serialized.empty() || serialized[0] == '-')
            return nullptr;

        string temp = "";
        queue<pair<int, char>> nodeVals;
        for (int i = 0; i < serialized.size(); i++) {
            if (serialized[i] == ' ') {
                nodeVals.push(pair<int, char>(stoi(temp), '\0'));
                temp = "";
                continue;
            }
            else if (i + 6 < serialized.size() && serialized[i] == '-' && serialized[i + 1] == '1' && serialized[i + 3] == '-' && serialized[i + 4] == '1') {
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

    void decompressStringHelper(string& tree, string& restored) {
        node* traverse = root;
        int i = 0;
        while (i < tree.size() - addedBits) {
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

    string convertByteOutputToNumbers() {
        string converted = "";
        int optimalSize = (compressed.size() % 8) * 4;
        converted.reserve(optimalSize);

        int counter = 0;
        int byteNumbers[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
        int currNum = 0;
        for (char ch : compressed) {
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

    void decimalToBits(int& num) {
        int bits[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
        for (int i = 0; i < 8; i++) {
            if (num >= bits[i]) {
                compressed.push_back('1');
                num -= bits[i];
            }
            else {
                compressed.push_back('0');
            }
        }
    }

    void convertToBits(string& decimalInput) {
        string number;
        number.reserve(3);

        // TODO: compressed.reserve()
        for (auto ch : decimalInput) {
            if (ch == ' ') {
                int num = stoi(number);
                decimalToBits(num);
                number.clear();
            }
            else {
                number.push_back(ch);
            }
        }
    }

public:

    HuffmanTree() {
        root = nullptr;
        initialString = "";
        compressed = "";
        addedBits = 0;
    }

    double getDegreeOfCompression() {
        double initialBytes = initialString.size() * 8;
        return (compressed.size() / initialBytes) * 100;
    }

    int readFromFile(string& fileName) {
        ifstream fileStream(fileName);
        
        if (!fileStream.is_open()) {
            cout << "Could not open file " << fileName << " for reading." << endl;
            return -1;
        }

        string line;
        while (getline(fileStream, line)) {
            initialString += line;
        }

        fileStream.close();

        return 0;
    }

    int readCompressedString(string& fileName, int compressionType) {
        ifstream fileStream(fileName);

        if (!fileStream.is_open()) {
            cout << "Could not open file " << fileName << " for reading." << endl;
            return -1;
        }

        string line;
        int i = 1;
        while (getline(fileStream, line)) {
            if (i == 2) {
                if (compressionType == 1) { // bits compression
                    compressed = line;
                }
                else { // decimal compression
                    convertToBits(line);
                }
            }
            else if (i == 4) {
                addedBits = stoi(line);
            }
            else if (i == 6) {
                root = deserialize(line);
            }
            i++;
        }

        fileStream.close();

        return 0;
    }

    void createHuffmanTree() {
        if (initialString.empty())
            return;

        buildHuffmanTree();
        buildEncodingTable();
    }

    void compressString() {
        if (initialString.empty())
            return;

        compressed.reserve(initialString.size() * root->depth);

        for (auto ch : initialString) {
            compressed.append(encodingTable[ch]);
        }

        if (compressed.size() % 8 != 0) {
            do {
                compressed.push_back('0');
                addedBits++;
            } while (compressed.size() % 8 != 0);
        }
    }

    void writeStringInFile(string& fileName, int compressionType) {
        ofstream fileStream(fileName);

        if (!fileStream.is_open()) {
            cout << "Could not open file " << fileName << " for writing." << endl;
            return;
        }

        if (compressionType == 1) { // bit compression
            fileStream << "Compressed string:" << endl << compressed << endl << "Added bits:" << endl << addedBits << endl << "Huffman tree:" << endl;
        }
        else { // decimal compression
            string compressedInDecimalFormat = convertByteOutputToNumbers();
            fileStream << "Compressed string:" << endl << compressedInDecimalFormat << endl << "Added bits:" << endl << addedBits << endl << "Huffman tree:" << endl;
        }

        serialize(fileStream);

        fileStream.close();
    }

    void decompressString() {
        decompressStringHelper(compressed, initialString);
    }

    string getInitialString() {
        return initialString;
    }
};


// TODO: change bit to binary
// TODO: check function names
// TODO: check if reserve is available at some strings
// TODO: testing
// TODO: implement UI
int main() {
    string inputFile, outputFile;
    cin >> inputFile;

    //string abc = "ABRACADABRA";
    //string abc = "A13aa-bAaB-1B3a-Aaa3b-AA3333--bbaBa---aaabbBBbab--abBaab-BBB-B--";
    //string abc = "AByEcc 11ayXEz2zbbB BBBCbdd1X 22121cdbECdzzz  22bEbCcccddCECECECECECECbdb1b1d111";
    //string abc = "ACAeBbCABbAbbAA";
    
    clock_t tStart = clock();
    /*
    HuffmanTree h;
    h.readFromFile(inputFile);
    h.createHuffmanTree();
    h.compressString();
    h.writeStringInFile(outputFile, 1);
    */
    
    HuffmanTree h;
    h.readCompressedString(inputFile, 2);
    h.decompressString();
    string res = h.getInitialString();
    cout << res << endl;
    cout << h.getDegreeOfCompression() << "%" << endl;

    cout << "Execution time: " << (double)(clock() - tStart) / CLOCKS_PER_SEC << endl;

    return 0;
}
