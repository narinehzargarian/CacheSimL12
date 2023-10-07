#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1
#define MASK_TAG_L1 0xfffffff << 4
#define MASK_TAG_L2 0xfffffff << 4
#define MASK_INDEX_L1 0xf
#define MASK_INDEX_L2 0xf 
#define MASK_BLOCK_OFFSET 0x7
#define L1_HIT 1
#define L2_HIT 8
#define MM_HIT 100

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
	int missL1; 
	int missL2; 
	int accL1;
	int accL2;
	// add more stat if needed. Don't forget to initialize!
};

class cache {
private:
	cacheBlock L1[L1_CACHE_SETS]; // 1 set per row.
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row 
	Stat myStat;
	unsigned int L1_tag, L2_tag;
	unsigned int L1_ind, L2_ind;
	int L2_offset;
	unsigned int address;
	int loadedData;
	int* MM;

	// add more things here
public:
	cache();
	void controller(bool MemR, bool MemW, int* data, int adr, int* myMem);
	void handleLoad();

	void handleStore(int* data); 
	
	bool loadL1(int ind, int tag); 
	bool loadL2(int ind, int tag); 
	void updateL1(int ind, int tag, int data);
	void updateL2(int ind, int tag, int data); 
	void evictL1(int ind); 
	void evictL2(int ind);
	void LoadFromMM();
	void updateLruPos(int ind, int cur_pos); 
	void lowerLruPos(int ind);
	void raiseLruPos(int ind, int cur_pos);
	int getLargestLru(int ind);
	bool storeL1(int ind, int tag);
	bool storeL2(int ind, int tag, int data);
	void updateMM(int data);
	float missRateL1();
	float missRateL2();
	float AAT();

	/*debugging fucntions*/
	void printL1();
	void printL2();
	
};


