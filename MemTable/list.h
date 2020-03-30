#pragma once
using namespace std;

template <typename T>
struct ListNode
{
	T data; 
	ListNode<T>* pred;
	ListNode<T>* succ;
	ListNode() {} 
	ListNode(T e, ListNode<T>* p = nullptr, ListNode<T>* s = nullptr)
		: data(e), pred(p), succ(s) {}
	ListNode<T>* insertAsPred(T const& e) {
		ListNode<T>* x = new ListNode(e, pred, this);
		pred->succ = x;pred = x;
		return x;
	}
	ListNode<T>* insertAsSucc(T const& e) {
		ListNode<T>* x = new ListNode(e, this, succ);
		succ->pred = x;succ = x;
		return x;
	}
};

template <typename T> class List
{
protected:
	int _size = 0;
	ListNode<T>* header = nullptr;
	ListNode<T>* tailor = nullptr;
	void init();
	int clear();
public:
	List() { init(); }
	List(List<T> const& L);
	~List(){ clear(); delete header; delete tailor; }
	int size() { return _size; }
	bool empty() const { return _size <= 0; }
	void pushFront(T const& e);
	void pushBack(T const& e);
	void insertBefore(ListNode<T>* p, T const& e);
	void insertAfter(ListNode<T>* p, T const& e);
	ListNode<T>* front() const { return header->succ; }
	ListNode<T>* back() const { return tailor->pred; }
	T remove(ListNode<T>* p);
};

template<typename T>
inline void List<T>::init() {
	header = new ListNode<T>;
	tailor = new ListNode<T>;
	header->succ = tailor; header->pred = nullptr;
	tailor->pred = header; tailor->succ = nullptr;
	_size = 0;
}

template<typename T>
inline List<T>::List(List<T> const& L) {
	copyNodes(L.first(), L._size); 
}

template<typename T>
inline void List<T>::pushFront(T const & e)
{
	_size++;
	header->insertAsSucc(e);
}

template<typename T>
inline void List<T>::pushBack(T const & e)
{
	_size++;
	tailor->insertAsPred(e);
}

template<typename T>
inline void List<T>::insertBefore(ListNode<T>* p, T const & e){
	_size++;
	p->insertAsPred(e);
}

template<typename T>
inline void List<T>::insertAfter(ListNode<T>* p, T const & e){
	_size++;
	p->insertAsSucc(e);
}

template<typename T>
inline T List<T>::remove(ListNode<T>* p) {
	T e = p->data;
	p->pred->succ = p->succ;
	p->succ->pred = p->pred;
	delete p; _size--;
	return e;
}

template<typename T>
inline int List<T>::clear() {
	int oldSize = _size;
	while (0 < _size)remove(header->succ);
	return oldSize;
}