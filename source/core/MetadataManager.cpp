#include "FreeSpaceManager.h"
#include <iostream>

void FreeSpaceManager::bind(std::vector<unsigned char>* bm, unsigned int total_blocks) {
    bitmap = bm;
    blocks = total_blocks;
    while (!free_stack.empty()) {
        unsigned int v;
        free_stack.pop(v);
    }
    for (unsigned int i = 1; i <= blocks; i++) {
        unsigned int byte_i = (i - 1) >> 3;
        unsigned int bit_i = (i - 1) & 7;
        bool used = ((*bitmap)[byte_i] >> bit_i) & 1u;
        if (!used) {
            free_stack.push(i);
        }
    }
    std::cout << "FreeSpaceManager bound with " << blocks << " blocks, free=" << free_stack.size() << std::endl;
}

bool FreeSpaceManager::mark_used(unsigned int i) {
    if (!bitmap) return false;
    if (i == 0 || i > blocks) return false;
    unsigned int byte_i = (i - 1) >> 3;
    unsigned int bit_i = (i - 1) & 7;
    (*bitmap)[byte_i] = (unsigned char)((*bitmap)[byte_i] | (1u << bit_i));
    return true;
}

bool FreeSpaceManager::mark_free(unsigned int i) {
    if (!bitmap) return false;
    if (i == 0 || i > blocks) return false;
    unsigned int byte_i = (i - 1) >> 3;
    unsigned int bit_i = (i - 1) & 7;
    (*bitmap)[byte_i] = (unsigned char)((*bitmap)[byte_i] & ~(1u << bit_i));
    free_stack.push(i);
    return true;
}

bool FreeSpaceManager::allocate_one(unsigned int& out) {
    std::cout << "Attempting to allocate a free block..." << std::endl;
    if (free_stack.empty()) {
        std::cout << "No free blocks left!" << std::endl;
        return false;
    }
    unsigned int v = 0;
    free_stack.pop(v);
    std::cout << "Allocated block: " << v << std::endl;
    if (!mark_used(v)) return false;
    out = v;
    return true;
}
