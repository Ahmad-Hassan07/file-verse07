#pragma once
#include <vector>
#include "../include/odf_types.hpp"
#include "../include/ofs_internal.h"
#include "BlockManager.h"
#include "FreeSpaceManager.h"

class MetadataManager {
    BlockManager* block_mgr;
    FreeSpaceManager* free_mgr;  
    unsigned long long metadata_size;
    unsigned long long start_block;

public:
    MetadataManager() : block_mgr(nullptr), free_mgr(nullptr), metadata_size(0), start_block(0) {}

    void configure(BlockManager* bm, FreeSpaceManager* fm, unsigned long long start_block, unsigned long long metadata_size);
    
    bool allocate_entry(MetadataEntry& entry);
    bool write_entry(unsigned int index, const MetadataEntry& entry);
    bool read_entry(unsigned int index, MetadataEntry& entry);
    bool find_entry_by_name(const char* name, MetadataEntry& entry);
};
