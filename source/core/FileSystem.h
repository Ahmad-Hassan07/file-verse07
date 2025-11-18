#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cstring>

#include "../include/odf_types.hpp"   // OMNIHeader, UserInfo, SessionInfo, FSStats, etc.
#include "config_parser.cpp"           // FSConfig + parse_uconf

// Layout of the on-disk .omni file (Phase 1)
struct FSLayout {
    uint64_t header_size;        // bytes reserved for header region
    uint64_t user_table_offset;  // where UserInfo table starts
    uint64_t user_table_size;    // total size of user table in bytes

    uint64_t meta_offset;        // reserved (Phase 2 / metadata area)
    uint64_t meta_size;          // reserved

    uint64_t free_map_offset;    // bitmap of data blocks
    uint64_t free_map_size;      // bytes in bitmap

    uint64_t data_offset;        // where data blocks start
    uint64_t data_size;          // total bytes available for blocks

    uint32_t blocks_count;       // how many blocks in data area
};

// In-memory representation of an active session
struct ActiveSession {
    SessionInfo info;            // contains user + session_id + times + op count
};

// Core filesystem object (no sockets/UI here)
class FileSystem {
public:
    FileSystem();
    ~FileSystem();

    // Create a brand new .omni file according to cfg and omni_path.
    // Overwrites any existing file at omni_path.
    bool format_new(const FSConfig& cfg, const char* omni_path);

    // Open an existing .omni file and load basic state (header, layout, users, bitmap).
    bool load_existing(const FSConfig& cfg, const char* omni_path);

    // Close stream, free sessions, clear state.
    void shutdown();

    // ======================
    // USER / SESSION METHODS
    // ======================
    OFSErrorCodes user_login(const char* username,
                             const char* password,
                             void** out_session);

    OFSErrorCodes user_logout(void* session);

    OFSErrorCodes user_create(void* admin_session,
                              const char* username,
                              const char* password,
                              UserRole role);

    OFSErrorCodes user_delete(void* admin_session,
                              const char* username);

    OFSErrorCodes user_list(void* admin_session,
                            UserInfo** out_users,
                            int* out_count);

    OFSErrorCodes get_session_info(void* session,
                                   SessionInfo* out_info);

    // Read-only accessors for tests / diagnostics
    const FSConfig&   get_config() const;
    const OMNIHeader& get_header() const;
    const FSLayout&   get_layout() const;

private:
    FSConfig  config;
    OMNIHeader header;
    FSLayout   layout;

    std::fstream stream;
    bool is_open;
    std::string omni_path;

    // On-disk user table loaded into memory
    std::vector<UserInfo> users;

    // Active sessions
    std::vector<ActiveSession*> sessions;

    // ======== Helpers ========
    bool compute_layout();
    bool open_stream(bool write);
    void close_stream();

    bool load_header();
    bool load_users_from_disk();
    bool flush_users_to_disk();

    int  find_user_index(const char* username) const;
    bool session_is_admin(void* session) const;
    ActiveSession* find_session(void* session) const;

    static uint64_t now_timestamp();
};

#endif // FILE_SYSTEM_H
