#include "FreeSpaceManager.h"
#include <iostream>

void FreeSpaceManager::bind(std::vector<unsigned char>* bm, unsigned int total_blocks) {
    bitmap = bm;
    blocks = total_blocks;

    // Iterate through all blocks and push free blocks to the free stack
    for (unsigned int i = 1; i <= blocks; i++) {
        unsigned int byte_i = (i - 1) >> 3;  // Calculate which byte
        unsigned int bit_i = (i - 1) & 7;   // Calculate which bit
        bool used = ((*bitmap)[byte_i] >> bit_i) & 1; // Check if the block is used
        if (!used) free_stack.push(i);  // If block is free, add to stack
    }
}

bool FreeSpaceManager::mark_used(unsigned int i) {
    if (!bitmap || i == 0 || i > blocks) return false;
    unsigned int byte_i = (i - 1) >> 3;
    unsigned int bit_i = (i - 1) & 7;
    (*bitmap)[byte_i] = (unsigned char)((*bitmap)[byte_i] | (1u << bit_i));  // Mark block as used
    return true;
}

bool FreeSpaceManager::mark_free(unsigned int i) {
    if (!bitmap || i == 0 || i > blocks) return false;
    unsigned int byte_i = (i - 1) >> 3;
    unsigned int bit_i = (i - 1) & 7;
    (*bitmap)[byte_i] = (unsigned char)((*bitmap)[byte_i] & ~(1u << bit_i));  // Mark block as free
    free_stack.push(i);  // Add to free stack
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
    if (!mark_used(v)) {
        std::cout << "Failed to mark block as used!" << std::endl;
        return false;
    }
    out = v;
    return true;
}
