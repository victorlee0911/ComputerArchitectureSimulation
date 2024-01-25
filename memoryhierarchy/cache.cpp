#include "cache.h"

cache::cache()
{
	for (int i=0; i<L1_CACHE_SETS; i++)
		L1[i].valid = false; 
	for (int i=0; i<L2_CACHE_SETS; i++)
	{
		L2Size[i] =0;
		for (int j=0; j<L2_CACHE_WAYS; j++)
			L2[i][j].valid = false; 
	}
	for (int i=0; i<VICTIM_SIZE; i++)
		vic[i].valid = false;

	// Do the same for Victim Cache ...

	this->myStat.missL1 =0;
	this->myStat.missL2 =0;
	this->myStat.accL1 =0;
	this->myStat.accL2 =0;
	this->myStat.accVic =0;
	this->myStat.missVic =0;

	vicSize =0;
	// Add stat for Victim cache ... 
	
}
void cache::controller(bool MemR, bool MemW, int* data, int adr, int* myMem)
{
	// add your code here
	string addr[3];
	addrProcess(adr, addr);
	int tag = (int)(bitset<6>(addr[0]).to_ulong());
	int index = (int)(bitset<4>(addr[1]).to_ulong());
	int vtag = (int)(bitset<10>(addr[0]+addr[1]).to_ulong());	//victim tag

	//cout << MemR << "t: " << tag << " i: " << index << " vt: " << vtag << endl;

	if (MemR)
	{
		load(tag, index, vtag);
	}
	else	//store
	{
		store(tag, index, vtag);
	}

	
	//cout << "miss: " << this->myStat.missL1 << ", acc: " << this->myStat.accL1 << " miss vic:" << this->myStat.missVic << " miss L2: " << this->myStat.missL2 << endl << endl;

}

void cache::addrProcess(int adr, string* split)
{
	string addr = bitset<12>(adr).to_string();

	split[0] = addr.substr(0, 6);
	split[1] = addr.substr(6, 4);
	split[2] = addr.substr(10, 2);

	return;
}


//load functions
void cache::load(int tag, int index, int vtag)
{
	//check L1 first

	//L1 actions: search L1, bring into L1
	this->myStat.accL1++;
	if (searchL1(tag, index))
		return;
	this->myStat.missL1++;

	this->myStat.accVic++;
	if (searchVic(vtag))
	{
		//cout << "inVictim" << endl;
		//move found to L1 and replace vic table

		evictVic(vtag);
		bringToL1(tag, index);
		return;
	}
	this->myStat.missVic++;

	this->myStat.accL2++;
	if (searchL2(tag, index))
	{
		//cout << "inL2" << endl;
		evictL2(tag, index);
		bringToL1(tag, index);
		return;
	}
	this->myStat.missL2++;

	//access memory
	bringToL1(tag, index);
	return;
}

bool cache::searchL1(int tag, int index)
{
	if(L1[index].valid and L1[index].tag == tag)
		return true;
	return false;
}

//returns replaced tag or -1
int cache::replaceL1(int tag, int index)
{
	//direct map
	int ret;
	if (L1[index].valid)
	{
		ret = L1[index].tag;
	}
	else 
	{
		ret = -1;
		L1[index].valid = true;
	}
	L1[index].tag = tag;
	return ret;
}

bool cache::searchVic(int vtag)
{
	for (int i =0; i < VICTIM_SIZE; i++)
		if (vic[i].valid && vic[i].tag == vtag)
			return true;
	return false;
}

//returns replaced vtag or -1
int cache::replaceVic(int vtag)
{
	int ret;

	//full
	if (vicSize == VICTIM_SIZE)
	{
		for (int i=0; i < VICTIM_SIZE; i++)
		{
			if (vic[i].lru_position == vicSize - 1)	//all valid
			{
				ret = vic[i].tag;
				vic[i].tag = vtag;
				vic[i].lru_position = -1;
				break;
			}
		}
	}
	else
	{
		vicSize++;
		for (int i=0; i < VICTIM_SIZE; i++)
		{
			if (!vic[i].valid)
			{
				ret = -1;
				vic[i].tag = vtag;
				vic[i].lru_position = -1;
				vic[i].valid = true;
				break;
			}
		}
	}

	//increase all lrupositions
	for (int i=0; i < VICTIM_SIZE; i++)
	{
		if (vic[i].valid)
			vic[i].lru_position++;
	}

	return ret;
}

void cache::evictVic(int vtag)
{
	int oldpos = -1;
	for (int i=0; i < VICTIM_SIZE; i++)
	{
		if (vic[i].valid && vic[i].tag == vtag)
		{
			vic[i].valid = false;
			oldpos = vic[i].lru_position;
			break;
		}
	}
	vicSize--;

	for (int i=0; i < VICTIM_SIZE; i++)
	{
		if (vic[i].valid && vic[i].lru_position > oldpos)
			vic[i].lru_position--;
	}
	return;
}

bool cache::searchL2(int tag, int index)
{
	//search
	for (int i=0; i < L2_CACHE_WAYS; i++)
		if (L2[index][i].valid && L2[index][i].tag == tag)
			return true;
	return false;
}

void cache::replaceL2(int tag, int index)
{
	int oldpos = -1;

	if (L2Size[index] == L2_CACHE_WAYS)	//full
	{
		for (int i=0; i < L2_CACHE_WAYS; i++)
		{
			if (L2[index][i].lru_position == L2_CACHE_WAYS - 1)
			{
				L2[index][i].tag = tag;
				L2[index][i].lru_position = -1;
				break;
			}
		}
	}
	else	//not full, no replacement
	{
		L2Size[index]++;
		for (int i=0; i < L2_CACHE_WAYS; i++)
		{
			if (!L2[index][i].valid)
			{
				L2[index][i].tag = tag;
				L2[index][i].lru_position = -1;
				L2[index][i].valid = true;
				break;
			}
		}
	}

	for (int i=0; i < L2_CACHE_WAYS; i++)
	{
		if(L2[index][i].valid)
			L2[index][i].lru_position++;
	}
	return;
}

void cache::evictL2(int tag, int index)
{
	int oldpos = -1;
	for (int i=0; i < L2_CACHE_WAYS; i++)
	{
		if (L2[index][i].valid && L2[index][i].tag == tag)
		{
			L2[index][i].valid = false;
			oldpos = L2[index][i].lru_position;
			break;
		}
	}
	L2Size[index]--;

	for (int i=0; i < L2_CACHE_WAYS; i++)
		if (L2[index][i].valid  && L2[index][i].lru_position > oldpos)
			L2[index][i].lru_position--;
}

//store functions
void cache::store(int tag, int index, int vtag)
{
	//L1 does nothing
	//update victim and L2
	if (updateVic(vtag))
	{
		//won't be in L2
		return;
	}
	//update L2
	updateL2(tag, index);

}

bool cache::updateVic(int vtag)
{
	int oldPos = -1;
	bool ret = false;
	for (int i=0; i < VICTIM_SIZE; i++)
	{
		if (vic[i].valid && vic[i].tag == vtag)
		{
			oldPos = vic[i].lru_position;
			vic[i].lru_position = -1;
			ret = true;
		}
	}
	//update all positions less than oldpos
	for (int i=0; i < VICTIM_SIZE; i++)
		if (vic[i].valid && vic[i].lru_position < oldPos)
			vic[i].lru_position++;
	return ret;
}

void cache::updateL2(int tag, int index)
{
	int oldpos = -1;
	for (int i=0; i < L2_CACHE_WAYS; i++)
	{
		if (L2[index][i].valid && L2[index][i].tag == tag)
		{
			oldpos = L2[index][i].lru_position;
			L2[index][i].lru_position = -1;
		}
	}

	for (int i=0; i < L2_CACHE_WAYS; i++)
	{
		if (L2[index][i].valid && L2[index][i].lru_position < oldpos)
			L2[index][i].lru_position++;
	}
}

int cache::tagToVtag(int tag, int index)
{
	string strTag = bitset<6>(tag).to_string();
	string strIndex = bitset<4>(index).to_string();
	int tempVtag = (int)(bitset<10>(strTag+strIndex).to_ulong());
	return tempVtag;
}

void cache::bringToL1(int tag, int index)
{
	int tempToVic = replaceL1(tag, index);
		if (tempToVic > -1)
		{
			int tempToL2 = replaceVic(tagToVtag(tempToVic, index));
			//replace in L2 if needed
			if (tempToL2 > -1)
			{
				string strVtag = bitset<10>(tempToL2).to_string();
				int replTag = (int)(bitset<6>(strVtag.substr(0, 6)).to_ulong());
				int replIndex = (int)(bitset<4>(strVtag.substr(6, 4)).to_ulong());
				replaceL2(replTag, replIndex);
			}
		}
}

void cache::computeStats(long double* l1miss, long double* l2miss, long double*aat)
{
	*l1miss =this->myStat.missL1/this->myStat.accL1;
	*l2miss = this->myStat.missL2/this->myStat.accL2;
	*aat = (this->myStat.accL1*1 + this->myStat.accVic*1 + this->myStat.accL2*8 + this->myStat.missL2*100)/this->myStat.accL1;
}



