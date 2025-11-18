#include "BlockManager.h"
#include <iostream>

bool BlockManager::init(void* file, uint64_t data_off,
                        uint32_t blk_size, uint32_t blk_count,
                        FreeSpaceManager* free_mgr)
{
    base = file;
    data_offset = data_off;
    block_size = blk_size;
    block_count = blk_count;
    fsm = free_mgr;
    return true;
}

inline uint8_t* block_ptr(void* base, uint64_t data_off,
                          uint32_t block_size, uint32_t blk)
{
    return ((uint8_t*)base) + data_off + blk * block_size;
}

int BlockManager::allocate_block() {
    int blk = fsm->allocate_block();
    if (blk < 0) return -1;

    uint8_t* ptr = block_ptr(base, data_offset, block_size, blk);
    memset(ptr, 0, block_size);

    // mark chain end
    *(uint32_t*)ptr = 0xFFFFFFFF;
    return blk;
}

void BlockManager::free_block_chain(uint32_t start) {
    uint32_t blk = start;
    while (blk != 0xFFFFFFFF) {
        uint32_t next = get_next(blk);
        fsm->free_block(blk);
        blk = next;
    }
}

bool BlockManager::read_block(uint32_t blk, void* out) {
    if (blk >= block_count) return false;
    uint8_t* ptr = block_ptr(base, data_offset, block_size, blk);
    memcpy(out, ptr, block_size);
    return true;
}

bool BlockManager::write_block(uint32_t blk, const void* in) {
    if (blk >= block_count) return false;
    uint8_t* ptr = block_ptr(base, data_offset, block_size, blk);
    memcpy(ptr, in, block_size);
    return true;
}

uint32_t BlockManager::get_next(uint32_t blk) {
    uint8_t* ptr = block_ptr(base, data_offset, block_size, blk);
    return *(uint32_t*)ptr;
}

void BlockManager::set_next(uint32_t blk, uint32_t next) {
    uint8_t* ptr = block_ptr(base, data_offset, block_size, blk);
    *(uint32_t*)ptr = next;
}

int BlockManager::write_file(uint32_t start, uint64_t off,
                             const uint8_t* data, uint64_t len)
{
    uint64_t pos = off;
    uint64_t remaining = len;
    uint32_t blk = start;

    uint32_t header = 4;
    uint32_t usable = block_size - header;

    // skip blocks until the correct block
    while (pos >= usable) {
        pos -= usable;
        uint32_t next = get_next(blk);
        if (next == 0xFFFFFFFF) {
            // allocate new block
            int newblk = allocate_block();
            if (newblk < 0) return -1;
            set_next(blk, newblk);
            blk = newblk;
        } else {
            blk = next;
        }
    }

    while (remaining > 0) {
        uint8_t* ptr = block_ptr(base, data_offset, block_size, blk);

        uint64_t write_here = std::min<uint64_t>(usable - pos, remaining);
        memcpy(ptr + 4 + pos, data, write_here);

        data += write_here;
        remaining -= write_here;

        pos = 0;

        if (remaining > 0) {
            uint32_t next = get_next(blk);
            if (next == 0xFFFFFFFF) {
                int newblk = allocate_block();
                if (newblk < 0) return -1;
                set_next(blk, newblk);
                blk = newblk;
            } else {
                blk = next;
            }
        }
    }

    return 1;
}

int BlockManager::read_file(uint32_t start, uint64_t off,
                            uint8_t* out, uint64_t len)
{
    uint64_t pos = off;
    uint64_t remaining = len;
    uint32_t blk = start;

    uint32_t header = 4;
    uint32_t usable = block_size - header;

    // skip blocks
    while (pos >= usable) {
        pos -= usable;
        blk = get_next(blk);
        if (blk == 0xFFFFFFFF) return -1;
    }

    while (remaining > 0) {
        uint8_t* ptr = block_ptr(base, data_offset, block_size, blk);

        uint64_t read_here = std::min<uint64_t>(usable - pos, remaining);
        memcpy(out, ptr + 4 + pos, read_here);

        out += read_here;
        remaining -= read_here;

        pos = 0;

        if (remaining > 0) {
            blk = get_next(blk);
            if (blk == 0xFFFFFFFF) return -1;
        }
    }

    return 1;
}
