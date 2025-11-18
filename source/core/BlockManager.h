#pragma once
#include <cstdint>
#include <vector>
#include <cstring>

#include "FreeSpaceManager.h"

class BlockManager {
private:
    void* base;
    uint64_t data_offset;
    uint32_t block_size;
    uint32_t block_count;

    FreeSpaceManager* fsm;

public:
    BlockManager() {
        base = nullptr;
        data_offset = 0;
        block_size = 0;
        block_count = 0;
        fsm = nullptr;
    }

    bool init(void* file, uint64_t data_off, uint32_t blk_size, uint32_t blk_count,
              FreeSpaceManager* free_mgr);

    // block operations
    int allocate_block();
    void free_block_chain(uint32_t start);

    // read/write block
    bool read_block(uint32_t blk, void* out);
    bool write_block(uint32_t blk, const void* in);

    // file chain helpers
    uint32_t get_next(uint32_t blk);
    void set_next(uint32_t blk, uint32_t next);

    // read/write file content
    int write_file(uint32_t start, uint64_t offset, const uint8_t* data, uint64_t len);
    int read_file(uint32_t start, uint64_t offset, uint8_t* out, uint64_t len);
};
