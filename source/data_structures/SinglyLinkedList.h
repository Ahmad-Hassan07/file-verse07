#pragma once
#include <vector>

template <typename T>
class SinglyLinkedList {
    struct Node {
        T value;
        Node* next;
        Node(const T& v): value(v), next(nullptr) {}
    };
    Node* head;
    Node* tail;
    unsigned long long n;
public:
    SinglyLinkedList(): head(nullptr), tail(nullptr), n(0) {}
    ~SinglyLinkedList() { clear(); }
    bool empty() const { return n == 0; }
    unsigned long long size() const { return n; }
    void clear() {
        Node* cur = head;
        while (cur) {
            Node* nx = cur->next;
            delete cur;
            cur = nx;
        }
        head = nullptr;
        tail = nullptr;
        n = 0;
    }
    void push_front(const T& v) {
        Node* node = new Node(v);
        node->next = head;
        head = node;
        if (!tail) tail = node;
        n++;
    }
    void push_back(const T& v) {
        Node* node = new Node(v);
        if (!tail) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        n++;
    }
    bool pop_front(T& out) {
        if (!head) return false;
        Node* node = head;
        out = node->value;
        head = node->next;
        if (!head) tail = nullptr;
        delete node;
        n--;
        return true;
    }
    bool front(T& out) const {
        if (!head) return false;
        out = head->value;
        return true;
    }
    bool back(T& out) const {
        if (!tail) return false;
        out = tail->value;
        return true;
    }
    template <typename F>
    void for_each(F f) const {
        Node* cur = head;
        while (cur) {
            f(cur->value);
            cur = cur->next;
        }
    }
};
