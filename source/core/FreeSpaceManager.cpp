#include "FreeSpaceManager.h"
#include <iostream>

bool FreeSpaceManager::init(void* file, uint64_t off, uint32_t blocks) {
    base = file;
    offset = off;
    block_count = blocks;
    return true;
}

bool FreeSpaceManager::get(uint32_t idx) const {
    uint32_t byte = idx >> 3;
    uint32_t bit  = idx & 7;
    return (ptr()[byte] >> bit) & 1;
}

void FreeSpaceManager::set_used(uint32_t idx) {
    uint32_t byte = idx >> 3;
    uint32_t bit  = idx & 7;
    ptr()[byte] |= (1 << bit);
}

void FreeSpaceManager::set_free(uint32_t idx) {
    uint32_t byte = idx >> 3;
    uint32_t bit  = idx & 7;
    ptr()[byte] &= ~(1 << bit);
}

bool FreeSpaceManager::is_used(uint32_t idx) const {
    if (idx >= block_count) return true;
    return get(idx);
}

int FreeSpaceManager::allocate_block() {
    for (uint32_t i = 0; i < block_count; i++) {
        if (!get(i)) {
            set_used(i);
            return i;
        }
    }
    return -1;
}

bool FreeSpaceManager::free_block(uint32_t idx) {
    if (idx >= block_count) return false;
    set_free(idx);
    return true;
}

int FreeSpaceManager::allocate_chain(uint32_t n, std::vector<uint32_t>& out) {
    out.clear();
    for (uint32_t i = 0; i < n; i++) {
        int b = allocate_block();
        if (b < 0) {
            // rollback
            for (int r : out) free_block(r);
            out.clear();
            return -1;
        }
        out.push_back(b);
    }
    return (int)out.size();
}

bool FreeSpaceManager::free_chain(const std::vector<uint32_t>& chain) {
    for (uint32_t b : chain) {
        if (b >= block_count) return false;
        set_free(b);
    }
    return true;
}
