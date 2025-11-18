#pragma once
#include <cstdint>
#include "odf_types.hpp"

struct FSConfig {
    uint64_t total_size;
    uint64_t header_size;
    uint64_t block_size;

    uint32_t max_users;
    uint32_t max_inodes;
    uint32_t max_files;              // <-- REQUIRED
    uint32_t max_filename_length;    // <-- REQUIRED

    uint32_t require_auth;
    uint32_t server_port;
    uint32_t max_connections;
    uint32_t queue_timeout;          // <-- REQUIRED

    char student_id[32];
    char submission_date[16];
    char admin_username[32];
    char admin_password[32];
    char private_key[64];

    FSConfig() {
        total_size = 0;
        header_size = 512;
        block_size = 0;

        max_users = 0;
        max_inodes = 0;
        max_files = 0;
        max_filename_length = 0;

        require_auth = 0;
        server_port = 0;
        max_connections = 0;
        queue_timeout = 0;

        memset(student_id, 0, sizeof(student_id));
        memset(submission_date, 0, sizeof(submission_date));
        memset(admin_username, 0, sizeof(admin_username));
        memset(admin_password, 0, sizeof(admin_password));
        memset(private_key, 0, sizeof(private_key));
    }
};

#pragma pack(push,1)
struct MetadataEntry {
    uint8_t valid_flag;
    uint8_t type_flag;
    uint32_t parent_index;
    char short_name[12];
    uint32_t start_index;
    uint64_t total_size;
    uint32_t owner;
    uint32_t permissions;
    uint64_t created_time;
    uint64_t modified_time;
    uint8_t reserved[18];
    MetadataEntry() {
        valid_flag = 0;
        type_flag = 0;
        parent_index = 0;
        for (int i = 0; i < 12; i++) short_name[i] = 0;
        start_index = 0;
        total_size = 0;
        owner = 0;
        permissions = 0;
        created_time = 0;
        modified_time = 0;
        for (int i = 0; i < 18; i++) reserved[i] = 0;
    }
};
#pragma pack(pop)

struct MountLayout {
    uint64_t user_table_offset;
    uint64_t user_table_size;
    uint64_t meta_offset;
    uint64_t meta_size;
    uint64_t free_map_offset;
    uint64_t free_map_size;
    uint64_t data_offset;
    uint64_t data_size;
};
