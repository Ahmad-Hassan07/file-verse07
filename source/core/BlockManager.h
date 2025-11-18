#pragma once
#include <vector>
#include <cstdint>

class BlockManager {
    std::vector<unsigned char>* omni;
    uint64_t block_size;
    uint64_t data_offset;
public:
    BlockManager() {
        omni = nullptr;
        block_size = 0;
        data_offset = 0;
    }
    void configure(std::vector<unsigned char>* buf, uint64_t blk_size, uint64_t data_off) {
        omni = buf;
        block_size = blk_size;
        data_offset = data_off;
    }
    uint64_t size() const {
        return block_size;
    }
    bool read_block(unsigned int index, std::vector<unsigned char>& out) {
        if (!omni) return false;
        if (index == 0) return false;
        uint64_t offset = data_offset + (uint64_t)(index - 1) * block_size;
        if (offset + block_size > omni->size()) return false;
        out.resize(block_size);
        for (uint64_t i = 0; i < block_size; i++) {
            out[(size_t)i] = (*omni)[(size_t)(offset + i)];
        }
        return true;
    }
    bool write_block(unsigned int index, const std::vector<unsigned char>& in) {
        if (!omni) return false;
        if (index == 0) return false;
        if (in.size() < block_size) return false;
        uint64_t offset = data_offset + (uint64_t)(index - 1) * block_size;
        if (offset + block_size > omni->size()) return false;
        for (uint64_t i = 0; i < block_size; i++) {
            (*omni)[(size_t)(offset + i)] = in[(size_t)i];
        }
        return true;
    }
};
