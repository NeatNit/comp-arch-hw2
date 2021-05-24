/* 046267 Computer Architecture - Spring 2021 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <stdexcept>
#include <limits>

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
    struct Way
    {
        unsigned long int tag;
        unsigned lru_index;
        bool valid;
        bool dirty;

        Way() : tag(0), lru_index(0), valid(false), dirty(false) {}
    };

	unsigned Assoc_Size;
    vector<Way> Ways;
public:
	cacheAssocSet(unsigned Assoc_Size) : Assoc_Size(Assoc_Size), Ways(twopow(Assoc_Size)) {}

	cacheAssocSet(const cacheAssocSet& cp) : cacheAssocSet(cp.Assoc_Size) {} // don't change this line - it's a copy constructor used in vector fill constructors

	// DEBUG ONLY
	// Check whether the tag is in the set, without actually accessing it and updating LRU
	bool isTagInSet(unsigned long int tag) const {
		for (int i = 0 ; i < Ways.size(); ++i) {
            const Way &way = Ways[i];
            if (way.valid && way.tag == tag) {
                return true;
            }
		}

        return false;
	}

	// Is the given tag in the set?
	// If so, this is an access to it, so update LRU
	bool accessTagInSet(unsigned long int tag, bool dirty) {
		for (int i = 0 ; i < Ways.size(); ++i) {
            Way &way = Ways[i];
            if (way.valid && way.tag == tag) {
                // Found it! Update LRU (algorithm from lecture)
                unsigned prev_index = way.lru_index;
                way.lru_index = Ways.size() - 1;
                way.dirty |= dirty;

                for (int j = 0; j < Ways.size(); ++j)
                {
                    Way &other_way = Ways[j];
                    if (j != i && other_way.lru_index > prev_index) {
                        --other_way.lru_index;
                    }
                }
                return true;
            }
		}

        return false;
	}

	// Evict the specified tag from the set
	// If the tag was not in the set in the first place, do nothing and return false
	// If it was evicted, return true
	bool evictTag(unsigned long int tag, bool & was_dirty) {
        for (int i = 0 ; i < Ways.size(); ++i) {
            Way &way = Ways[i];
            if (way.valid && way.tag == tag) {
                // Found it! Evict, update LRU and return false
                unsigned prev_index = way.lru_index;
                was_dirty = way.dirty;

                way.valid = false;
                way.lru_index = 0;

                for (int j = 0; j < Ways.size(); ++j)
                {
                    Way &other_way = Ways[j];
                    if (j != i && other_way.lru_index < prev_index) {
                        ++other_way.lru_index;
                    }
                }
                return true;
            }
        }
        return false;
	}

	// Add a tag to this set
	// if the set is full, you must evict another tag. In that case:
	// 1. evict the least recently used tag (up to you to track it! you can use the method shown in the lecture, or whatever other method you can think of)
	// 2. set the output value: evicted_tag = <tag you had to evict>
	// 3. return true
	//
	// if you didn't have to evict a tag, return false
	bool addTag(unsigned long int tag, bool dirty, unsigned long int & evicted_tag, bool & was_dirty) {
        /* Maybe add an assert that the tag is not here */

        for (int i = 0 ; i < Ways.size(); ++i) {
            Way &way = Ways[i];
            if (way.lru_index == 0) {
                // Found our victim
                bool was_valid = way.valid;
                unsigned prev_index = way.lru_index; // always 0, but keep similar to other code

                if (was_valid) {
                	evicted_tag = way.tag;
                	was_dirty = way.dirty;
                }

                way.tag = tag;
                way.lru_index = Ways.size() - 1;
                way.valid = true;
                way.dirty = dirty;

                for (int j = 0; j < Ways.size(); ++j)
                {
                    Way &other_way = Ways[j];
                    if (j != i && other_way.lru_index > prev_index) {
                        --other_way.lru_index;
                    }
                }
                return was_valid;
            }
        }

        throw logic_error("Could not find a tag with lru_index == 0"); // there should always be at least one way with lru_index = 0
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

    AddressParts SplitAddress(unsigned long int address) const {
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

	sets(numOfSets, cacheAssocSet(Assoc))
	{}

	// DEBUG ONLY
	// Is the block containing the given address in the cache?
	// (no real access, no updating LRU)
	bool isBlockInCache(unsigned long int address) const {
		AddressParts parts = SplitAddress(address);
		return sets[parts.set].isTagInSet(parts.tag);
	}

	// Is the block containing the given address in the cache?
	// If so, access it (updates LRU accordingly)
	bool accessBlockInCache(unsigned long int address, bool dirty) {
		AddressParts parts = SplitAddress(address);
		return sets[parts.set].accessTagInSet(parts.tag, dirty);
	}

	// Evict the block containing the given address from the cache
	// If the block was not in the cache in the first place, do nothing and return false
	// If it was evicted, return true
	bool evictBlock(unsigned long int address, bool & was_dirty) {
		AddressParts parts = SplitAddress(address);
		return sets[parts.set].evictTag(parts.tag, was_dirty);
	}

	// Add the block containing the given address to this cache
	// Returns whether a block had to be evicted
	// If so, the base address of that block will be written to evicted_block
	bool addBlock(unsigned long int address, bool dirty, unsigned long int & evicted_block, bool & was_dirty) {
		AddressParts parts = SplitAddress(address);
		unsigned long int evicted_tag;
		bool evicted = sets[parts.set].addTag(parts.tag, dirty, evicted_tag, was_dirty);

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

	// helper function
	void AddToL1(unsigned long int address, bool dirty) {
		unsigned long int evicted_block;
		bool evicted, was_dirty;

		evicted = L1Cache.addBlock(address, dirty, evicted_block, was_dirty);

		if (evicted) {
			if (was_dirty) {
				// write-back!
				if (!L2Cache.accessBlockInCache(evicted_block, true)) throw logic_error("Cache inclusion policy error: L1 evicted a block that wasn't in L2");

				// NOTE: it is possible that `evicted_block` (E) ended up in the same set in L2 as `address` (A).
				// In that case, E will be more recently used than A in L2.
				// As far as I could tell, this is the desired behaviour.
				// If not, uncomment the following line to correct for it:
				//L2Cache.accessBlockInCache(address, false);
			} else {
				// no write-back, only sanity checking. just debug, not done in real hardware
				if (!L2Cache.isBlockInCache(evicted_block)) throw logic_error("Cache inclusion policy error: L1 evicted a block that wasn't in L2");
			}
		}
	}

	// helper function
	// doesn't need `dirty` argument because we never need to add a block to L2 and immediately have it dirty
	void AddToL2(unsigned long int address) {
		unsigned long int evicted_block;
		bool evicted, was_dirty;

		evicted = L2Cache.addBlock(address, false, evicted_block, was_dirty);

		if (evicted) {
			// evict from L1 as well, to maintain the inclusion policy
			L1Cache.evictBlock(evicted_block, was_dirty);
			// don't care about L1 eviction return values
			// (we would care if there were more levels below or we needed to track data)
		}
	}

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

		unsigned long int cyc = L1Cyc;

		if (L1Cache.accessBlockInCache(address, false)) {
			// L1 HIT
			// sanity check - this would not be checked in hardware
			if (!L2Cache.isBlockInCache(address)) throw logic_error("Cache inclusion policy error: L1 contains a block that L2 does not");
			return cyc;
		}

		// L1 MISS
		++L1Miss;
		++L2Total;
		cyc += L2Cyc;

		if (L2Cache.accessBlockInCache(address, false)) {
			// L2 HIT

			// Copy the block into L1 cache
			AddToL1(address, false);

			return cyc;
		}

		// L2 MISS
		++L2Miss;
		cyc += MemCyc;

		// read block from memory, and add it to the L2Cache then to L1
		AddToL2(address);
		AddToL1(address, false);

		return cyc;
	}

	// simulate a write to an address, return the number of cycles it took
	unsigned long int writeAddress(unsigned long int address) {
		++L1Total;
		unsigned long int cyc = L1Cyc;

		if (L1Cache.accessBlockInCache(address, true)) {
			// L1 HIT
			// sanity check - just code debug, this is not simulating anything real
			if (!L2Cache.isBlockInCache(address)) throw logic_error("Cache inclusion policy error: L1 contains a block that L2 does not");

			return cyc;
		}

		// L1 MISS
		++L1Miss;
		++L2Total;
		cyc += L2Cyc;

		// Try to use the block from L2
		if (L2Cache.accessBlockInCache(address, !WrAlloc)) {
			// L2 HIT

			if (WrAlloc) {
				// Copy the block to L1Cache, and write into that
				AddToL1(address, true);
			}

			return cyc;
		}

		// L2 MISS
		++L2Miss;
		cyc += MemCyc;

		if (WrAlloc) {
			// pull from memory to L2 and then to L1, then write to L1
			AddToL2(address);
			AddToL1(address, true);
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
		//cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		//cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		//cout << " (dec) " << num << endl;

		if (operation == 'r') {
			totalCyc += sim.readAddress(num);
		} else if (operation == 'w') {
			totalCyc += sim.writeAddress(num);
		}
		++totalAccesses;
	}

	unsigned long int L1Total, L1Miss, L2Total, L2Miss;
	sim.getStats(L1Total, L1Miss, L2Total, L2Miss);

	double L1MissRate = static_cast<double>(L1Miss) / static_cast<double>(L1Total);
	double L2MissRate = static_cast<double>(L2Miss) / static_cast<double>(L2Total);
	double avgAccTime = static_cast<double>(totalCyc) / static_cast<double>(totalAccesses);

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
