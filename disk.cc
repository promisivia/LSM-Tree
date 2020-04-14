#include "disk.h"

void LevelInfo::removeFile( string filename) {
	auto iter = fileList.begin();
	while (iter != fileList.end()) {
		if ((*iter)->filename == filename) {
			iter = fileList.erase(iter);
			string path = "./data/level" +  to_string(level) + "/" + filename + ".txt";
			remove((char*)path.data());
		}
		else iter++;
	}
}

bool LevelInfo::getFilePath(string filename, string& path) {
	auto iter = fileList.begin();
	while (iter != fileList.end()) {
		if ((*iter)->filename == filename) {
			path = "./data/level" + to_string(level) + "/" + filename + ".txt";
			return true;
		}
		iter++;
	}
	return false;
}

vector<SSTable*> LevelInfo::pickFiles() {
	vector<SSTable*> pickedFiles;
	sort(fileList.begin(), fileList.end(), [](const SSTable* a, const SSTable* b) {
		return stoll(a->filename) < stoll(b->filename);
	});
	for (size_t i = capacity; i < fileList.size(); i++) {
		pickedFiles.push_back(fileList[i]);
	}	
	return pickedFiles;
}

bool LevelInfo::isSorted() {
	for (size_t i = 0; i < fileList.size();i++) {
		for (size_t j = i+1; j < fileList.size(); j++) {
			if (fileList[i]->maxKey >= fileList[j]->minKey && fileList[i]->minKey <= fileList[j]->maxKey)
				return false;
		}
	}
	return true;
}

bool DiskInfo::newLevel(size_t level) {
	bool create = false;

	if (LevelList.size() <= level) {
		vector<SSTable*> fileList;
		LevelList.push_back(new LevelInfo(level, fileList));
		create = true;
	}	
	if (!fs::exists("./data/level" + to_string(level))) {
		fs::create_directory("./data/level" + to_string(level));
	}
	return create;
}

dK DiskInfo::findMaxKey(vector<SSTable*> fileList) {
	dK res = fileList[0]->maxKey;
	for (auto &c : fileList) {
		if (c->maxKey > res)
			res = c->maxKey;
	}
	return res;
}

dK DiskInfo::findMinKey(vector<SSTable*> fileList) {
	dK res = fileList[0]->minKey;
	for (auto c : fileList) {
		if (c->minKey < res)
			res = c->minKey;
	}
	return res;
}

vector<SSTable*> DiskInfo::SelectNextLevel(size_t level, vector<SSTable*> filesToMove) {
	
	vector<SSTable*> filesSelected;
	dK minKey = findMinKey(filesToMove);
	dK maxKey = findMaxKey(filesToMove);
	for (auto &c:LevelList[level]->fileList){
		if (!(c->minKey > maxKey || c->maxKey < minKey)){
			filesSelected.push_back(c);
		}
	}
	return filesSelected;
}

vector<SSTable*> DiskInfo::filterFiles(vector<SSTable*> filesSelected, vector<SSTable*>& filesToMove) {
	vector<SSTable*> filesFiltered;
	dK minKey = findMinKey(filesSelected);
	dK maxKey = findMaxKey(filesSelected);
	auto iter = filesToMove.begin();
	while (iter != filesToMove.end()) {
		if (!((*iter)->minKey > maxKey || (*iter)->maxKey < minKey)) {
			filesFiltered.push_back((*iter));
			iter = filesToMove.erase(iter);
		}else iter++;
	}
	return filesFiltered;
}

void DiskInfo::moveFiles(vector<SSTable*> filelist, size_t curl, size_t tarl) {
	string name, oldP, newP;
	for (size_t i = 0; i < filelist.size(); i++)
	{
		name = filelist[i]->filename;
		// disk
		oldP = "./data/level" + to_string(curl) + "/" + name + ".txt";
		newP = "./data/level" + to_string(tarl) + "/" + name + ".txt";
		fs::rename(oldP, newP);
		// memory
		LevelList[tarl]->addFile(filelist[i]);
		for (auto iter = LevelList[curl]->fileList.begin(); iter != LevelList[curl]->fileList.end(); iter++) {
			if ((*iter)->filename == name) {
				LevelList[curl]->fileList.erase(iter);	
				break;
			}
		}		
	}
}

void DiskInfo::AddLevel(size_t level, vector<SSTable*> filesToMove)
{
	size_t tar_l = level + 1;
	//change
	for (size_t l = LevelList.size()-1; l > level; l--) {
		LevelList[l]->level += 1;
		LevelList[l]->capacity *= 2;
		fs::rename("./data/level" + to_string(l), "./data/level" + to_string(l+1));
	}
	//add
	vector<SSTable*> fileList;
	LevelList.insert(LevelList.begin()+ tar_l, new LevelInfo(tar_l, fileList));
	fs::create_directory("./data/level" + to_string(tar_l));
	//move
	moveFiles(filesToMove, level, tar_l);
}

void DiskInfo::Compaction(size_t levelNow) {
	if (!LevelList[levelNow]->isFull()) return;
	newLevel(levelNow + 1);

	//select files
	vector<SSTable*> filesToMove;
	vector<SSTable*> filesToMerge;

	if (levelNow == 0) {
		filesToMove = LevelList[0]->fileList;
		if (!empty(1)) 
			filesToMerge = SelectNextLevel(1, filesToMove);
		if (LevelList[0]->isSorted()) { //下层为空，本层有序 or 下层不为空但没有选到
			if ( empty(1) || (!empty(1) && filesToMerge.empty())) {
				moveFiles(filesToMove, 0, 1);
				Compaction(1);
				return;
			}
		}
		//其他情况都需要merge
		filesToMerge.insert(filesToMerge.end(), filesToMove.begin(), filesToMove.end());
		sort(filesToMerge.begin(), filesToMerge.end(),[](const SSTable* a, const SSTable* b) {
			return stoll(a->filename) > stoll(b->filename);
		});
		mergeFiles(filesToMerge, 0);
		Compaction(1);
		return;
	}		
	else {
		filesToMove = LevelList[levelNow]->pickFiles();
		if (empty(levelNow + 1)) 
			moveFiles(filesToMove, levelNow, levelNow + 1);
		else { //如果下一层存在文件，去下一层选取文件
			filesToMerge = SelectNextLevel(levelNow + 1, filesToMove);
			if (filesToMerge.empty()) {//如果没有选到
				moveFiles(filesToMove, levelNow, levelNow + 1);
			}else {//如果选到了
				if (filesToMerge.size() + filesToMerge.size() > 5) {
					AddLevel(levelNow, filesToMove);
				}
				else {
					filesToMerge.insert(filesToMerge.end(), filesToMove.begin(), filesToMove.end());				
					sort(filesToMerge.begin(), filesToMerge.end(),[](const SSTable* a, const SSTable* b) {
						return stoll(a->filename) > stoll(b->filename);
					});
					mergeFiles(filesToMerge, levelNow);	
				}							
			}
		}			
		if (LevelList[levelNow]->isFull())Compaction(levelNow);
		else Compaction(levelNow + 1);
		return;
	}
}

void DiskInfo::mergeFiles(vector<SSTable*> filesToMerge, size_t curLevel)
{
	size_t fileCount = filesToMerge.size();
	vector<vector<Pair>::iterator> iterList;
	vector<vector<Pair>::iterator> iterEndList;
	vector<ifstream*> inFileList;
	vector<ofstream*> outFileList;
	vector<SSTable*> cacheFileList;

	// init list of iter and list of files to read
	for (size_t i = 0; i < fileCount; i++)
	{
		iterList.push_back(filesToMerge[i]->cache.begin());
		iterEndList.push_back(filesToMerge[i]->cache.end());

		string path;
		ifstream* infile;
		if(LevelList[curLevel]->getFilePath(filesToMerge[i]->filename,path))
			infile = new ifstream(path, ios::binary);
		else 
			infile = new ifstream("./data/level" + to_string(curLevel+1) + "/" + filesToMerge[i]->filename + ".txt", ios::binary);
		inFileList.push_back(infile);
	}
	
	// init files to write
	string filename;size_t indexOut = 0;
	ofstream* outfile = createOutFile(curLevel + 1, filename);
	outFileList.push_back(outfile);

	// init the detail of SSTable
	size_t curFileSize = 0;
	dV value; dK lastKey = -1;
	auto offset = outfile->tellp(); //offset
	SSTable* cacheFile = new SSTable(filename, 0, 0, 0); 
	cacheFileList.push_back(cacheFile);	

	// merge infiles to outfiles	
	while (!ReachEnd(iterList, iterEndList)) {
		// choose a file to read (with smallest key)
		size_t index = getIterWithMinKey(iterList, iterEndList);
		// read
		inFileList[index]->seekg(iterList[index]->offset); 
		if (inFileList[index]->peek() == TOMBSTONE)value = "";
		else {
			uint64_t length;
			if (iterList[index] + 1 != iterEndList[index]) {
				auto temp = iterList[index] + 1;
				length = temp->offset - iterList[index]->offset - 2 - to_string(temp->key).length();
			}
			else {
				length = int(filesToMerge[index]->divide) - iterList[index]->offset - 1;
			}
			char* c = new char[length+1];
			inFileList[index]->read(c, length);
			c[length] = '\0';
			value = c;
			delete c;
		}
		
		// write
		if (iterList[index]->key == lastKey) {
			iterList[index]++;continue;
		}
		lastKey = iterList[index]->key;
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
			// update	
			ofstream* outfile = createOutFile(curLevel + 1, filename); 
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
		LevelList[curLevel+1]->removeFile(filesToMerge[i]->filename);
	}
}

bool DiskInfo::ReachEnd(vector<vector<Pair>::iterator> indexList, vector<vector<Pair>::iterator> indexEndList) {
	for (size_t i = 0; i < indexList.size(); ++i) {
		if (indexList[i] != indexEndList[i])
			return false;
	}
	return true;
}

size_t DiskInfo::getIterWithMinKey(vector<vector<Pair>::iterator> iterList, vector<vector<Pair>::iterator> indexEndList) {
	vector<size_t> able;
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

ofstream* DiskInfo::createOutFile(size_t Level, string& filename){
	filename = timeStamp.getTimeStamp();
	string path = "./data/level" + to_string(Level);
	if (!fs::exists(path)) {
		fs::create_directories(path);
	}
	ofstream* outfile = new ofstream(path + "/" + filename + ".txt", ios::binary);
	if (!outfile) {
		cerr << "Failed to open the file!";
		exit(0);
	}
	return outfile;
}

void DiskInfo::finishOutFile(size_t level, ofstream* outfile, SSTable* cacheFile) {
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

dV* DiskInfo::get(uint64_t key) {
	dV* value;
	if (LevelList.empty())
		load();
	for (size_t i = 0; i < LevelList.size(); i++){
		value = LevelList[i]->get(key);
		if (value) return value;
	}
	return nullptr;
}

dV* LevelInfo::get(uint64_t key) {
	dV* value;
	for (size_t i = 0; i < fileList.size(); i++)
	{
		value = fileList[i]->search(key, level);
		if (value) return value;
	}
	return nullptr;
}

dV* SSTable::search(dK key, size_t level) {
	if (key > maxKey || key < minKey)return nullptr;
	auto iter = find_if(cache.begin(), cache.end(), [key](Pair const& n) {
		return n.key == key;
	});
	if (iter == cache.end())return nullptr;
	
	auto offset = iter->offset;
	ifstream infile("./data/level" + to_string(level) + "/" + filename + ".txt", ios::binary);
	infile.seekg(offset);
	dV v;
	if (infile.peek() == TOMBSTONE)  v = "";
	else {
		uint64_t length;
		if (iter + 1 != cache.end()) {
			auto temp = iter + 1;
			length = temp->offset - iter->offset - 2 - to_string(temp->key).length();
		}
		else {
			length = int(divide) - iter->offset - 1;
		}
		char* c = new char[length+1];
		infile.read(c, length);
		c[length] = '\0';
		v = c;
		delete c;
	}
	
	dV* value = new dV(v);
	return value;
}

void DiskInfo::load() {
	size_t level = 0;
	while (1) {
		fs::path dir_path = "./data/level" + to_string(level);
		if (!fs::exists(dir_path)) return;
		// file list for this level
		vector<SSTable*> fileList;
		// for every file, load information
		for (const auto& file : recursive_directory_iterator(dir_path)) {			
			SSTable* sst = loadFile(file.path(),level);
			fileList.push_back(sst);	
			timeStamp.updateTimeStamp(stoll(sst->filename) + 1);
		}
		LevelList.push_back(new LevelInfo(level, fileList)); 
		level++;
	}
}

SSTable* DiskInfo::loadFile(fs::path filePath, size_t level)
{
	auto filename = filePath.filename().string();	
	filename = filename.substr(0,filename.length()-4);
	ifstream infile(filePath, ios::binary);
	// read the divide
	infile.seekg(0, ios::end);
	while (infile.peek() != TOMBSTONE)
	{
		infile.seekg(-1, infile.cur);
	}
	auto end = infile.tellg();
	size_t divide;
	infile >> divide;
	SSTable* sst = new SSTable(filename, 0, 0, divide);
	// read the cache
	infile.seekg(divide);
	dK key; dK offset;
	while (infile.tellg()!= end) {
		infile >> key;
		infile >> offset;
		sst->cache.push_back(Pair(key, offset));
	}
	infile.close();
	sst->minKey = sst->cache.front().key;
	sst->maxKey = sst->cache.back().key;
	return sst;
}