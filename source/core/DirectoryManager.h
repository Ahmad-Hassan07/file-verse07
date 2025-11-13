#pragma once
#include <vector>
#include "../include/odf_types.hpp"
#include "../include/ofs_internal.h"
#include "../data_structures/DirectoryNode.h"
#include "../data_structures/HashTable.h"

class DirectoryManager {
    DirectoryNode* root;
    HashTable<unsigned long long, unsigned int> path_to_entry;
    unsigned long long key_of_path(const char* path) const {
        unsigned long long h = 1469598103934665603ull;
        unsigned long long i = 0;
        while (path && path[i]) { unsigned char b = (unsigned char)path[i]; h ^= b; h *= 1099511628211ull; i++; }
        return h;
    }
public:
    DirectoryManager(): root(nullptr) {}
    void set_root(DirectoryNode* r) { root = r; }
    bool index_path(const char* path, unsigned int entry_index) {
        unsigned long long k = key_of_path(path);
        return path_to_entry.put(k, entry_index);
    }
    bool resolve(const char* path, unsigned int& entry_index) const {
        unsigned long long k = key_of_path(path);
        return path_to_entry.get(k, entry_index);
    }
};
