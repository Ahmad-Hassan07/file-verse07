#include "FileSystem.h"
#include "../include/ofs_internal.h"
#include "../include/odf_types.hpp"
#include <fstream>
#include <cstring>

bool FileSystem::format_new(const FSConfig& cfg, const char* omni_path) {
    if (!omni_path) return false;
    if (cfg.total_size == 0 || cfg.block_size == 0 || cfg.max_users == 0 || cfg.max_inodes == 0) return false;
    uint64_t total = cfg.total_size;
    omni.assign(total, 0);
    layout.user_table_offset = cfg.header_size;
    layout.user_table_size = cfg.max_users * sizeof(UserInfo);
    layout.meta_offset = layout.user_table_offset + layout.user_table_size;
    layout.meta_size = cfg.max_inodes * sizeof(MetadataEntry);
    layout.free_map_offset = layout.meta_offset + layout.meta_size;
    uint64_t remaining = total - layout.free_map_offset;
    uint64_t blocks = remaining / cfg.block_size;
    if (blocks == 0) return false;
    uint64_t bitmap_bytes = (blocks + 7) / 8;
    layout.free_map_size = bitmap_bytes;
    layout.data_offset = layout.free_map_offset + layout.free_map_size;
    if (layout.data_offset > total) return false;
    layout.data_size = total - layout.data_offset;
    header = OMNIHeader(0x00010000u, total, cfg.header_size, cfg.block_size);
    std::memcpy(header.magic, "OMNIFS01", 8);
    std::memcpy(header.student_id, cfg.student_id, 32);
    std::memcpy(header.submission_date, cfg.submission_date, 16);
    header.user_table_offset = (uint32_t)layout.user_table_offset;
    header.max_users = cfg.max_users;
    header.file_state_storage_offset = 0;
    header.change_log_offset = 0;
    std::memcpy(omni.data(), &header, sizeof(OMNIHeader));
    unsigned char* user_ptr = omni.data() + layout.user_table_offset;
    std::memset(user_ptr, 0, layout.user_table_size);
    std::string admin_user(cfg.admin_username);
    std::string admin_pass(cfg.admin_password);
    UserInfo admin(admin_user, admin_pass, UserRole::ADMIN, 0);
    std::memcpy(user_ptr, &admin, sizeof(UserInfo));
    unsigned char* meta_ptr = omni.data() + layout.meta_offset;
    std::memset(meta_ptr, 0, layout.meta_size);
    MetadataEntry root;
    root.valid_flag = 0;
    root.type_flag = 1;
    root.parent_index = 0;
    root.short_name[0] = '/';
    root.short_name[1] = 0;
    std::memcpy(meta_ptr, &root, sizeof(MetadataEntry));
    unsigned char* bitmap_ptr = omni.data() + layout.free_map_offset;
    std::memset(bitmap_ptr, 0, layout.free_map_size);
    std::ofstream out(omni_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    out.write((const char*)omni.data(), (std::streamsize)omni.size());
    if (!out.good()) return false;
    out.close();
    return true;
}
