#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <filesystem>
#include <algorithm> 
#include <cmath> 

#define dK uint64_t 
#define dV string
#define MAX_FILE_SIZE 2*1024*1024
#define TOMBSTONE ' '

using namespace std;
namespace fs = filesystem;
using recursive_directory_iterator = filesystem::recursive_directory_iterator;

struct Pair{
	dK key; dK offset; 
	Pair(dK k, dK of) :key(k), offset(of) {}
	bool operator==(Pair const&e) { return key == e.key; }
	bool operator!=(Pair const&e) { return key != e.key; }
};

struct SSTable {
	string filename;
	dK minKey = 0;
	dK maxKey = 0;
	streampos divide;	
	vector<Pair> cache;
	SSTable(string fn, dK minK, dK maxK, streampos div)
		:filename(fn), minKey(minK), maxKey(maxK), divide(div){}
	dV* search(dK key, size_t level);
};

class LevelInfo{
public:
	size_t level;
	size_t capacity;
	vector<SSTable*> fileList;
	LevelInfo(size_t l, vector<SSTable*> f) :level(l), fileList(f) { capacity = l > 5 ? 128 : (size_t)pow(2, (l + 1)); }
	bool isFull(){return (fileList.size()>capacity);}
	bool empty() {return !fileList.size();}
	void addFile(SSTable* file){fileList.push_back(file);}
	void removeFile(string filename);
	bool getFilePath(string filename, std::string& path);
	bool isSorted();
	std::vector<SSTable*> pickFiles();
	dV* get(uint64_t key);	
};

class DiskInfo{
	std::vector<LevelInfo*> LevelList;
	struct TimeStamp {
		uint64_t  timestamp = 0;
		std::string getTimeStamp() { timestamp += 1; return to_string(timestamp); }
		void updateTimeStamp(uint64_t need) {
			timestamp =( need > timestamp) ? need : timestamp;
		}
	}timeStamp;	
protected:		
	bool ReachEnd(vector<vector<Pair>::iterator> indexList, vector<vector<Pair>::iterator> indexEndList);
	size_t getIterWithMinKey(vector<vector<Pair>::iterator> indexList, vector<vector<Pair>::iterator> indexEndList);
	dK findMinKey(vector<SSTable*> fileList);
	dK findMaxKey(vector<SSTable*> fileList);
	vector<SSTable*> SelectNextLevel(size_t level, vector<SSTable*> filesToMove);
	vector<SSTable*> filterFiles(vector<SSTable*> filesSelected, vector<SSTable*>& filesToMove);
	bool newLevel(size_t level);
	bool empty(size_t level) { return LevelList[level]->empty(); }
	void mergeFiles(std::vector<SSTable*>filesToMerge, size_t curLevel);
	void moveFiles(std::vector<SSTable*> filelists, size_t curl, size_t tarl);
	void AddLevel(size_t level, vector<SSTable*> filesToMove);
public:
	chrono::system_clock::time_point total_start;
	DiskInfo(){}
	~DiskInfo(){}
	void Compaction(size_t levelNow);
	ofstream* createOutFile(size_t Level, string& filename);
	void finishOutFile(size_t level, ofstream* outfile, SSTable* cacheFile);
	dV* get(uint64_t key);
	void load();
	SSTable* loadFile(fs::path filePath, size_t level);
};
