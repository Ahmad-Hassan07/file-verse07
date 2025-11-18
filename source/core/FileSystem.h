#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <string>
#include <vector>
#include <fstream>
#include "../include/odf_types.hpp"
#include "config_parser.h"

struct FSLayout {
    uint64_t user_table_offset;
    uint64_t user_table_size;
    uint64_t meta_offset;
    uint64_t meta_size;
    uint64_t free_map_offset;
    uint64_t free_map_size;
    uint64_t data_offset;
    uint64_t data_size;
    uint32_t blocks_count;
};

struct ActiveSession {
    SessionInfo info;
};

class FileSystem {
public:
    FileSystem();
    ~FileSystem();

    bool format_new(const FSConfig& cfg, const char* omni_path);
    bool load_existing(const FSConfig& cfg, const char* omni_path);
    void shutdown();

    OFSErrorCodes user_login(const char* username, const char* password, void** session);
    OFSErrorCodes user_logout(void* session);
    OFSErrorCodes user_create(void* admin_session, const char* username, const char* password, UserRole role);
    OFSErrorCodes user_delete(void* admin_session, const char* username);
    OFSErrorCodes user_list(void* admin_session, UserInfo** users, int* count);
    OFSErrorCodes get_session_info(void* session, SessionInfo* info);

    const FSConfig& get_config() const;
    const OMNIHeader& get_header() const;
    const FSLayout& get_layout() const;

private:
    FSConfig config;
    OMNIHeader header;
    FSLayout layout;
    std::string omni_path;
    std::fstream omni;
    bool is_open;

    std::vector<UserInfo> users;
    std::vector<ActiveSession*> sessions;

    bool compute_layout();
    bool open_stream(bool write);
    void close_stream();
    bool load_header();
    bool load_users_from_disk();
    bool flush_users_to_disk();

    int find_user_index(const char* username) const;
    bool session_is_admin(void* session) const;
    ActiveSession* find_session(void* session) const;
};

#endif
