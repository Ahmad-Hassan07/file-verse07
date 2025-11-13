#pragma once
#include <vector>
#include "SinglyLinkedList.h"

template <typename T>
class Stack {
    SinglyLinkedList<T> list;
public:
    Stack() {}
    bool empty() const { return list.size() == 0; }
    unsigned long long size() const { return list.size(); }
    void push(const T& v) { list.push_front(v); }
    bool pop(T& out) { return list.pop_front(out); }
    bool peek(T& out) const { return list.front(out); }
};
