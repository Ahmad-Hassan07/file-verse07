#pragma once
#include <vector>

template <typename K, typename V>
class AVLTree {
    struct Node {
        K k;
        V v;
        int h;
        Node* l;
        Node* r;
        Node(const K& key, const V& val): k(key), v(val), h(1), l(nullptr), r(nullptr) {}
    };
    Node* root;
    int height(Node* n) const { return n ? n->h : 0; }
    int bal(Node* n) const { return n ? height(n->l) - height(n->r) : 0; }
    void upd(Node* n) { if (n) { int hl = height(n->l); int hr = height(n->r); n->h = (hl > hr ? hl : hr) + 1; } }
    Node* rot_r(Node* y) {
        Node* x = y->l;
        Node* t2 = x->r;
        x->r = y;
        y->l = t2;
        upd(y);
        upd(x);
        return x;
    }
    Node* rot_l(Node* x) {
        Node* y = x->r;
        Node* t2 = y->l;
        y->l = x;
        x->r = t2;
        upd(x);
        upd(y);
        return y;
    }
    Node* rebalance(Node* n) {
        upd(n);
        int b = bal(n);
        if (b > 1) {
            if (bal(n->l) < 0) n->l = rot_l(n->l);
            return rot_r(n);
        }
        if (b < -1) {
            if (bal(n->r) > 0) n->r = rot_r(n->r);
            return rot_l(n);
        }
        return n;
    }
    Node* insert_node(Node* n, const K& k, const V& v) {
        if (!n) return new Node(k, v);
        if (k < n->k) n->l = insert_node(n->l, k, v);
        else if (n->k < k) n->r = insert_node(n->r, k, v);
        else { n->v = v; return n; }
        return rebalance(n);
    }
    Node* min_node(Node* n) const {
        Node* cur = n;
        while (cur && cur->l) cur = cur->l;
        return cur;
    }
    Node* erase_node(Node* n, const K& k, bool& removed) {
        if (!n) return nullptr;
        if (k < n->k) n->l = erase_node(n->l, k, removed);
        else if (n->k < k) n->r = erase_node(n->r, k, removed);
        else {
            removed = true;
            if (!n->l || !n->r) {
                Node* t = n->l ? n->l : n->r;
                if (!t) {
                    delete n;
                    return nullptr;
                } else {
                    Node tmp = *t;
                    delete t;
                    n->k = tmp.k;
                    n->v = tmp.v;
                    n->l = tmp.l;
                    n->r = tmp.r;
                    n->h = tmp.h;
                }
            } else {
                Node* t = min_node(n->r);
                n->k = t->k;
                n->v = t->v;
                bool dummy = false;
                n->r = erase_node(n->r, t->k, dummy);
            }
        }
        return rebalance(n);
    }
    void inorder_collect(Node* n, std::vector<K>& out) const {
        if (!n) return;
        inorder_collect(n->l, out);
        out.push_back(n->k);
        inorder_collect(n->r, out);
    }
    void destroy(Node* n) {
        if (!n) return;
        destroy(n->l);
        destroy(n->r);
        delete n;
    }
    bool find_node(Node* n, const K& k, V& out) const {
        Node* cur = n;
        while (cur) {
            if (k < cur->k) cur = cur->l;
            else if (cur->k < k) cur = cur->r;
            else { out = cur->v; return true; }
        }
        return false;
    }
public:
    AVLTree(): root(nullptr) {}
    ~AVLTree() { destroy(root); }
    void insert(const K& k, const V& v) { root = insert_node(root, k, v); }
    bool erase(const K& k) { bool removed = false; root = erase_node(root, k, removed); return removed; }
    bool get(const K& k, V& out) const { return find_node(root, k, out); }
    bool contains(const K& k) const { V dummy; return find_node(root, k, dummy); }
    std::vector<K> inorder_keys() const { std::vector<K> v; inorder_collect(root, v); return v; }
    bool empty() const { return root == nullptr; }
};
