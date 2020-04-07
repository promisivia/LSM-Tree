#pragma once

#include <cstdint>
#include <string>
#define K uint64_t 
#define V std::string
#define NodePos QuadlistNode*

struct Entry {
	K key;
	V value;
	Entry(K k = K(), V v = V()) :key(k), value(v) {};
	Entry(Entry const& e) :key(e.key), value(e.value) {};
	bool operator==(Entry const&e) { return key == e.key; }
	bool operator!=(Entry const&e) { return key != e.key; }
	K getKey() { return key; }
	V* getValue() { return &value; }
};

struct QuadlistNode
{
	Entry entry;
	NodePos pred; NodePos succ;
	NodePos above; NodePos below;
	QuadlistNode(Entry e = Entry(), NodePos p = NULL, NodePos s = NULL, NodePos a = NULL, NodePos b = NULL) :
		entry(e), pred(p), succ(s), above(a), below(b) {}
	NodePos insertAsSuccAbove(Entry const& e, NodePos b = NULL);
};

inline NodePos QuadlistNode::insertAsSuccAbove(Entry const &e, NodePos b) {
	NodePos x = new QuadlistNode(e, this, succ, NULL, b);
	succ->pred = x; succ = x;
	if (b) b->above = x;
	return x;
}
