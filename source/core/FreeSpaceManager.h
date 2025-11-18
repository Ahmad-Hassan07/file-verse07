#pragma once
#include <cstdint>
#include <vector>
#include <cstring>

class FreeSpaceManager {
private:
    void* base;                // pointer to full fs image
    uint64_t offset;           // where bitmap starts
    uint32_t block_count;      // total blocks

public:
    FreeSpaceManager() {
        base = nullptr;
        offset = 0;
        block_count = 0;
    }

    bool init(void* file, uint64_t off, uint32_t blocks);

    // single block alloc/free
    int allocate_block();
    bool free_block(uint32_t idx);

    // allocate N blocks in a chain
    int allocate_chain(uint32_t n, std::vector<uint32_t>& out);
    bool free_chain(const std::vector<uint32_t>& chain);

    // check if block is used
    bool is_used(uint32_t idx) const;

private:
    inline uint8_t* ptr() const { return (uint8_t*)base + offset; }
    void set_used(uint32_t idx);
    void set_free(uint32_t idx);
    bool get(uint32_t idx) const;
};
