#pragma once
#include <vector>
#include "SinglyLinkedList.h"

template <typename T>
class Queue {
    SinglyLinkedList<T> list;
public:
    Queue() {}
    bool empty() const { return list.size() == 0; }
    unsigned long long size() const { return list.size(); }
    void enqueue(const T& v) { list.push_back(v); }
    bool dequeue(T& out) { return list.pop_front(out); }
    bool front(T& out) const { return list.front(out); }
    bool back(T& out) const { return list.back(out); }
};
