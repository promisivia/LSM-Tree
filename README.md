



# LSM-Tree

The log-structured merge-tree (or LSM tree) is a data structure with performance characteristics that make it attractive for providing indexed access to files with high insert volume.

## Files Structures

```C
·
├── data      	   		// Data directory
├── kvstore.cc     		// kvstore
├── kvstore.h      		// kvstore
├── kvstore_api.h  		// KVStoreAPI
├──MemTable
   ├── list.h			// a simple list
   ├── quadlistnode.h	// the node for skiplist
   ├── quadlist.h		// the list to store quadlistnode
   └── skiplist.h		// the skip list 
├── disk.cc        		// disk part
├── disk.h         		// disk part 
├── correctness.cc 		// Correctness test
├── persistence.cc 		// Persistence test
└── test.h         		// Base class for testing
```

## Building

To test the correctness

```c++
g++ -o LSM test.h correctness.cc kvstore_api.h kvstore.h kvstore.cc disk.cc disk.h ".\MemTable\list.h" ".\MemTable\quadlist.h" ".\MemTable\quadlistnode.h" ".\MemTable\skiplist.h" -std=c++17
```

To test the persistence

```c++
g++ -o LSM test.h persistence.cc kvstore_api.h kvstore.h kvstore.cc disk.cc disk.h ".\MemTable\list.h" ".\MemTable\quadlist.h" ".\MemTable\quadlistnode.h" ".\MemTable\skiplist.h" -std=c++17
```

## Design and Test

see in document.pdf

