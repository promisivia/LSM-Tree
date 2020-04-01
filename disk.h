#ifndef DISK_H
#define DISK_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <experimental/filesystem>
#include <algorithm> 
#include <math.h> 

#define K uint64_t 
#define V std::string
#define ToString std::to_string
#define MAX_FILE_SIZE 2*1024*1024

namespace fs = std::experimental::filesystem;
using recursive_directory_iterator = std::experimental::filesystem::recursive_directory_iterator;

struct Pair{
	K key; K offset; 
	Pair(K k, K of) :key(k), offset(of) {}
	bool operator==(Pair const&e) { return key == e.key; }
	bool operator!=(Pair const&e) { return key != e.key; }
};

struct SSTable {
	std::string filename = "";
	K minKey = 0;
	K maxKey = 0;
	std::streampos divide;	
	std::vector<Pair> cache;
	SSTable(std::string fn, K minK, K maxK, std::streampos div)
		:filename(fn), minKey(minK), maxKey(maxK), divide(div){}
	V* search(K key, size_t level);
};

class LevelInfo{
	size_t level;
	size_t max_size;
public:
	std::vector<SSTable*> fileList;
	LevelInfo(size_t l, std::vector<SSTable*> f) 
		:level(l), fileList(f) {max_size = std::pow(2, (l + 1));}
	bool isFull(){return (fileList.size()>max_size);}
	bool empty() {return !fileList.size();}
	void addFile(SSTable* file){fileList.push_back(file);}
	void removeFile(std::string filename);
	bool getFilePath(std::string filename, std::string& path);
	std::vector<SSTable*> pickFiles();
	V* get(uint64_t key);
	bool isSorted();
};

class DiskInfo{
	std::vector<LevelInfo*> LevelList;
	struct TimeStamp {
		uint64_t  timestamp = 0;
		std::string getTimeStamp() { timestamp += 1; return ToString(timestamp); }
	}timeStamp;
protected:	
	bool ReachEnd(std::vector<std::vector<Pair>::iterator> indexList, std::vector<std::vector<Pair>::iterator> indexEndList);
	size_t getIterWithMinKey(std::vector<std::vector<Pair>::iterator> indexList, std::vector<std::vector<Pair>::iterator> indexEndList);
	K findMinKey(std::vector<SSTable*> fileList);
	K findMaxKey(std::vector<SSTable*> fileList);
	std::vector<SSTable*> SelectNextLevel(size_t level, std::vector<SSTable*>& filesToSort);
	std::vector<SSTable*> filterFiles(std::vector<SSTable*> filesSelected, std::vector<SSTable*>& filesToMove);
	bool newLevel(size_t level);
	bool empty(size_t level) { return LevelList[level]->empty(); }
	void mergeFiles(std::vector<SSTable*>filesToMerge, size_t curLevel);
	void moveAllFiles(std::vector<SSTable*> filelists, size_t curl, size_t tarl);
public:
	DiskInfo(){fs::remove_all("./dir");}
	~DiskInfo(){fs::remove_all("./dir");}
	void Compaction(size_t levelNow);
	std::ofstream* createOutFile(size_t Level, std::string& filename);
	void finishOutFile(size_t level, std::ofstream* outfile, SSTable* cacheFile);
	V* get(uint64_t key);
	void load();
	void loadCache(fs::path dirEntry);
};
#endif
