#pragma once
#include <vector>
#include "odf_types.hpp"

struct FSConfig {
    uint64_t total_size;
    uint64_t header_size;
    uint64_t block_size;
    uint32_t max_users;
    uint32_t max_inodes;
    char student_id[32];
    char submission_date[16];
    char admin_username[32];
    char admin_password[32];
    char private_key[64];
    uint32_t require_auth;
    uint32_t server_port;
    uint32_t max_connections;
    FSConfig() {
        total_size = 0;
        header_size = 512;
        block_size = 0;
        max_users = 0;
        max_inodes = 0;
        require_auth = 0;
        server_port = 0;
        max_connections = 0;
        for (int i = 0; i < 32; i++) {
            student_id[i] = 0;
            submission_date[i < 16 ? i : 0] = submission_date[i < 16 ? i : 0];
            admin_username[i] = 0;
            admin_password[i] = 0;
        }
        for (int i = 0; i < 16; i++) submission_date[i] = 0;
        for (int i = 0; i < 64; i++) private_key[i] = 0;
    }
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
