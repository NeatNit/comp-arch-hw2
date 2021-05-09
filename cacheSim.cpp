/* 046267 Computer Architecture - Spring 2021 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using std::map;
using std::vector;

class cacheAssoc
{
	unsigned BSize, Assoc;
public:
	cacheAssoc(unsigned BSize, unsigned Assoc) : BSize(BSize), Assoc(Assoc) {}

	cacheAssoc(const cacheAssoc & cp) : cacheAssoc(cp.BSize, cp.Assoc) {}

	// read the given address
	// if successful, return true
	// if not in this cache association, return false
	bool readAddress(unsigned long int block_identifier, unsigned long int offset, unsigned long int & data) {
		// TO DO
		data = /* TO DO */;
	}

	// write to the given address
	// if successful, return true
	// if not in this cache association, return false
	bool writeAddress(unsigned long int block_identifier, unsigned long int offset, unsigned long int data) {
		// TO DO
	}

	// add a block to this cache association
	// address = base address (offset = 0) of the added block
	// data = the data in the block
	//
	// if you had to evict a block to add this one, then:
	// 1. evict the least recently used block (up to you to track it!)
	// 2. write the evicted block's identifier into evicted_address
	// 3. return a vector containing the data in the
	//
	// if you didn't have to evict a block, return an empty vector
	std::vector<unsigned long int> addBlock(unsigned long int block_identifier, std::vector<unsigned long int> data, unsigned long int & evicted_identifier) {
		// TO DO
	}
};

class cacheSim
{
	unsigned MemCyc, BSize, L1Size, L2Size, L1Assoc,
		L2Assoc, L1Cyc, L2Cyc, WrAlloc;

	std::map<unsigned long int, unsigned long int> main_memory;

	std::vector<cacheAssoc> L1Cache, L2Cache;

public:
	cacheSim(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Assoc,
		unsigned L2Assoc, unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc)
		: MemCyc(MemCyc), BSize(BSize), L1Size(L1Size), L2Size(L2Size), L1Assoc(L1Assoc),
		L2Assoc(L2Assoc), L1Cyc(L1Cyc), L2Cyc(L2Cyc), WrAlloc(WrAlloc)
		{

		}

};


int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

	}

	double L1MissRate;
	double L2MissRate;
	double avgAccTime;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
