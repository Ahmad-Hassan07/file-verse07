#include "MetadataManager.h"
#include <cstring>
#include <iostream>

void MetadataManager::configure(BlockManager* bm, FreeSpaceManager* fm, unsigned long long start_block, unsigned long long metadata_size) {
    block_mgr = bm;
    free_mgr = fm;  // Store free_mgr to use in block allocation
    this->start_block = start_block;
    this->metadata_size = metadata_size;
}

bool MetadataManager::allocate_entry(MetadataEntry& entry) {
    std::vector<unsigned char> blk;
    unsigned int block_index = 0;
    
    // Use free_mgr to allocate a block
    if (!free_mgr->allocate_one(block_index)) return false;

    blk.resize(block_mgr->size());
    std::memset(blk.data(), 0, blk.size());
    entry.valid_flag = 1;  // Mark the entry as valid
    entry.start_index = block_index;

    // Write the entry into the block
    std::memcpy(blk.data(), &entry, sizeof(MetadataEntry));
    if (!block_mgr->write_block(block_index, blk)) return false;
    
    return true;
}

bool MetadataManager::write_entry(unsigned int index, const MetadataEntry& entry) {
    std::vector<unsigned char> blk;
    blk.resize(block_mgr->size());
    std::memset(blk.data(), 0, blk.size());
    std::memcpy(blk.data(), &entry, sizeof(MetadataEntry));

    return block_mgr->write_block(index, blk);
}

bool MetadataManager::read_entry(unsigned int index, MetadataEntry& entry) {
    std::vector<unsigned char> blk;
    if (!block_mgr->read_block(index, blk)) return false;
    std::memcpy(&entry, blk.data(), sizeof(MetadataEntry));
    return true;
}

bool MetadataManager::find_entry_by_name(const char* name, MetadataEntry& entry) {
    unsigned int index = start_block;
    while (index < metadata_size) {
        if (!read_entry(index, entry)) return false;
        if (std::strncmp(entry.short_name, name, 12) == 0) return true;
        index++;
    }
    return false;
}
