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

// log base 2 of an unsigned integer, rounded down
// note: there are assembly instructions to do this but inline assembly sucks...
inline unsigned long int log2(unsigned long int x) {
    unsigned long int l = 0;
    if (x == 0) return std::numeric_limits<unsigned long int>::max();
    while(x >>= 1) ++l;
    return l;
}

// returns 2^x
inline uint32_t twopow(uint32_t x) {
    return 1 << x;
}

// create a bitmask where the last x bits are 1
// for example: lsbmask(5) produces 00...00011111 in binary
inline unsigned long int lsbmask(unsigned long int x) {
    return (1 << x) - 1;
}


class cacheAssoc
{
	unsigned BSize, Assoc;
public:
	cacheAssoc(unsigned BSize, unsigned Assoc) : BSize(BSize), Assoc(Assoc) {}

	cacheAssoc(const cacheAssoc & cp) : cacheAssoc(cp.BSize, cp.Assoc) {}

	// read the given address
	// if successful, return true
	// if not in this cache association, return false
	bool readAddress(unsigned long int tag, unsigned long int offset, unsigned long int & data) {
		// TO DO
		data = /* TO DO */;
	}

	// write to the given address
	// if successful, return true
	// if not in this cache association, return false
	bool writeAddress(unsigned long int tag, unsigned long int offset, unsigned long int data) {
		// TO DO
	}

	// add a block to this cache association
	// address = base address (offset = 0) of the added block
	// data = the data in the block
	//
	// if you had to evict a block to add this one, then:
	// 1. evict the least recently used block (up to you to track it!)
	// 2. write the evicted block's identifier into evicted_address
	// 3. return a vector containing the data in the evicted line
	//
	// if you didn't have to evict a block, return an empty vector
	std::vector<unsigned long int> addBlock(unsigned long int tag, std::vector<unsigned long int> line, unsigned long int & evicted_tag) {
		// TO DO
	}
};

class LevelCache
{
	unsigned BSize, Assoc, LSize, numOfSets;

	std::vector<cacheAssoc> sets;

	struct AddressParts
    {
        unsigned long int tag;
        unsigned long int set;
        unsigned long int offset;
    };

    AddressParts SplitAddress(unsigned long int address) {
        AddressParts parts;

        // lowest BSize bits are the offset
        unsigned long int offset_length = BSize;
        parts.offset = address & lsbmask(offset_length);
        address >>= offset_length;

        // next set_length bits are the set
        unsigned long int set_length = LSize - BSize - Assoc;
        parts.set = address & lsbmask(set_length);
        address >>= set_length;

        // rest of the bits are the tag
        parts.tag = address;

        return parts;
    }

    unsigned long int MergeAddress(AddressParts parts) {
    	// most significant bits are the tag
    	unsigned long int address = parts.tag;

    	// add the set to the end
    	unsigned long int set_length = LSize - BSize - Assoc;
    	address <<= set_length;
    	address |= parts.set;

    	// add the offset after that
    	address <<= BSize;
    	address |= parts.offset;

    	return address;
    }
public:
	LevelCache(unsigned BSize, unsigned Assoc, unsigned LSize)
	: BSize(BSize), Assoc(Assoc), LSize(LSize), numOfSets(twopow(LSize - BSize - Assoc)),

	sets(numOfSets, cacheAssoc(BSize, Assoc))
	{}

	// read the given address
	// if successful, return true
	// if not in this cache, return false
	bool readAddress(unsigned long int address, unsigned long int & data) {
		AddressParts parts = SplitAddress(address);
		return sets[parts.set].readAddress(parts.tag, parts.offset, data);
	}

	// write to the given address
	// if successful, return true
	// if not in this cache, return false
	bool writeAddress(unsigned long int address, unsigned long int data) {
		AddressParts parts = SplitAddress(address);
		return sets[parts.set].writeAddress(parts.tag, parts.offset, data);
	}

	// add a block to this cache
	// address = base address (offset = 0) of the added block
	// data = the data in the block
	//
	// if you had to evict a block to add this one, then:
	// 1. evict the least recently used block (up to you to track it!)
	// 2. write the evicted block's identifier into evicted_address
	// 3. return a vector containing the data in the evicted line
	//
	// if you didn't have to evict a block, return an empty vector
	std::vector<unsigned long int> addBlock(unsigned long int address, std::vector<unsigned long int> line, unsigned long int & evicted_address) {
		AddressParts parts = SplitAddress(address);
		unsigned long int evicted_tag;
		std::vector<unsigned long int> evicted_line = sets[parts.set].addBlock(parts.tag, line, evicted_tag);

		if (!evicted_line:empty()) {
			// a line has been evicted!
			parts.tag = evicted_tag;
			evicted_address = MergeAddress(parts);
		}
		return evicted_line;
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

		//
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
