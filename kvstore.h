#pragma once

#include "kvstore_api.h"
#include <filesystem>

class KVStore : public KVStoreAPI {
private:
	Skiplist* MemTable;
	DiskInfo* Disk;
protected:
	/**
	 * When the MemTable is full.
	 * Create a SSTable and put it to disk.
	 */
	void createSSTable();
	/**
	 * Reset the MemTable.
	 */
	void resetMemTable();
public:
	KVStore(const std::string &dir);
	~KVStore();
	void put(uint64_t key, const std::string &s) override;
	std::string get(uint64_t key) override;
	bool del(uint64_t key) override;
	void reset() override;
};
