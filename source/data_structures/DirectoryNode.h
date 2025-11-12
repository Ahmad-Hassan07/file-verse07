#pragma once
#include <vector>
#include "AVLTree.h"

struct DirChild {
    char name[12];
    unsigned int entry_index;
};

class DirectoryNode {
    char name[12];
    unsigned int entry_index;
    DirectoryNode* parent;
    AVLTree<unsigned long long, DirChild> children;
    unsigned long long key_of(const char n[12]) const {
        unsigned long long h = 1469598103934665603ull;
        for (int i = 0; i < 12; i++) { unsigned char b = (unsigned char)n[i]; h ^= b; h *= 1099511628211ull; }
        return h;
    }
public:
    DirectoryNode(): entry_index(0), parent(nullptr) { for (int i = 0; i < 12; i++) name[i] = 0; }
    DirectoryNode(const char n[12], unsigned int idx, DirectoryNode* p): entry_index(idx), parent(p) {
        for (int i = 0; i < 12; i++) name[i] = n[i];
    }
    unsigned int index() const { return entry_index; }
    DirectoryNode* get_parent() const { return parent; }
    bool add_child(const char child_name[12], unsigned int child_index) {
        DirChild d;
        for (int i = 0; i < 12; i++) d.name[i] = child_name[i];
        d.entry_index = child_index;
        unsigned long long k = key_of(child_name);
        children.insert(k, d);
        return true;
    }
    bool remove_child(const char child_name[12]) {
        unsigned long long k = key_of(child_name);
        return children.erase(k);
    }
    bool has_child(const char child_name[12]) const {
        unsigned long long k = key_of(child_name);
        DirChild d;
        return children.get(k, d);
    }
    bool get_child(const char child_name[12], DirChild& out) const {
        unsigned long long k = key_of(child_name);
        return children.get(k, out);
    }
    std::vector<DirChild> list_children() const {
        std::vector<DirChild> v;
        std::vector<unsigned long long> ks = children.inorder_keys();
        for (unsigned long long i = 0; i < ks.size(); i++) {
            DirChild d;
            if (children.get(ks[i], d)) v.push_back(d);
        }
        return v;
    }
    void get_name(char out[12]) const {
        for (int i = 0; i < 12; i++) out[i] = name[i];
    }
};
