#pragma once
#include <vector>
#include "../include/odf_types.hpp"
#include "../include/ofs_internal.h"

class BlockManager {
    std::vector<unsigned char>* storage;
    unsigned long long block_size;
    unsigned long long data_offset;
public:
    BlockManager(): storage(nullptr), block_size(0), data_offset(0) {}
    void configure(std::vector<unsigned char>* s, unsigned long long blk, unsigned long long data_off) { storage = s; block_size = blk; data_offset = data_off; }
    unsigned long long size() const { return block_size; }
    bool read_block(unsigned int index, std::vector<unsigned char>& out) const {
        if (!storage || index == 0) return false;
        unsigned long long off = data_offset + (index - 1) * block_size;
        if (off + block_size > storage->size()) return false;
        out.resize(block_size);
        for (unsigned long long i = 0; i < block_size; i++) out[i] = (*storage)[off + i];
        return true;
    }
    bool write_block(unsigned int index, const std::vector<unsigned char>& in) {
        if (!storage || index == 0) return false;
        unsigned long long off = data_offset + (index - 1) * block_size;
        if (off + block_size > storage->size()) return false;
        unsigned long long n = in.size() < block_size ? in.size() : block_size;
        for (unsigned long long i = 0; i < n; i++) (*storage)[off + i] = in[i];
        for (unsigned long long i = n; i < block_size; i++) (*storage)[off + i] = 0;
        return true;
    }
    unsigned int next_of(const std::vector<unsigned char>& blk) const {
        if (blk.size() < 4) return 0;
        unsigned int v = 0;
        v |= (unsigned int)blk[0];
        v |= ((unsigned int)blk[1]) << 8;
        v |= ((unsigned int)blk[2]) << 16;
        v |= ((unsigned int)blk[3]) << 24;
        return v;
    }
    void set_next(std::vector<unsigned char>& blk, unsigned int next_index) const {
        if (blk.size() < 4) return;
        blk[0] = (unsigned char)(next_index & 0xff);
        blk[1] = (unsigned char)((next_index >> 8) & 0xff);
        blk[2] = (unsigned char)((next_index >> 16) & 0xff);
        blk[3] = (unsigned char)((next_index >> 24) & 0xff);
    }
};
