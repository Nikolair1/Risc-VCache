#include "cache.h"

cache::cache()
{
	for (int i = 0; i < L1_CACHE_SETS; i++)
		L1[i].valid = false;
	for (int i = 0; i < L2_CACHE_SETS; i++)
		for (int j = 0; j < L2_CACHE_WAYS; j++)
			L2[i][j].valid = false;
	for (int i = 0; i < VICTIM_SIZE; i++)
	{
		VC[i].valid = false;
	}

	// Do the same for Victim Cache ...

	this->myStat.missL1 = 0;
	this->myStat.missL2 = 0;
	this->myStat.accL1 = 0;
	this->myStat.accL2 = 0;
	this->myStat.missVic = 0;
	this->myStat.accVic = 0;
	// Add stat for Victim cache ...
}
void cache::controller(bool MemR, bool MemW, int *data, int adr, int *myMem)
{
	if (MemR)
	{
		execute_lw(adr, myMem);
	}
	else
	{
		execute_sw(data, adr, myMem);
	}
}

void cache::execute_lw(int adr, int *myMem)
{
	bool status = false;
	int row = 0;
	myStat.accL1++;
	if (search_L1(adr))
	{
		// Do nothing for hit in L1
	}
	else
	{
		myStat.missL1++;
		myStat.accVic++;

		if (search_VC(adr, row))
		{
			// Hit in Victim cache, call execution function
			execute_hit_VC(adr, row);
		}
		else
		{
			myStat.missVic++;
			myStat.accL2++;
			int way = 0;
			if (search_L2(adr, way))
			{
				// Hit in L2, call execution function
				execute_hit_L2(adr, way);
			}
			else
			{
				// Else, we missed in L2, call execution function
				myStat.missL2++;
				execute_miss_l2(adr, myMem);
			}
		}
	}
}

void cache::execute_sw(int *data, int adr, int *myMem)
{
	bool status = false;
	int index = (adr & 0b111100) >> 2;
	int row = 0;
	int way = 0;

	if (search_L1(adr))
	{
		// if data is in both L1 cache and memory, both should be updated
		L1[index].data = *data;
		myMem[adr] = *data;
	}
	else
	{
		if (search_VC(adr, row))
		{
			// Call execute hit VC and update L1 with that data as well as mem
			execute_hit_VC(adr, row);
			L1[index].data = *data;
			myMem[adr] = *data;
		}
		else
		{
			int way = 0;
			if (search_L2(adr, way))
			{
				// L2 is a hit, update it, execute the hit, bringing the data to L1, then update memory
				L2[index][way].data = *data;
				execute_hit_L2(adr, way);
				myMem[adr] = *data;
			}
			else
			{
				// Here we have to just update main mem since L2 was a miss
				myMem[adr] = *data;
			}
		}
	}
}

bool cache::search_L1(int adr)
{
	// to search L1, we just need to isolate the index, then check L1[index] == tag
	int index = (adr & 0b111100) >> 2;
	int tag = adr >> 6;
	if (L1[index].valid && L1[index].tag == tag)
	{
		return true;
	}
	return false;
}

bool cache::search_VC(int adr, int &row)
{
	// Have to compare entire tag in SA cache
	int tag = adr >> 2;
	bool hit = false;
	for (int i = 0; i < VICTIM_SIZE; i++)
	{
		if (VC[i].valid && VC[i].tag == tag)
		{
			hit = true;
			row = i;
			break;
		}
	}
	return hit;
}

bool cache::search_L2(int adr, int &way)
{
	// to search through L2, we have to loop through each way in row L2[index]
	int index = (adr & 0b111100) >> 2;
	int tag = adr >> 6;
	cacheBlock *row = L2[index];
	bool hit = false;
	for (int i = 0; i < L2_CACHE_WAYS; i++)
	{
		if (L2[index][i].valid && (tag == L2[index][i].tag))
		{
			// saving way, since pass by reference function
			way = i;
			hit = true;
		}
	}
	return hit;
}

void cache::execute_hit_VC(int adr, int row)
{
	cacheBlock curr = VC[row];
	int DMTag = curr.tag >> 4; // converting SA tag to a DM Tag
	int DMIndex = curr.tag & 0b1111;
	// Now let's save the value in L1 before we override it
	struct cacheBlock save;
	save.valid = L1[DMIndex].valid;
	save.tag = L1[DMIndex].tag;
	save.data = L1[DMIndex].data;
	// override L1 with curr
	L1[DMIndex].valid = true;
	L1[DMIndex].tag = DMTag;
	L1[DMIndex].data = curr.data;
	VC[row].valid = false;
	// now add saved value to victim cache if it is valid
	if (save.valid)
	{
		VC[row].valid = true;
		VC[row].tag = (save.tag << 4) | DMIndex;
		VC[row].data = save.data;
	}
	update_LRU_VC(curr.lru_position, row);
}

void cache::update_LRU_VC(int oldLRUPos, int row)
{
	// algorithm is to decrement every value of LRU that is greater than removed one (oldLRUPos),
	// then replace old one (row) with VICTIM_SIZE - 1 aka highest priority
	for (int i = 0; i < VICTIM_SIZE; i++)
	{
		if (VC[i].valid && (VC[i].lru_position > oldLRUPos))
		{
			VC[i].lru_position--;
		}
	}

	VC[row].lru_position = VICTIM_SIZE - 1;
}

void cache::update_LRU_L2(int oldLRUPos, int row, int way)
{
	// same algorithm as VC one
	for (int i = 0; i < L2_CACHE_WAYS; i++)
	{
		if (L2[row][i].valid && (L2[row][i].lru_position > oldLRUPos))
		{
			L2[row][i].lru_position--;
		}
	}
	L2[row][way].lru_position = L2_CACHE_WAYS - 1;
}

void cache::execute_hit_L2(int adr, int way)
{
	// bring data to L1, remove from L2,
	// and update LRU, eviction from L1 -> victim, victim  -> L2
	// L2 eviction -> nowhere
	int index = (adr & 0b111100) >> 2;
	int tag = adr >> 6;
	// save L1
	struct cacheBlock save;
	save.tag = L1[index].tag;
	save.data = L1[index].data;
	save.valid = L1[index].valid;
	// Bring data from L2 -> L1
	L1[index].tag = L2[index][way].tag;
	L1[index].data = L2[index][way].data;
	L1[index].valid = true;
	// invalidate L2 spot now
	L2[index][way].valid = false;
	// now add saved value to victim cache if L1 was valid
	if (save.valid)
	{
		int found = 0;
		for (int i = 0; i < VICTIM_SIZE; i++)
		{
			if (VC[i].valid == false)
			{
				// we can copy L1 here
				found = 1;
				VC[i].tag = (save.tag << 4) | index;
				VC[i].data = save.data;
				VC[i].valid = true;
				update_LRU_VC(0, i);
				break;
			}
		}
		if (found == 0)
		{
			// This means there was no open spot in the Victim cache
			int oldest_lru_index = 0;
			// Looping to find LRU
			for (int i = 0; i < VICTIM_SIZE; i++)
			{
				if ((VC[i].valid == true) && (VC[i].lru_position == 0))
				{
					oldest_lru_index = i;
					break;
				}
			}

			// VC -> L2, converting tag from SA -> DM
			int L2TAG = VC[oldest_lru_index].tag >> 4;
			int L2INDEX = VC[oldest_lru_index].tag & 0b1111;
			L2[L2INDEX][way].tag = L2TAG;
			L2[L2INDEX][way].data = VC[oldest_lru_index].data;
			L2[L2INDEX][way].valid = true;
			update_LRU_L2(VC[oldest_lru_index].lru_position, index, way);

			// Now a VC spot is open, add in L1 data there
			VC[oldest_lru_index].data = save.data;
			VC[oldest_lru_index].tag = (save.tag << 4) | index;
			VC[oldest_lru_index].valid = true;
			update_LRU_VC(0, oldest_lru_index);
		}
	}
}

void cache::execute_miss_l2(int adr, int *myMem)
{
	int index = (adr & 0b111100) >> 2;
	int tag = adr >> 6;
	// save L1
	struct cacheBlock save;
	save.tag = L1[index].tag;
	save.data = L1[index].data;
	save.valid = L1[index].valid;
	// load main mem into L1
	L1[index].tag = tag;
	L1[index].valid = true;
	L1[index].data = myMem[adr];

	if (save.valid)
	{
		// Now we have to kick out L1 and put in VC
		int found = 0;
		for (int i = 0; i < VICTIM_SIZE; i++)
		{
			if (VC[i].valid == false)
			{
				// Done if we have an empty spot in VC
				found = 1;
				VC[i].tag = (save.tag << 4) | index;
				VC[i].data = save.data;
				VC[i].valid = true;
				update_LRU_VC(0, i);
				break;
			}
		}
		if (found == 0)
		{
			// Looking for LRU in VC
			int oldest_lru_index = 0;
			for (int i = 0; i < VICTIM_SIZE; i++)
			{
				if ((VC[i].valid == true) && (VC[i].lru_position == 0))
				{
					oldest_lru_index = i;
					break;
				}
			}
			int L2TAG = VC[oldest_lru_index].tag >> 4; // Converting SA to DM for L2 search
			int L2INDEX = VC[oldest_lru_index].tag & 0b1111;

			// Since VC was full, have to find an open spot in L2 for VC ejection
			int open_way = -1;
			for (int i = 0; i < L2_CACHE_WAYS; i++)
			{
				if (L2[L2INDEX][i].valid == false)
				{
					// Open spot found
					open_way = i;
				}
			}
			if (open_way == -1)
			{
				// Since no open spot, have to evict
				for (int i = 0; i < L2_CACHE_WAYS; i++)
				{
					if (L2[L2INDEX][i].lru_position == 0)
					{
						open_way = i;
						break;
					}
				}
			}
			update_LRU_L2(0, L2INDEX, open_way);

			// Now have to get the index from the victim, and shift victim to L2
			L2[L2INDEX][open_way].tag = L2TAG;
			L2[L2INDEX][open_way].data = VC[oldest_lru_index].data;
			L2[L2INDEX][open_way].valid = true;

			// L1 to VC
			VC[oldest_lru_index].tag = (save.tag << 4) | index;
			VC[oldest_lru_index].data = save.data;
			VC[oldest_lru_index].valid = true;
			update_LRU_VC(0, oldest_lru_index);
		}
	}
}

Stat cache::get_stats()
{
	return this->myStat;
}