#ifndef DISK_H
#define DISK_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <experimental/filesystem>
#include <algorithm> 
#include <cmath> 

#define K uint64_t 
#define V string
#define MAX_FILE_SIZE 2*1024*1024
#define TOMBSTONE ' '

using namespace std;
namespace fs = experimental::filesystem;
using recursive_directory_iterator = experimental::filesystem::recursive_directory_iterator;

struct Pair{
	K key; K offset; 
	Pair(K k, K of) :key(k), offset(of) {}
	bool operator==(Pair const&e) { return key == e.key; }
	bool operator!=(Pair const&e) { return key != e.key; }
};

struct SSTable {
	string filename;
	K minKey = 0;
	K maxKey = 0;
	streampos divide;	
	vector<Pair> cache;
	SSTable(string fn, K minK, K maxK, streampos div)
		:filename(fn), minKey(minK), maxKey(maxK), divide(div){}
	V* search(K key, size_t level);
};

class LevelInfo{
	size_t level;
	size_t capacity;
public:
	vector<SSTable*> fileList;
	LevelInfo(size_t l, vector<SSTable*> f) :level(l), fileList(f) { capacity = l > 5 ? 128 : (size_t)pow(2, (l + 1)); }
	bool isFull(){return (fileList.size()>capacity);}
	bool empty() {return !fileList.size();}
	void addFile(SSTable* file){fileList.push_back(file);}
	void removeFile(string filename);
	bool getFilePath(string filename, std::string& path);
	bool isSorted();
	std::vector<SSTable*> pickFiles();
	V* get(uint64_t key);	
};

class DiskInfo{
	std::vector<LevelInfo*> LevelList;
	struct TimeStamp {
		uint64_t  timestamp = 0;
		std::string getTimeStamp() { timestamp += 1; return to_string(timestamp); }
	}timeStamp;
protected:	
	bool ReachEnd(vector<vector<Pair>::iterator> indexList, vector<vector<Pair>::iterator> indexEndList);
	size_t getIterWithMinKey(vector<vector<Pair>::iterator> indexList, vector<vector<Pair>::iterator> indexEndList);
	K findMinKey(vector<SSTable*> fileList);
	K findMaxKey(vector<SSTable*> fileList);
	vector<SSTable*> SelectNextLevel(size_t level, vector<SSTable*> filesToMove);
	vector<SSTable*> filterFiles(vector<SSTable*> filesSelected, vector<SSTable*>& filesToMove);
	bool newLevel(size_t level);
	bool empty(size_t level) { return LevelList[level]->empty(); }
	void mergeFiles(std::vector<SSTable*>filesToMerge, size_t curLevel);
	void moveFiles(std::vector<SSTable*> filelists, size_t curl, size_t tarl);
public:
	DiskInfo(){fs::remove_all("./dir");}
	~DiskInfo(){fs::remove_all("./dir");}
	void Compaction(size_t levelNow);
	ofstream* createOutFile(size_t Level, string& filename);
	void finishOutFile(size_t level, ofstream* outfile, SSTable* cacheFile);
	V* get(uint64_t key);
	void load();
	void loadCache(fs::path dirEntry);
};
#endif
