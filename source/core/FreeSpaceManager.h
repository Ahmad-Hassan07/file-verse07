#pragma once
#include <vector>
#include "../include/odf_types.hpp"
#include "../include/ofs_internal.h"
#include "../data_structures/Stack.h"

class FreeSpaceManager {
    std::vector<unsigned char>* bitmap;
    unsigned int blocks;
    Stack<unsigned int> free_stack;
public:
    FreeSpaceManager(): bitmap(nullptr), blocks(0) {}
    void bind(std::vector<unsigned char>* bm, unsigned int total_blocks) {
        bitmap = bm;
        blocks = total_blocks;
        for (unsigned int i = 1; i <= blocks; i++) {
            unsigned int byte_i = (i - 1) >> 3;
            unsigned int bit_i = (i - 1) & 7;
            bool used = ((*bitmap)[byte_i] >> bit_i) & 1;
            if (!used) free_stack.push(i);
        }
    }
    bool mark_used(unsigned int i) {
        if (!bitmap || i == 0 || i > blocks) return false;
        unsigned int byte_i = (i - 1) >> 3;
        unsigned int bit_i = (i - 1) & 7;
        (*bitmap)[byte_i] = (unsigned char)((*bitmap)[byte_i] | (1u << bit_i));
        return true;
    }
    bool mark_free(unsigned int i) {
        if (!bitmap || i == 0 || i > blocks) return false;
        unsigned int byte_i = (i - 1) >> 3;
        unsigned int bit_i = (i - 1) & 7;
        (*bitmap)[byte_i] = (unsigned char)((*bitmap)[byte_i] & ~(1u << bit_i));
        free_stack.push(i);
        return true;
    }
    bool allocate_one(unsigned int& out) {
        if (free_stack.empty()) return false;
        unsigned int v = 0;
        free_stack.pop(v);
        if (!mark_used(v)) return false;
        out = v;
        return true;
    }
    std::vector<unsigned int> allocate_many(unsigned int n) {
        std::vector<unsigned int> v;
        for (unsigned int i = 0; i < n; i++) {
            unsigned int b = 0;
            if (!allocate_one(b)) break;
            v.push_back(b);
        }
        return v;
    }
};
