#pragma once
#include <vector>
#include "../include/odf_types.hpp"
#include "../include/ofs_internal.h"
#include "BlockManager.h"
#include "FreeSpaceManager.h"
#include "SecurityManager.h"
#include "DirectoryManager.h"
#include "UserManager.h"

class FileSystem {
    std::vector<unsigned char> omni;
    std::vector<unsigned char> bitmap_area;
    std::vector<UserInfo> user_table_mem;
    OMNIHeader header;
    BlockManager block_mgr;
    FreeSpaceManager free_mgr;
    SecurityManager sec_mgr;
    DirectoryManager dir_mgr;
    UserManager user_mgr;
    MountLayout layout;
public:
    FileSystem(): header() {}
    bool load_empty(const FSConfig& cfg) {
        unsigned long long total = cfg.total_size;
        omni.resize(total);
        layout.user_table_offset = cfg.header_size;
        layout.user_table_size = cfg.max_users * sizeof(UserInfo);
        layout.meta_offset = layout.user_table_offset + layout.user_table_size;
        layout.meta_size = cfg.max_inodes * 72;
        layout.free_map_offset = layout.meta_offset + layout.meta_size;
        unsigned long long blocks = (total - layout.free_map_offset) / cfg.block_size;
        unsigned long long bitmap_bytes = (blocks + 7) / 8;
        layout.free_map_size = bitmap_bytes;
        layout.data_offset = layout.free_map_offset + layout.free_map_size;
        layout.data_size = total - layout.data_offset;
        bitmap_area.resize(layout.free_map_size);
        block_mgr.configure(&omni, cfg.block_size, layout.data_offset);
        free_mgr.bind(&bitmap_area, (unsigned int)blocks);
        return true;
    }
    BlockManager& blocks() { return block_mgr; }
    FreeSpaceManager& freemap() { return free_mgr; }
    SecurityManager& security() { return sec_mgr; }
    DirectoryManager& directory() { return dir_mgr; }
    UserManager& users() { return user_mgr; }
    OMNIHeader& hdr() { return header; }
    MountLayout& mount() { return layout; }
    std::vector<unsigned char>& omni_bytes() { return omni; }
    std::vector<unsigned char>& bitmap_bytes() { return bitmap_area; }
};
