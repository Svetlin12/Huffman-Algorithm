#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <queue>
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

    node* root; // root of huffman tree
    string initialString, compressed; // initialString - place to store the string that is in the input file; compressed - store the compressed string
    int addedBits, inputFileSize, outputFileSize; // in order to decompress the string we need to know the added 0s at the end of the binary representation we get from the compressed file; in the other variables we store the sizes of the output and input file in order to get the decompression percentage
    unordered_map<char, string> encodingTable; // in this unordered map we store the binary representation of each character that we get from the huffman tree

    void buildHuffmanTree() {  
        unordered_map<char, int> freq; // char - its frequency in the string
        priority_queue<node*, vector<node*>, CompareFrequencies> minFreq; // min heap which will store nodes and will be sorted by their frequency

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
        code.reserve(root->depth + 1); // the maximum length of a code will be the height (or depth in this case) of the tree (we add 1 because of the end of string character)
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
            if (curr == nullptr) { // write -1 representing child of leaf node
                fileStream << +leafVal;
                continue;
            }
            fileStream << curr->frequency << " ";
            if (curr->data) {
                fileStream << +leafVal << " " << +leafVal << " ";
                if (curr->data == '\n') { // if the character is newline, write the char as '\n' rather than an actual newline because then it will bug the decomposition (it expects one line of huffman tree)
                    fileStream << "\\n ";
                }
                else {
                    fileStream << curr->data << " ";
                }
            }
            else {
                // always add left and right child
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
            // if space is met, then a node's frequency has been read fully
            if (serialized[i] == ' ') {
                nodeVals.push(pair<int, char>(stoi(temp), '\0'));
                temp = ""; // reset temp for next node
                continue;
            }
            // if the last added node is a leaf, then change its character value (this means it will be of the form: [frequency] -1 -1 [value])
            else if (i + 6 < serialized.size() && serialized[i] == '-' && serialized[i + 1] == '1' && serialized[i + 3] == '-' && serialized[i + 4] == '1') {
                if (i + 7 < serialized.size() && serialized[i + 6] == '\\' && serialized[i + 7] == 'n') {
                    nodeVals.back().second = '\n';
                    i += 8;
                }
                else {
                    nodeVals.back().second = serialized[i + 6];
                    i += 7;
                }
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
        const int byteNumbers[] = { 128, 64, 32, 16, 8, 4, 2, 1 }; // use it in order to have reference of every bit value
        uint8_t currNum = 0;
        for (int i = 0; i < compressed.size(); i++) {
            if (compressed[i] == '1') {
                currNum += byteNumbers[counter];
            }

            counter++;
            if (counter == 8) {
                fileStream << currNum;
                if (i == compressed.size() - 1) {
                    fileForDecimalRepresentation << +currNum; // add + in front of the variable in order to be written in its decimal representation instead of a character
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
        int bits[] = { 128, 64, 32, 16, 8, 4, 2, 1 }; // used for reference of bit values
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

        return ((double)outputFileSize / inputFileSize) * 100; // return in percentage form (force division's result to be double, otherwise it will be interpreted as integer)
    }

    // used when the user gives file to be compressed
    int readFromFile(string& fileName) {
        ifstream fileStream(fileName);
        
        // check if file stream is opened
        if (!fileStream.is_open()) {
            cout << "Could not open file " << fileName << " for reading." << endl;
            return -1;
        }

        string line;
        while (getline(fileStream, line)) {
            if (initialString.size() != 0) { // add newline after the first getline
                initialString += "\n";
            }
            initialString += line;
        }

        fileStream.close();

        ifstream stream(fileName, ifstream::ate | ifstream::binary); // open the file stream at the end of the file because we want to get the file size and open the stream in binary mode
        inputFileSize = stream.tellg(); // returns the position of the input sequence - meaning that it will return the file size since the current position is at the end

        stream.close();

        return 0;
    }

    // used when the user gives a file to be decompressed
    int readCompressedString(string& fileName) {
        string onlyFileName = fileName.substr(0, fileName.size() - 15); // 15 - remove _compressed.txt from the string

        string additionalFileName = onlyFileName + "_additional.txt"; // the name of the file where we will get the huffman tree and added 0s count
        ifstream additional(additionalFileName);

        if (!additional.is_open()) {
            cout << "Could not open file " << additionalFileName << " for reading." << endl;
            return -1;
        }

        string line;
        int i = 1;
        /*
        * the first line will contain the serialized huffman tree
        * the next will contain an integer representing the number of 0s added in the binary representation of the compressed string
        */
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

        ifstream fileStream(fileName, ios::binary | ios::in); // open the input file stream in binary mode because we will need to read the file bit by bit

        if (!fileStream.is_open()) {
            cout << "Could not open file " << fileName << " for reading." << endl;
            return -1;
        }

        char bit;
        while (fileStream.get(bit)) {
            unsigned char ch = bit;
            decimalNumToBinaryNum(ch); // ch is converted to int because the parameter of the function is of type int, so we want to convert the decimal representation of the bit to a binary one
        }

        fileStream.close();

        ifstream stream(fileName, ifstream::ate | ifstream::binary); // get the size of the compressed file; this technique is explained in the function above
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
        // when the user has given a file to be compressed, we want to save the compressed file and its additional info
        if (type == 1) {
            string onlyInitialName = initalFileName.substr(0, initalFileName.size() - 4); // 4 - remove .txt from the string

            string compressedFileName = onlyInitialName + "_compressed.txt";
            ofstream compressedFileStream(compressedFileName, ofstream::binary); // open or create the file where will store the compressed string in binary format
            if (!compressedFileStream.is_open()) {
                cout << "Could not open/create file " << compressedFileName << " for writing" << endl;
                return -1;
            }

            string additionalInfoFileName = onlyInitialName + "_additional.txt"; // open or create the file in which we will store the huffman tree and the added 0s count
            ofstream additional(additionalInfoFileName);
            if (!additional.is_open()) {
                cout << "Could not open/create file " << additionalInfoFileName << " for writing" << endl;
                return -1;
            }

            string decimalRepresentationFileName = onlyInitialName + "_decimal.txt"; // file for decimal representation of the compressed string
            ofstream decimalRepresentFileStream(decimalRepresentationFileName);
            if (!decimalRepresentFileStream.is_open()) {
                cout << "Could not open/create file " << decimalRepresentationFileName << " for writing" << endl;
                return -1;
            }

            string binaryRepresentationFileName = onlyInitialName + "_binary.txt"; // file for binary representation of the compressed string
            ofstream binaryRepresentFileStream(binaryRepresentationFileName);
            if (!binaryRepresentFileStream.is_open()) {
                cout << "Could not open/create file " << binaryRepresentationFileName << " for writing" << endl;
                return -1;
            }

            binaryRepresentFileStream << compressed; // write the binary representation of the compressed string
            writeInDecimalHelper(compressedFileStream, decimalRepresentFileStream); // write the decimal representation of the compressed string and write the true compressed string at the same time

            serialize(additional);
            uint8_t bits = addedBits;
            additional << endl << +bits << endl; // write the additional info (serialized huffman tree and added 0s count)

            compressedFileStream.close();
            additional.close();
            decimalRepresentFileStream.close();

            ifstream stream(compressedFileName, ifstream::ate | ifstream::binary); // compute the size of the compressed file; explained in the first read function
            outputFileSize = stream.tellg();

            stream.close();
        }
        // when the user has given a file to be decompressed, we want to save the decompressed string
        else if (type == 2) {
            string onlyInitialName = initalFileName.substr(0, initalFileName.size() - 15); // 15 - remove _compressed.txt from the string
            string decompressedFileName = onlyInitialName + "_decompressed.txt";
            ofstream decompressedFileStream(decompressedFileName, ofstream::trunc); // create or open the file where we will save the decompressed string

            if (!decompressedFileStream.is_open()) {
                cout << "Could not open/create file " << decompressedFileName << " for writing" << endl;
                return -1;
            }

            decompressedFileStream << initialString;

            decompressedFileStream.close();

            // get size of output file
            ifstream stream(decompressedFileName, ifstream::ate | ifstream::binary);
            inputFileSize = stream.tellg();

            stream.close();
        }

        return 0;
    }

    void decompressString() {
        decompressStringHelper(compressed);
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
        if (h.writeStringInFile(fileName, 2) == -1) {
            cout << "Writing is stopped" << endl;
            return -1;
        }
        cout << "Degree of compression is: " << h.getDegreeOfCompression() << "%" << endl;
    }

    return 0;
}