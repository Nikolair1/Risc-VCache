#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
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
	int tag;		  // you need to compute offset and index to find the tag.
	int lru_position; // for SA only
	int data;		  // the actual data stored in the cache/memory
	bool valid;
	// add more things here if needed
};

struct Stat
{
	int missL1;
	int missL2;
	int accL1;
	int accL2;
	int accVic;
	int missVic;
	// add more stat if needed. Don't forget to initialize!
};

class cache
{
private:
	cacheBlock L1[L1_CACHE_SETS];				 // 1 set per row.
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row
	cacheBlock VC[VICTIM_SIZE];
	// Add your Victim cache here ...

	Stat myStat;
	// add more things here
	void execute_lw(int adr, int *myMem);
	void execute_sw(int *data, int adr, int *myMem);
	bool search_L1(int adr);
	bool search_VC(int adr, int &row);
	bool search_L2(int adr, int &way);
	void execute_hit_VC(int adr, int row);
	void update_LRU_VC(int oldLruPos, int row);
	void update_LRU_L2(int oldLRUPos, int row, int way);
	void execute_miss_l2(int adr, int *myMem);
	void execute_hit_L2(int adr, int way);

public:
	cache();
	void controller(bool MemR, bool MemW, int *data, int adr, int *myMem);
	Stat get_stats();
	// add more functions here ...
};
