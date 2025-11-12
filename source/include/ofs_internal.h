#pragma once
#include <vector>
#include "odf_types.hpp"

struct FSConfig {
    uint64_t total_size;
    uint64_t block_size;
    uint32_t max_users;
    uint32_t max_inodes;
    char student_id[32];
    char submission_date[16];
};

struct ByteSpan {
    uint64_t offset;
    uint64_t size;
};

struct BlockSpan {
    uint32_t start;
    uint32_t count;
};

struct FreeSpan {
    uint32_t start;
    uint32_t count;
};

struct PathIndex {
    uint32_t parent_inode;
    uint32_t inode;
};

struct OpenFileHandle {
    uint32_t inode;
    uint64_t position;
};

struct MountLayout {
    uint64_t user_table_offset;
    uint64_t user_table_size;
    uint64_t free_map_offset;
    uint64_t free_map_size;
    uint64_t meta_offset;
    uint64_t meta_size;
    uint64_t data_offset;
    uint64_t data_size;
};

struct SessionState {
    SessionInfo session;
    uint32_t mounted;
};
