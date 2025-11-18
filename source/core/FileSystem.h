#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cstring>

#include "../include/odf_types.hpp"    // OMNIHeader, UserInfo, SessionInfo, FSStats, FileEntry...
#include "config_parser.cpp"             // FSConfig + parse_uconf
#include "MetadataManager.cpp"
#include "directory_tree.cpp"
#include "FreeSpaceManager.cpp"
#include "BlockManager.cpp"

// ===============================
// On-disk layout information
// ===============================
struct FSLayout {
    uint64_t header_size;

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

// ===============================
// In-memory active session
// ===============================
struct ActiveSession {
    SessionInfo info;   // includes user info + timestamps + ops count
};

// ===============================
// FileSystem CLASS
// ===============================
class FileSystem {
public:
    FileSystem();
    ~FileSystem();

    // ===============================
    // FORMAT + LOAD
    // ===============================
    bool format_new(const FSConfig& cfg, const char* omni_path);
    bool load_existing(const FSConfig& cfg, const char* omni_path);
    void shutdown();

    // ===============================
    // USER + SESSION MANAGEMENT
    // ===============================
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

    // ===============================
    // DIRECTORY OPERATIONS (Phase 2)
    // ===============================
    OFSErrorCodes dir_create(void* session, const char* path);
    OFSErrorCodes dir_delete(void* session, const char* path);
    OFSErrorCodes dir_list(void* session, const char* path, FileEntry** entries, int* count);

    // ===============================
    // FILE OPERATIONS (Phase 2)
    // ===============================
    OFSErrorCodes file_create(void* session,
                              const char* path,
                              const char* data,
                              size_t size);

    OFSErrorCodes file_read(void* session,
                            const char* path,
                            char** out_buffer,
                            size_t* out_size);

    OFSErrorCodes file_edit(void* session,
                            const char* path,
                            const char* data,
                            size_t size,
                            unsigned int index);

    OFSErrorCodes file_delete(void* session,
                              const char* path);

    OFSErrorCodes file_truncate(void* session,
                                const char* path);

    // ===============================
    // METADATA + PERMISSIONS
    // ===============================
    OFSErrorCodes get_metadata(void* session,
                               const char* path,
                               FileMetadata* meta);

    OFSErrorCodes set_permissions(void* session,
                                  const char* path,
                                  uint32_t permissions);

    // ===============================
    // FILESYSTEM STATS
    // ===============================
    OFSErrorCodes get_stats(void* session,
                            FSStats* stats);

    // ===============================
    // Accessors
    // ===============================
    const FSConfig&   get_config() const { return config; }
    const OMNIHeader& get_header() const { return header; }
    const FSLayout&   get_layout() const { return layout; }

private:
    // ===============================
    // STORED STATE
    // ===============================
    FSConfig  config;
    OMNIHeader header;
    FSLayout   layout;

    std::fstream stream;
    bool is_open;
    std::string omni_path;

    std::vector<UserInfo> users;
    std::vector<ActiveSession*> sessions;

    // ===============================
    // MANAGERS (Phase 2)
    // ===============================
    MetadataManager meta;
    DirectoryTree   tree;
    FreeSpaceManager fsm;
    BlockManager     blockman;

    // ===============================
    // INTERNAL HELPERS
    // ===============================
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

    // ------- INTERNAL FS HELPERS -------
    int resolve_path(const char* path);
    bool is_dir(int meta_idx);
    bool is_file(int meta_idx);
    bool has_permission(const ActiveSession* sess, const MetadataEntry& e, bool write_needed);
    OFSErrorCodes allocate_file_entry(int parent_idx,
                                      const std::string& name,
                                      uint32_t& out_meta_idx);
};

#endif // FILE_SYSTEM_H
