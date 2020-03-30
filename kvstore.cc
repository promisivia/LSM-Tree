#include "kvstore.h"
#include <string>
#include <iostream>
#include <fstream>

KVStore::KVStore(const std::string &dir): KVStoreAPI(dir)
{
	KVStore::MemTable = new Skiplist();
	KVStore::Disk = new DiskInfo();
}

KVStore::~KVStore()
{
	reset();
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s)
{
	if (MemTable->put(key, s)) {
		return;
	}
	else {
		//std::cerr << "The MenTable is full." << std::endl;
		createSSTable();
		MemTable->put(key, s);
	}	
}
/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key)
{
	V* value = new V();
	value = MemTable->get(key);
	// if can find, return
	if (value) {
		return *value;
	}
	// if can't find, find in disk
	else {
		value = Disk->get(key);
		if (value) {
			//cout << "get value in d:" << *value << endl;
			return *value;
		}
		else return "";
	}
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key)
{
	V value = get(key);
	if (value == "") {		
		return false;
	}
	put(key,"");
	return true;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset()
{
	delete MemTable;
	delete Disk;
}

void KVStore::resetMemTable(){
	delete MemTable;
	MemTable = nullptr;
	KVStore::MemTable = new Skiplist();
}

void KVStore::createSSTable(){
	std::string filename;
	std::ofstream* outfile = Disk->createOutFile(0, filename);
	auto offset = outfile->tellp();

	QuadlistNode* list = MemTable->getFirst();
	SSTable* cacheFile = new SSTable(filename, 0, 0, 0);
	do {
		*outfile << list->entry.key << " ";
		offset = outfile->tellp();
		*outfile << list->entry.value << " ";
		cacheFile->cache.push_back(Pair(list->entry.getKey(), offset));
		list = list->succ;
	} while (list->succ);	
	// put to level 0
	
	Disk->finishOutFile(0, outfile, cacheFile);		
	Disk->Compaction(0);
	resetMemTable();
}
