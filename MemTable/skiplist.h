#pragma once

#include "Quadlist.h"
#include "list.h"

#define prob 0.5 //生长概率
#define MAX_SIZE 2*1024*1024

class Skiplist: public List<Quadlist*>
{
private:
	int fileSize = 0;
protected:
	bool skipSearch(ListNode<Quadlist*>* &l, QuadlistNode* &n, K& k);
public:
	Skiplist() {};
	~Skiplist() {};
	int size() const { return empty() ? 0 : back()->data->size(); }
	int level() { return size(); }
	bool put(const K, const V);
	V* get(K key);
	bool remove(K key);
	QuadlistNode* getFirst() { return back()->data->front(); }
	QuadlistNode* getLast() { return back()->data->back(); }
	bool full() { return fileSize > MAX_SIZE; }
	void reset();
};

inline void Skiplist::reset() {
	fileSize = 0;
	clear();
}

inline bool Skiplist::skipSearch(ListNode<Quadlist*>* &qlist, QuadlistNode* &p, K& k) {
	while (true) {
		while (p->succ && (p->entry.key <= k)) {
			p = p->succ;
		}//直到到达更大的key或者p=tailor
		p = p->pred;
		if (p->pred && (k == p->entry.key))return true;//命中
		qlist = qlist->succ;//转入下一层
		if (!qlist->succ)return false;//已到底层
		if (p->pred) p = p->below;
		else p = qlist->data->front();//转至当前塔的下一节点
	}
}

inline bool Skiplist::put(K k, V v)
{
	if (full()) return false;//skiplist满了不插入
	Entry e = Entry(k,v);
	if (empty())
		pushFront(new Quadlist());
	ListNode<Quadlist*>* qlist = front();
	QuadlistNode* p = qlist->data->front();
	if (skipSearch(qlist, p, k)) {//查找合适的插入位置：不大于关键码k的最后一个节点p
		while (p->below) {
			p = p->below;//强制转入塔底		
		}
		while (p) {
			p->entry.value = v;
			p = p->above;
		}
		return true;
	}
	qlist = back();
	QuadlistNode* newList = qlist->data->insertAfterAbove(e, p);
	while (rand() & 1) {
		while (qlist->data->valid(p) && !p->above)p = p->pred;
		if (!qlist->data->valid(p)) {//若该前驱是header
			if (qlist == front()) {//当前层是最顶层
				pushFront(new Quadlist());//首先创建新的一层
			}
			p = qlist->pred->data->front()->pred;//p转至上一层skiplist的header
		}
		else {
			p = p->above;
		}
		qlist = qlist->pred;
		newList = qlist->data->insertAfterAbove(e, p, newList);//插入新节点
	}
	fileSize += v.size() + 16;
	return true;
}

inline V * Skiplist::get(K key)
{
	if (empty()) return nullptr;
	ListNode<Quadlist*>* qlist = front();//顶层Quadlist的
	QuadlistNode* p = qlist->data->front();//第一个节点开始
	if (skipSearch(qlist, p, key)) {
		return p->entry.getValue();
	}
	else return nullptr;
}

inline bool Skiplist::remove(K key)
{
	if (empty())return false;
	ListNode<Quadlist*>* qlist = front();//从顶层四联表的
	QuadlistNode* p = qlist->data->front();//首节点出发
	if (!skipSearch(qlist, p, key))return false;//目标词条不存在，直接返回
	do {
		QuadlistNode* lower = p->below;
		qlist->data->remove(p);//删除当前层节点
		p = lower; qlist = qlist->succ;//转入下一层
	} while (qlist->succ);//直到塔基
	while (!empty() && front()->data->empty()) {
		List::remove(front());//清除可能不含词条的顶层Quadlist
	}
	return true;
}
