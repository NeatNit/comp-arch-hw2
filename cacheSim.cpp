/* 046267 Computer Architecture - Spring 2021 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <stdexcept>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using std::map;
using std::vector;
using std::logic_error;

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


class cacheAssocSet
{
	unsigned Assoc;

	// your data structure(s) here!
	// don't forget to initialize them in the constructor
public:
	cacheAssocSet(unsigned Assoc) : Assoc(Assoc) {}

	cacheAssocSet(const cacheAssocSet & cp) : cacheAssocSet(cp.Assoc) {} // don't change this line - it's a copy constructor used in vector fill constructors

	// Is the given tag in the set?
	bool isTagInSet(unsigned long int tag) {
		// TO DO
	}

	// Evict the specified tag from the set
	// If the tag was not in the tag in the first place, do nothing and return false
	// If it was evicted, return true
	bool evictTag(unsigned long int tag) {
		// TO DO
	}

	// Add a tag to this set
	// if the set is full, you must evict another tag. In that case:
	// 1. evict the least recently used tag (up to you to track it! you can use the method shown in the lecture, or whatever other method you can think of)
	// 2. set the output value: evicted_tag = <tag you had to evict>
	// 3. return true
	//
	// if you didn't have to evict a tag, return false
	bool addTag(unsigned long int tag, unsigned long int & evicted_tag) {
		// TO DO
	}
};

class LevelCache
{
	unsigned BSize, Assoc, LSize, numOfSets;

	std::vector<cacheAssocSet> sets;

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

	sets(numOfSets, cacheAssocSet(BSize, Assoc))
	{}

	// Is the block containing the given address in the cache?
	bool isBlockInCache(unsigned long int address) {
		AddressParts parts = SplitAddress(address);
		return sets[parts.set].isTagInSet(parts.tag);
	}

	// Evict the block containing the given address from the cache
	// If the block was not in the cache in the first place, do nothing and return false
	// If it was evicted, return true
	bool evictBlock(unsigned long int address) {
		AddressParts parts = SplitAddress(address);
		return sets[parts.set].evictTag(parts.tag);
	}

	// Add the block containing the given address to this cache
	// Returns whether a block had to be evicted
	// If so, the base address of that block will be written to evicted_block
	bool addBlock(unsigned long int address, unsigned long int & evicted_block) {
		AddressParts parts = SplitAddress(address);
		unsigned long int evicted_tag;
		bool evicted = sets[parts.set].addTag(parts.tag, evicted_tag);

		if (evicted) {
			// a block has been evicted!
			parts.tag = evicted_tag;
			evicted_block = MergeAddress(parts);
		}

		return evicted;
	}
};

class CacheSim
{
	unsigned MemCyc, BSize, L1Size, L2Size, L1Assoc,
		L2Assoc, L1Cyc, L2Cyc, WrAlloc;

	// caches
	LevelCache L1Cache, L2Cache;

	// stats
	unsigned long int L1Total, L1Miss, L2Total, L2Miss;
public:
	CacheSim(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Assoc,
		unsigned L2Assoc, unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc)
		: MemCyc(MemCyc), BSize(BSize), L1Size(L1Size), L2Size(L2Size), L1Assoc(L1Assoc),
		L2Assoc(L2Assoc), L1Cyc(L1Cyc), L2Cyc(L2Cyc), WrAlloc(WrAlloc),

		// L1 and L2 caches
		L1Cache(BSize, L1Assoc, L1Size),
		L2Cache(BSize, L2Assoc, L2Size),

		L1Total(0), L1Miss(0), L2Total(0), L2Miss(0)
		{}

	void getStats(unsigned long int & L1Total, unsigned long int & L1Miss, unsigned long int & L2Total, unsigned long int & L2Miss) {
		L1Total = this->L1Total;
		L1Miss = this->L1Miss;

		L2Total = this->L2Total;
		L2Miss = this->L2Miss;
	}

	// simulate a read from an address, return the number of cycles it took
	unsigned long int readAddress(unsigned long int address) {
		++L1Total;

		unsigned long int evicted_block;
		bool evicted;
		unsigned long int cyc = L1Cyc;

		if (L1Cache.isBlockInCache(address)) {
			// L1 HIT
			// sanity check - this would not be checked in hardware
			if (!L2Cache.isBlockInCache(address)) throw logic_error("Cache inclusion policy error: L1 contains a block that L2 does not");
			return cyc;
		}

		// L1 MISS
		++L1Miss;
		++L2Total;
		cyc += L2Cyc;

		if (L2Cache.isBlockInCache(address)) {
			// L2 HIT

			// must add the block to L1Cache
			evicted = L1Cache.addBlock(address, evicted_block);

			if (evicted) {
				// just some sanity checking
				// can think of this as simulating the write-back
				if (!L2Cache.isBlockInCache(evicted_block)) throw logic_error("Cache inclusion policy error: L1 evicted a block that wasn't in L2");
			}

			return cyc;
		}

		// L2 MISS
		++L2Miss;
		cyc += MemCyc;

		// read block from memory, and add it to the L2Cache
		evicted = L2Cache.addBlock(address, evicted_block);
		if (evicted) {
			// evict from L1 as well, to maintain the inclusion policy
			L1Cache.evictBlock(evicted_block); // don't care about return value (we would care if there were more levels below)
		}

		// now add to L1
		evicted = L1Cache.addBlock(address, evicted_block);

		if (evicted) {
			// just some sanity checking
			// can think of this as simulating the write-back
			if (!L2Cache.isBlockInCache(evicted_block)) throw logic_error("Cache inclusion policy error: L1 evicted a block that wasn't in L2");
		}

		return cyc;
	}

	// simulate a write to an address, return the number of cycles it took
	unsigned long int writeAddress(unsigned long int address) {
		++L1Total;

		unsigned long int evicted_block;
		bool evicted;
		unsigned long int cyc = L1Cyc;

		if (L1Cache.isBlockInCache(address)) {
			// L1 HIT
			// sanity check - just code debug, this is not simulating anything real
			if (!L2Cache.isBlockInCache(address)) throw logic_error("Cache inclusion policy error: L1 contains a block that L2 does not");
			return cyc;
		}

		// L1 MISS
		++L1Miss;
		++L2Total;
		cyc += L2Cyc;

		if (L2Cache.isBlockInCache(address)) {
			// L2 HIT

			if (WrAlloc) {
				// must add the block to L1Cache
				evicted = L1Cache.addBlock(address, evicted_block);

				if (evicted) {
					// just some sanity checking
					// can think of this as simulating the write-back
					if (!L2Cache.isBlockInCache(evicted_block)) throw logic_error("Cache inclusion policy error: L1 evicted a block that wasn't in L2");
				}
			}

			return cyc;
		}

		// L2 MISS
		++L2Miss;
		cyc += MemCyc;

		if (WrAlloc) {
			// read block from memory, and add it to the L2Cache
			evicted = L2Cache.addBlock(address, evicted_block);
			if (evicted) {
				// evict from L1 as well, to maintain the inclusion policy
				L1Cache.evictBlock(evicted_block); // don't care about return value (we would care if there were more levels below)
			}

			// now add to L1
			evicted = L1Cache.addBlock(address, evicted_block);

			if (evicted) {
				// just some sanity checking
				// can think of this as simulating the write-back
				if (!L2Cache.isBlockInCache(evicted_block)) throw logic_error("Cache inclusion policy error: L1 evicted a block that wasn't in L2");
			}
		}

		return cyc;
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

	CacheSim sim(MemCyc, BSize, L1Size, L2Size, L1Assoc, L2Assoc, L1Cyc, L2Cyc, WrAlloc);

	unsigned long int totalAccesses = 0, totalCyc = 0;
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

		if (operation == 'r') {
			totalCyc += sim.readAddress(address);
		} else if (operation == 'w') {
			totalCyc += sim.writeAddress(address);
		}
		++totalAccesses;
	}

	unsigned long int L1Total, L1Miss, L2Total, L2Miss;
	sim.getStats(L1Total, L1Miss, L2Total, L2Miss);

	double L1MissRate = L1Miss / L1Total;
	double L2MissRate = L2Miss / L2Total;
	double avgAccTime = totalCyc / totalAccesses;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
