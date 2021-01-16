#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <queue>
#include <bitset>
using namespace std;

struct node {
    char data; // '\0' if not a leaf
    int frequency, depth = 0; // depth is counted as such: 0 is the depth of the leafs and it goes up by 1
    node* left, * right;

    node(int frequency, char data = '\0', node* left = nullptr, node* right = nullptr) : data(data), frequency(frequency), left(left), right(right) {}
};

// used to sort nodes by their frequency in the priority_queue (min heap)
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
    int addedBits, inputFileSize, outputFileSize;
    unordered_map<char, string> encodingTable;

    void buildHuffmanTree() {  
        unordered_map<char, int> freq;
        priority_queue<node*, vector<node*>, CompareFrequencies> minFreq;

        // create the frequency table
        for (auto ch : initialString) {
            freq[ch]++;
        }

        // for each char and its frequency, create a node and put it in the min heap
        for (auto kvp : freq) {
            node* newNode = new node(kvp.second, kvp.first);
            minFreq.push(newNode);
        }

        // get the two topmost min nodes from the min heap and combine them into a new one and push the new one into the min heap
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

    // each left turn is a 0 and each right turn is a 1 in the code with which a character will be associated
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

    // build the encoding table (for each initial character there is a corresponding code in binary with which it will be compressed)
    void buildEncodingTable() {
        string code = "";
        code.reserve(root->depth + 1);
        buildEncodingTableDFSTraversal(root, code);
    }

    /* 
        Break down the huffman tree to save it in a file and later use it to decompress the string it compressed.
        A null child is represented as "-1". This algorithm is essentialy a BFS traversal.
    */
    void serialize(ofstream& fileStream) {
        queue<node*> q;
        q.push(root);
        int8_t leafVal = -1;
        while (!q.empty()) {
            node* curr = q.front();
            q.pop();
            if (curr == nullptr) {
                //fileStream << "-1 ";
                fileStream << +leafVal;
                continue;
            }
            fileStream << curr->frequency << " ";
            if (curr->data) {
                //fileStream << "-1 -1 " << curr->data << " ";
                fileStream << +leafVal << " " << +leafVal << " " << curr->data << " ";
            }
            else {
                q.push(curr->left);
                q.push(curr->right);
            }
        }
    }

    // reconstruct a serialized tree in order to use it to decompress a string
    node* deserialize(string& serialized) {
        if (serialized.empty() || serialized[0] == '-') // do nothing if the tree is empty
            return nullptr;

        string temp = "";
        queue<pair<int, char>> nodeVals; // pair: character's frequency, character ('\0' if internal node)
        for (int i = 0; i < serialized.size(); i++) {
            if (serialized[i] == ' ') {
                nodeVals.push(pair<int, char>(stoi(temp), '\0'));
                temp = "";
                continue;
            }
            // if the last added node is a leaf, then change its character value (this means it will be of the form: [frequency] -1 -1 [value])
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

        /* 
            connect the nodes - pop 2 values from nodeVals (left and right child) and construct nodes from these values, 
            then connect them with the firstly added node in the queue q
        */
        while (!q.empty() && !nodeVals.empty()) {
            node* curr = q.front();
            q.pop();

            pair<int, char> currChild = nodeVals.front();
            nodeVals.pop();

            // if child is not a null child
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

    /*
        go through the compressed string and traverse the Huffman tree using it's values
        0 -> to go left and 1 -> to go right
        whenever a child node is reached, start the traversal again from the root
    */
    void decompressStringHelper(string& tree) {
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
                initialString.push_back(traverse->data);
                traverse = root;
            }
        }
    }

    void writeInDecimalHelper(ofstream& fileStream, ofstream& fileForDecimalRepresentation) {
        int counter = 0;
        const int byteNumbers[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
        uint8_t currNum = 0;
        for (int i = 0; i < compressed.size(); i++) {
            if (compressed[i] == '1') {
                currNum += byteNumbers[counter];
            }

            counter++;
            if (counter == 8) {
                fileStream << currNum;
                if (i == compressed.size() - 1) {
                    fileForDecimalRepresentation << +currNum;
                }
                else {
                    fileForDecimalRepresentation << +currNum << " ";
                }
                currNum = 0;
                counter = 0;
            }
        }
    }

    void decimalNumToBinaryNum(int num) {
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

public:

    HuffmanTree() {
        root = nullptr;
        initialString = "";
        compressed = "";
        addedBits = 0;
    }

    double getDegreeOfCompression() {
        if (inputFileSize == 0) {
            return 0;
        }

        return ((double)outputFileSize / inputFileSize) * 100;
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

        ifstream stream(fileName, ifstream::ate | ifstream::binary);
        inputFileSize = stream.tellg();

        stream.close();

        return 0;
    }

    int readCompressedString(string& fileName) {
        string onlyFileName = fileName.substr(0, fileName.size() - 15); // 15 - remove _compressed.txt from the string

        string additionalFileName = onlyFileName + "_additional.txt";
        ifstream additional(additionalFileName);

        if (!additional.is_open()) {
            cout << "Could not open file " << additionalFileName << " for reading." << endl;
            return -1;
        }

        string line;
        int i = 1;
        while (getline(additional, line)) {
            if (i == 1) {
                root = deserialize(line);
            }
            else if (i == 2) {
                addedBits = stoi(line);
            }
            i++;
        }

        additional.close();

        ifstream fileStream(fileName);

        if (!fileStream.is_open()) {
            cout << "Could not open file " << fileName << " for reading." << endl;
            return -1;
        }

        char bit;
        while (fileStream.get(bit)) {
            unsigned char ch = bit;
            decimalNumToBinaryNum(ch);
        }

        fileStream.close();

        ifstream stream(fileName, ifstream::ate | ifstream::binary);
        outputFileSize = stream.tellg();

        stream.close();
        return 0;
    }

    void createHuffmanTree() {
        if (initialString.empty())
            return;

        buildHuffmanTree();
        buildEncodingTable();
    }

    // go through the initial string and for each of its characters, replace them with their corresponding code
    void compressString() {
        if (initialString.empty())
            return;

        compressed.reserve(initialString.size() * root->depth);

        for (auto ch : initialString) {
            compressed.append(encodingTable[ch]);
        }

        // if the compressed string's final byte is not of length 8, then complete it with 0s
        if (compressed.size() % 8 != 0) {
            do {
                compressed.push_back('0');
                addedBits++;
            } while (compressed.size() % 8 != 0);
        }
    }

    int writeStringInFile(string& initalFileName, int type) {       
        if (type == 1) {
            string onlyInitialName = initalFileName.substr(0, initalFileName.size() - 4); // 4 - remove .txt from the string

            string compressedFileName = onlyInitialName + "_compressed.txt";
            ofstream compressedFileStream(compressedFileName, ofstream::binary);
            if (!compressedFileStream.is_open()) {
                cout << "Could not open/create file " << compressedFileName << " for writing" << endl;
                return -1;
            }

            string additionalInfoFileName = onlyInitialName + "_additional.txt";
            ofstream additional(additionalInfoFileName);
            if (!additional.is_open()) {
                cout << "Could not open/create file " << additionalInfoFileName << " for writing" << endl;
                return -1;
            }

            string decimalRepresentationFileName = onlyInitialName + "_decimal.txt";
            ofstream decimalRepresentFileStream(decimalRepresentationFileName);
            if (!decimalRepresentFileStream.is_open()) {
                cout << "Could not open/create file " << decimalRepresentationFileName << " for writing" << endl;
                return -1;
            }

            string binaryRepresentationFileName = onlyInitialName + "_binary.txt";
            ofstream binaryRepresentFileStream(binaryRepresentationFileName);
            if (!binaryRepresentFileStream.is_open()) {
                cout << "Could not open/create file " << binaryRepresentationFileName << " for writing" << endl;
                return -1;
            }

            binaryRepresentFileStream << compressed;
            writeInDecimalHelper(compressedFileStream, decimalRepresentFileStream);

            serialize(additional);
            uint8_t bits = addedBits;
            additional << endl << +bits << endl;

            compressedFileStream.close();
            additional.close();
            decimalRepresentFileStream.close();

            ifstream stream(compressedFileName, ifstream::ate | ifstream::binary);
            outputFileSize = stream.tellg();

            stream.close();
        }
        else if (type == 2) {
            string onlyInitialName = initalFileName.substr(0, initalFileName.size() - 15); // 15 - remove _compressed.txt from the string
            string decompressedFileName = onlyInitialName + "_decompressed.txt";
            ofstream decompressedFileStream(decompressedFileName);

            if (!decompressedFileStream.is_open()) {
                cout << "Could not open/create file " << decompressedFileName << " for writing" << endl;
                return -1;
            }

            decompressedFileStream << initialString;

            decompressedFileStream.close();

            ifstream stream(decompressedFileName, ifstream::ate | ifstream::binary);
            inputFileSize = stream.tellg();

            stream.close();
        }

        return 0;
    }

    void decompressString() {
        decompressStringHelper(compressed);
    }

    string getInitialString() {
        return initialString;
    }
};

int main() {
    cout << "1. Compress from a file" << endl << "2. Decompress from a file" << endl << "Type \"1\" or \"2\" to indicate your intention:" << endl;

    string option = "";

    while (getline(cin, option) && (option != "1" && option != "2")) {
        cout << "Invalid input. Try typing only \"1\" or only \"2\":" << endl;
    }

    cout << "Type in the name of the input file (with path where needed): " << endl;
    string fileName;
    getline(cin, fileName);
    
    if (option == "1") {
        HuffmanTree h;
        if (h.readFromFile(fileName) == -1) {
            cout << "Invalid name of file. Ending process..." << endl;
            return -1;
        }

        h.createHuffmanTree();
        h.compressString();

        if (h.writeStringInFile(fileName, 1) == -1) {
            cout << "Writing is stopped" << endl;
            return -1;
        }

        cout << "Degree of compression is: " << h.getDegreeOfCompression() << "%" << endl;
    }
    else {
        HuffmanTree h;

        if (h.readCompressedString(fileName) == -1) {
            cout << "Invalid name of file. Ending process..." << endl;
            return -1;
        }

        h.decompressString();
        h.writeStringInFile(fileName, 2);
        cout << "Degree of compression is: " << h.getDegreeOfCompression() << "%" << endl;
    }

    return 0;
}
