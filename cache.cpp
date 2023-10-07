#include "cache.h"

cache::cache()
{
	for (int i=0; i<L1_CACHE_SETS; i++)
		L1[i].valid = false; 
	for (int i=0; i<L2_CACHE_SETS; i++)
		for (int j=0; j<L2_CACHE_WAYS; j++)
			L2[i][j].valid = false; 

	this->myStat.missL1 =0;
	this->myStat.missL2 =0;
	this->myStat.accL1 =0;
	this->myStat.accL2 =0;
}

void cache::controller(bool MemR, bool MemW, int* data, int adr, int* myMem)
{
	//extract address
	L1_tag = (MASK_TAG_L1 & adr) >> 4;
	L1_ind = MASK_INDEX_L1 & adr;	
	L2_tag = (MASK_TAG_L2 & adr) >> 4;	
	L2_ind = MASK_INDEX_L2 & adr;
	address = adr;
	MM = myMem;

	if (MemR){
		handleLoad();

	}
	else{
		handleStore(data);
	}
	
}

void cache:: handleLoad(){
	int data;
	if (loadL1(L1_ind, L1_tag)){
		myStat.accL1 += 1; //data in L1
	}
	else {
		myStat.accL1 += 1;
		myStat.missL1 +=1;
		if (loadL2(L2_ind, L2_tag)){
			myStat.accL2 += 1; //data in L2
			updateL1(L1_ind, L1_tag, loadedData);
		}
		else{
			myStat.accL2 += 1;
			myStat.missL2 += 1;
			LoadFromMM();
			updateL1(L1_ind, L1_tag, loadedData);

		}
	}
	
	return;
}

void cache:: handleStore (int* data){
	/*if the tag is in L1 update the data in MM and L1*/
	if(storeL1(L1_ind, L1_tag)){
		myStat.accL1 += 1;
		updateL1(L1_ind, L1_tag, *data);
		updateMM(*data);
	}
	else{
		myStat.accL1 += 1;
		myStat.missL1 += 1;
		if(storeL2(L2_ind, L2_tag, *data)){
			/*tag is in L2, send it to L1 with new data, invalidate the old data in L2.
			Also, update the data in memory*/
			myStat.accL2 += 1;
			updateL1(L1_ind, L1_tag, *data);
			updateMM(*data);
		}
		else{
			myStat.accL2 += 1;
			myStat.missL2 += 1;
			updateMM(*data);
		}
	}
	return;
}

bool cache:: loadL1(int ind, int tag){
	if(L1[ind].valid == false){ // is the entry valid?
		return false;
	}
		
	else if (L1[ind].tag != tag){ // check the tag
		return false;
	}
	else return true;
}


bool cache:: loadL2(int ind, int tag){
	for(int i = 0; i < L2_CACHE_WAYS; i++){
		if (L2[ind][i].valid && L2[ind][i].tag == tag){
			loadedData = L2[ind][i].data; // load the data
			int cur_lru_pos = L2[ind][i].lru_position;
			L2[ind][i].valid = false; //remove from L2
			raiseLruPos(ind, cur_lru_pos);
			return true;
		}

	}

	return false; //data is not found
}

void cache:: updateLruPos(int set, int cur_pos){
	int stop = L2_CACHE_WAYS - 1;
	for(int i = 0; i < L2_CACHE_WAYS; i++){
		if (stop == cur_pos) break;
		
		if (L2[set][i].lru_position == stop){
			L2[set][i].lru_position--; //reduce the pos by one
			stop--; 
		}

	}

	/*updated lru pos*/
	for(int i = 0; i < L2_CACHE_SETS; i++){
		for (int j = 0; j < L2_CACHE_WAYS; j++){
			cout << "set: " << i << " tag: " << L2[i][j].tag << " position: " << L2[i][j].lru_position << endl;
		}
		cout << endl;
	}


}


void cache:: lowerLruPos(int ind){ //for installing new data in L2
	for(int i = 0; i < L2_CACHE_WAYS; i++){
		if(L2[ind][i].valid){
			L2[ind][i].lru_position--;
		}
	}
}


void cache:: LoadFromMM(){
	loadedData = MM[address];
}


void cache:: updateL1(int ind, int tag, int data){
	//the cache line is invalid
	if(L1[ind].valid){
		evictL1(ind); //remove the L1[index] from L1
	}
	
	L1[ind].valid = 1;
	L1[ind].tag = tag;
	L1[ind].data = data;

}

void cache:: updateL2(int ind, int tag, int data){
	/*find and invalid spot*/
	for(int i = 0; i < L2_CACHE_WAYS; i++){
		if(!L2[ind][i].valid){
			int lruPos = getLargestLru(ind); //the youngest lru in the set
			L2[ind][i].valid = true;
			L2[ind][i].tag = tag;
			L2[ind][i].data = data;
			if(lruPos < L2_CACHE_WAYS - 1)
				L2[ind][i].lru_position = lruPos + 1;
			else { //all the lines are full
				lowerLruPos(ind);
				L2[ind][i].lru_position = L2_CACHE_WAYS - 1;
			}
			return;
			
		}
	}

	/*if there is no invalid spot, remove the oldest, 
	install the tag, update lru positons*/
	for(int i = 0; i < L2_CACHE_WAYS; i++){
		if(L2[ind][i].lru_position == 0){
			lowerLruPos(ind);
			L2[ind][i].tag = tag; 
			L2[ind][i].data = data;
			L2[ind][i].lru_position = L2_CACHE_WAYS - 1;
			L2[ind][i].valid = true;
			return;
		}
	}

}

void cache:: evictL1(int ind){
	//get the old data @ the index
	int oldTag= L1[ind].tag ;
	int oldData= L1[ind].data;
	L1[ind].valid = 0; //invalidate the data
	updateL2(ind, oldTag, oldData); //install in L2
}

void cache:: evictL2(int ind){

}

void cache:: raiseLruPos(int ind, int cur_pos){ //removing data from L2
	for(int i =0; i < L2_CACHE_WAYS; i++){
		if(L2[ind][i].valid && L2[ind][i].lru_position < cur_pos){
			L2[ind][i].lru_position++;
		}
	}
}

int cache:: getLargestLru(int ind){
	int max = -1;
	for(int i = 0; i < L2_CACHE_WAYS; i++){
		if(L2[ind][i].valid && L2[ind][i].lru_position > max){
			max = L2[ind][i].lru_position;
		}
	}
	return max;
}

bool cache:: storeL1(int ind, int tag){
	if(L1[ind].valid && L1[ind].tag == tag){ //tag is already in L1
		L1[ind].valid = false; //invalidate the old data
		return true;
	}

	return false;
}

bool cache:: storeL2(int ind, int tag, int data){
	for(int i = 0; i < L2_CACHE_WAYS; i++){
		if(L2[ind][i].valid && L2[ind][i].tag == tag){
			int cur_lru_pos = L2[ind][i].lru_position;
			L2[ind][i].valid = false; //remove from L2
			raiseLruPos(ind, cur_lru_pos); 
			return true;
		}
	}

	return false; //data is not found
}

void cache:: updateMM(int data){
	MM[address] = data;
}

float cache::AAT(){
	
	float hitL1 = (myStat.accL1 - myStat.missL1) * L1_HIT;
	float hitL2 =  (myStat.accL2 - myStat.missL2) * L2_HIT;
	float missL1 = missRateL1();
	float missL2 = missRateL2();
	float AAT = L1_HIT + (missL1 * (L2_HIT +(missL2 * MM_HIT)));

	return AAT;
}
float cache:: missRateL1(){
	float missRate = float(myStat.missL1)/ float(myStat.accL1);
	return missRate;
}

float cache:: missRateL2(){
	
	float missRate = float(myStat.missL2)/ float(myStat.accL2);
	return missRate;
}

/*debugging*/
void cache:: printL1(){
	for(int i = 0; i < L1_CACHE_SETS; i++){
		cout <<"set#" << i
		<< " data: " << L1[i].data
		<<" tag: " << L1[i].tag
		<< " valid: " << L1[i].valid <<endl<<endl;
	}
}

void cache:: printL2(){
	int stopSet = 4;
	cout << "_____________________________________________" << endl;

	for(int i = 0; i < stopSet; i++){
		for (int j = 0 ; j < L2_CACHE_WAYS; j++){
			
			cout <<"set# " << i
			<< " way " << j
			<< " data: " << L2[i][j].data
			<<" tag: " << L2[i][j].tag
			<< " valid: " << L2[i][j].valid 
			<< " lru_pos: " << L2[i][j].lru_position <<endl<<endl;
		}
		cout << "_________________________________________________" <<endl;
	}
}

