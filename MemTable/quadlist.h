#pragma once

#include "quadlistnode.h"

class Quadlist
{
private:
	int _size;
	NodePos header;
	NodePos tailor;//头尾哨兵
protected:
	void init();
	int clear();//清除所有节点
public:
	Quadlist() { init(); }
	~Quadlist() { clear(); delete header; delete tailor; }
	
	int size() const { return _size; }
	bool empty() const { return _size <= 0; }
	NodePos front() const { return header->succ; }
	NodePos back() const { return tailor->pred; }
	bool valid(NodePos p) { return p && (tailor != p) && (header != p); }//是否对外合法

	Entry remove(NodePos p);
	NodePos insertAfterAbove(Entry const& e, NodePos p, NodePos b = NULL);
	//将*e作为p的后继、b的上邻插入
};

inline void Quadlist::init() {
	_size = 0;
	header = new QuadlistNode();
	tailor = new QuadlistNode();
	header->succ = tailor; 
	tailor->pred = header;
	header->above = header->below = header->pred = nullptr;
	tailor->above = tailor->below = tailor->succ = nullptr;
}

inline int Quadlist::clear() {
	int oldSize = _size;
	while (_size > 0) remove(header->succ);
	return oldSize;
}

inline Entry Quadlist::remove(NodePos p)
{
	p->pred->succ = p->succ;
	p->succ->pred = p->pred;
	_size--;
	Entry e = p->entry;
	delete p;
	return e;
}

inline NodePos Quadlist::insertAfterAbove(Entry const & e, NodePos p, NodePos b)
{
	_size++;
	return p->insertAsSuccAbove(e, b);
}
