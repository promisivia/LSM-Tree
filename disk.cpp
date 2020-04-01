#include "disk.h"
#include <assert.h>

void LevelInfo::removeFile(std::string filename) {
	auto iter = fileList.begin();
	while (iter != fileList.end()) {
		if ((*iter)->filename == filename) {
			iter = fileList.erase(iter);
			std::string path = "./dir/level" + std::to_string(level) + "/" + filename + ".txt";
			remove((char*)path.data());
		}
		else iter++;
	}
}

bool LevelInfo::getFilePath(std::string filename, std::string& path) {
	auto iter = fileList.begin();
	while (iter != fileList.end()) {
		if ((*iter)->filename == filename) {
			path = "./dir/level" + std::to_string(level) + "/" + filename + ".txt";
			return true;
		}
		iter++;
	}
	return false;
}

std::vector<SSTable*> LevelInfo::pickFiles() {
	std::vector<SSTable*> pickedFiles;
	size_t num = fileList.size() - max_size + 1;
	for (size_t i = 0; i < num; i++) {
		pickedFiles.push_back(fileList[i]);
	}
	return pickedFiles;
}

bool LevelInfo::isSorted() {
	for (size_t i = 0; i < fileList.size();i++) {
		for (size_t j = i; j < fileList.size(); j++) {
			if (fileList[i]->maxKey >= fileList[j]->minKey && fileList[i]->minKey <= fileList[j]->maxKey)
				return false;
		}
	}
	return true;
}

bool DiskInfo::newLevel(size_t level) {
	bool create = false;
	if (!fs::exists("./dir/level" + ToString(level))) {
		fs::create_directory("./dir/level" + ToString(level));
	}
	if (LevelList.size() <= level) {
		std::vector<SSTable*> fileList;
		LevelList.push_back(new LevelInfo(level, fileList));
		create = true;
	}
	return create;
}

K DiskInfo::findMaxKey(std::vector<SSTable*> fileList) {
	K res = fileList[0]->maxKey;
	for (auto &c : fileList) {
		if (c->maxKey > res)
			res = c->maxKey;
	}
	return res;
}

K DiskInfo::findMinKey(std::vector<SSTable*> fileList) {
	K res = fileList[0]->minKey;
	for (auto c : fileList) {
		if (c->minKey < res)
			res = c->minKey;
	}
	return res;
}

std::vector<SSTable*> DiskInfo::SelectNextLevel(size_t level, std::vector<SSTable*>& filesToSort) {
	
	std::vector<SSTable*> filesSelected;
	K minKey = findMinKey(filesToSort);
	K maxKey = findMaxKey(filesToSort);
	for (auto &c:LevelList[level]->fileList){
		if (!(c->minKey > maxKey || c->maxKey < minKey)){
			filesSelected.push_back(c);
		}
	}
	return filesSelected;
}

std::vector<SSTable*> DiskInfo::filterFiles(std::vector<SSTable*> filesSelected, std::vector<SSTable*>& filesToMove) {
	std::vector<SSTable*> filesFiltered;
	K minKey = findMinKey(filesSelected);
	K maxKey = findMaxKey(filesSelected);
	auto iter = filesToMove.begin();
	while (iter != filesToMove.end()) {
		if (!((*iter)->minKey > maxKey || (*iter)->maxKey < minKey)) {
			filesFiltered.push_back((*iter));
			iter = filesToMove.erase(iter);
		}else iter++;
	}
	return filesFiltered;
}

void DiskInfo::moveAllFiles(std::vector<SSTable*> filelist, size_t curl, size_t tarl) {
	std::string name, oldP, newP;
	for (size_t i = 0; i < filelist.size(); i++)
	{
		name = filelist[i]->filename;
		// disk
		oldP = "./dir/level" + ToString(curl) + "/" + name + ".txt";
		newP = "./dir/level" + ToString(tarl) + "/" + name + ".txt";
		fs::rename(oldP, newP);
		// memory
		LevelList[tarl]->addFile(filelist[i]);

		auto iter = LevelList[curl]->fileList.begin();
		while (iter != LevelList[curl]->fileList.end()) {
			if ((*iter)->filename == name) {
				iter = LevelList[curl]->fileList.erase(iter);				
			}
			else iter++;
		}		
	}
}

void DiskInfo::Compaction(size_t levelNow) {
	if (!LevelList[levelNow]->isFull()) {
		return;
	}
	//select files
	std::vector<SSTable*> filesToMove;
	std::vector<SSTable*> filesToMerge;
	newLevel(levelNow + 1);

	if (levelNow == 0) {
		filesToMove = LevelList[0]->fileList;//第一层的所有文件	
		/*
		if (LevelList[0]->isSorted()) {
			moveAllFiles(filesToMove, 0, 1);
			Compaction(1);
			return;
		}
		*/		
		if (!empty(1)) {
			//如果下一层存在文件，去下一层选取文件
			std::vector<SSTable*> filesSelected = SelectNextLevel(1, filesToMove);
			if (filesSelected.empty()) {
				//如果没有选到
				moveAllFiles(filesToMove, 0, 1);
				Compaction(1);
				return;
			}
		}

		//ordered with their create time
		std::sort(filesToMove.begin(), filesToMove.end(),
			[](const SSTable* a, const SSTable* b) {
			return std::stoi(a->filename) > std::stoi(b->filename);
		});

		//merge files
		mergeFiles(filesToMove, 0);
		Compaction(1);
		return;
	}		
	else {
		filesToMove = LevelList[levelNow]->pickFiles();
		newLevel(levelNow + 1);
		if (!empty(levelNow + 1)) {
			//如果下一层存在文件，去下一层选取文件
			std::vector<SSTable*> filesSelected = SelectNextLevel(levelNow + 1, filesToMove);
			if (filesSelected.empty()) {
				//如果没有选到
				moveAllFiles(filesToMove, levelNow, levelNow + 1);
				Compaction(levelNow + 1);
				return;
			}
			else {
				//如果选到了
				filesToMerge = filterFiles(filesSelected, filesToMove);
				moveAllFiles(filesToMove, levelNow, levelNow + 1);
				mergeFiles(filesToMerge, levelNow);
				//ordered with their create time
				std::sort(filesToMerge.begin(), filesToMerge.end(),
					[](const SSTable* a, const SSTable* b) {
					return std::stoi(a->filename) > std::stoi(b->filename);
				});
				Compaction(levelNow + 1);
				return;
			}
		}
		else {
			//如果下一层为空
			moveAllFiles(filesToMove, levelNow, levelNow + 1);
			Compaction(levelNow + 1);
			return;
		}
	}
}

void DiskInfo::mergeFiles(std::vector<SSTable*> filesToMerge, size_t curLevel)
{
	size_t fileCount = filesToMerge.size();

	// init list of iter and list of files to read
	std::vector<std::vector<Pair>::iterator> iterList;
	std::vector<std::vector<Pair>::iterator> iterEndList;
	std::vector <std::ifstream*> inFileList;
	
	for (size_t i = 0; i < fileCount; i++)
	{
		iterList.push_back(filesToMerge[i]->cache.begin());
		iterEndList.push_back(filesToMerge[i]->cache.end());

		std::string path;
		std::ifstream* infile;
		if(LevelList[curLevel]->getFilePath(filesToMerge[i]->filename,path)){
			infile = new std::ifstream(path, std::ios::binary);
		}
		else {
			infile = new std::ifstream("./dir/level" + ToString(curLevel+1) + "/" + filesToMerge[i]->filename + ".txt", std::ios::binary);
		}
		inFileList.push_back(infile);
	}
	
	// init files to write
	std::string filename;
	std::ofstream* outfile = createOutFile(curLevel + 1, filename);//在下一层创建文件
	std::vector <std::ofstream*> outFileList;
	outFileList.push_back(outfile);

	// init the detail of SSTable
	size_t curFileSize = 0;
	std::string value;
	char test;
	auto offset = outfile->tellp(); //offset
	SSTable* cacheFile = new SSTable(filename, 0, 0, 0); 
	std::vector<SSTable*> cacheFileList;
	cacheFileList.push_back(cacheFile);

	size_t indexOut = 0;

	// merge infiles to outfiles	
	while (!ReachEnd(iterList, iterEndList)) {
		// choose a file to read (with smallest key)
		size_t index = getIterWithMinKey(iterList, iterEndList);

		// read
		inFileList[index]->seekg(iterList[index]->offset); 
		*(inFileList[index]) >> std::noskipws >> test;
		if (test == ' ') {
			value = "";
		}
		else {
			inFileList[index]->seekg(iterList[index]->offset);
			*(inFileList[index]) >> value;
		}
		
		// write
		if (!cacheFileList[indexOut]->cache.empty()) {
			if (iterList[index]->key == cacheFileList[indexOut]->cache.back().key) {
				iterList[index]++;
				continue;
			}
		}

		*outFileList[indexOut] << iterList[index]->key << " ";
		offset = outFileList[indexOut]->tellp();
		*outFileList[indexOut] << value << " ";

		// update size, cache, iter
		curFileSize += (value.size() + 16);
		cacheFileList[indexOut]->cache.push_back(Pair(iterList[index]->key, offset));
		
		iterList[index] += 1;

		// create another file if too big or reach end
		if (curFileSize > MAX_FILE_SIZE){
			if (ReachEnd(iterList, iterEndList)) break;

			// record file
			finishOutFile(curLevel + 1, outFileList[indexOut], cacheFileList[indexOut]);
		
			// update size, offset			

			std::string filename;
			std::ofstream* outfile = createOutFile(curLevel + 1, filename); 
			outFileList.push_back(outfile);

			SSTable* cacheFile = new SSTable(filename, 0, 0, 0);
			cacheFileList.push_back(cacheFile);

			indexOut++;
			curFileSize = 0;
			offset = 0;	
		}
	}

	finishOutFile(curLevel + 1, outFileList[indexOut], cacheFileList[indexOut]);

	for (size_t i = 0; i < outFileList.size(); i++) {
		outFileList[i]->close();
		delete outFileList[i];
	}

	for (size_t i = 0; i < fileCount; i++) {
		inFileList[i]->close();
		delete inFileList[i];
		LevelList[curLevel]->removeFile(filesToMerge[i]->filename);
	}
}

bool DiskInfo::ReachEnd(std::vector<std::vector<Pair>::iterator> indexList, std::vector<std::vector<Pair>::iterator> indexEndList) {
	for (size_t i = 0; i < indexList.size(); ++i) {
		if (indexList[i] != indexEndList[i])
			return false;
	}
	return true;
}

size_t DiskInfo::getIterWithMinKey(std::vector<std::vector<Pair>::iterator> iterList, std::vector<std::vector<Pair>::iterator> indexEndList) {
	std::vector<size_t> able;
	for (size_t i = 0; i < iterList.size(); ++i) {
		if (iterList[i] != indexEndList[i]) {
			able.push_back(i);
		}			
	}
	size_t index = able[0];
	for (size_t i = 1; i < able.size(); ++i) {
		if (iterList[able[i]]->key < iterList[index]->key)
			index = able[i];
		else if (iterList[able[i]]->key == iterList[index]->key) {
			iterList[able[i]]++;
		}
	}
	return index;
}

std::ofstream* DiskInfo::createOutFile(size_t Level, std::string& filename){
	filename = timeStamp.getTimeStamp();
	std::string path = "./dir/level" + ToString(Level);
	if (!fs::exists(path)) {
		fs::create_directories(path);
	}
	std::ofstream* outfile = new std::ofstream(path + "/" + filename + ".txt", std::ios::binary);
	if (!outfile) {
		std::cerr << "Failed to open the file!";
		exit(0);
	}
	return outfile;
}

void DiskInfo::finishOutFile(size_t level, std::ofstream* outfile, SSTable* cacheFile) {
	//update cache file
	cacheFile->minKey = cacheFile->cache.front().key;
	cacheFile->maxKey = cacheFile->cache.back().key;
	cacheFile->divide = outfile->tellp();
	//if this level doesn't exist, create it
	newLevel(level);
	LevelList[level]->addFile(cacheFile);	
	//write key-offset pair
	for (auto iter = cacheFile->cache.begin(); iter != cacheFile->cache.end(); iter++) {
		*outfile << (*iter).key << " " << (*iter).offset << " ";
	}
	*outfile << cacheFile->divide;
	outfile->close();
}

V* DiskInfo::get(uint64_t key) {
	V* value;
	if (LevelList.empty())
		load();
	for (size_t i = 0; i < LevelList.size(); i++){
		value = LevelList[i]->get(key);
		if (value) return value;
	}
	return nullptr;
}

V* LevelInfo::get(uint64_t key) {
	V* value;
	for (size_t i = 0; i < fileList.size(); i++)
	{
		value = fileList[i]->search(key, level);
		if (value) return value;
	}
	return nullptr;
}

V* SSTable::search(K key, size_t level) {
	if (key > maxKey || key < minKey)return nullptr;
	auto iter = std::find_if(cache.begin(), cache.end(), [key](Pair const& n) {
		return n.key == key;
	});
	if (iter == cache.end())return nullptr;
	
	auto offset = iter->offset;
	std::ifstream infile("./dir/level" + ToString(level) + "/" + filename + ".txt", std::ios::binary);
	infile.seekg(offset);
	
	V v;char test;
	infile >> std::noskipws >> test;
	if (test == ' ') {
		v = "";
	}else {
		infile.seekg(offset);
		infile >> v;
	}
	
	V* value = new V(v);
	return value;
}

void DiskInfo::load() {
	size_t level = 0;
	while (1) {
		fs::path dir_path = "./dir/level" + ToString(level);
		if (!exists(dir_path)) return;
		for (const auto& filename : recursive_directory_iterator(dir_path)) {
			loadCache(filename.path());
		}
		level++;
	}
}

void DiskInfo::loadCache(fs::path filepath)
{
	std::ifstream infile(filepath, std::ios::binary);

}