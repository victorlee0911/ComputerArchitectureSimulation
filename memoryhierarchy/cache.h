#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define VICTIM_SIZE 4
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1

struct cacheBlock
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for SA only
	int data; // the actual data stored in the cache/memory
	bool valid;
	// add more things here if needed
};

struct Stat
{
	long double missL1; 
	long double missL2; 
	long double accL1;
	long double accL2;
	long double accVic;
	long double missVic;
	// add more stat if needed. Don't forget to initialize!
};

class cache {
private:
	cacheBlock L1[L1_CACHE_SETS]; // 1 set per row.
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row 
	// Add your Victim cache here ...
	cacheBlock vic[VICTIM_SIZE];
	
	Stat myStat;
	// add more things here
	int vicSize;	//# of things in victim 
	int L2Size[L2_CACHE_SETS];	//# of things in each set
public:
	cache();
	void controller(bool MemR, bool MemW, int* data, int adr, int* myMem);
	// add more functions here ...	
	void addrProcess(int adr, string* split);
	void load(int tag, int index, int vtag);
	void store(int tag, int index, int vtag);

	bool searchL1(int tag, int index);
	int replaceL1(int tag, int index);

	bool searchVic(int vtag);
	int replaceVic(int vtag);
	bool updateVic(int vtag);
	void evictVic(int vtag);

	bool searchL2(int tag, int index);
	void replaceL2(int tag, int index);
	void updateL2(int tag, int index);
	void evictL2(int tag, int index);

	//helpers
	int tagToVtag(int tag, int index);
	void bringToL1(int tag, int index);

	void computeStats(long double* l1miss, long double* l2miss, long double* aat);
};


