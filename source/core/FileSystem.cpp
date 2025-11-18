#include "FileSystem.h"
#include <iostream>
#include <algorithm>
#include <ctime>

// ==========================================================
// Utility: current timestamp
// ==========================================================
uint64_t FileSystem::now_timestamp() {
    return static_cast<uint64_t>(std::time(nullptr));
}

// ==========================================================
// Constructor / destructor
// ==========================================================
FileSystem::FileSystem() {
    is_open = false;
}

FileSystem::~FileSystem() {
    shutdown();
}

// ==========================================================
// Low-level stream opening
// ==========================================================
bool FileSystem::open_stream(bool write) {
    if (is_open) return true;

    std::ios::openmode mode = std::ios::binary;
    if (write) mode |= std::ios::in | std::ios::out;
    else mode |= std::ios::in;

    stream.open(omni_path, mode);
    if (!stream.is_open()) return false;

    is_open = true;
    return true;
}

void FileSystem::close_stream() {
    if (is_open) stream.close();
    is_open = false;
}

// ==========================================================
// COMPUTE LAYOUT (Phase-1 logic)
// ==========================================================
bool FileSystem::compute_layout() {
    layout.header_size = header.header_size;

    // ----- user table -----
    layout.user_table_offset = layout.header_size;
    layout.user_table_size   = header.max_users * sizeof(UserInfo);

    // ----- metadata region (Phase 2) -----
    layout.meta_offset = layout.user_table_offset + layout.user_table_size;
    layout.meta_size   = config.max_files * sizeof(MetadataEntry);

    // ----- free-map + data area -----
    layout.free_map_offset = layout.meta_offset + layout.meta_size;

    uint64_t block_sz = config.block_size;
    if (block_sz == 0) return false;

    // Space left after we reach the start of the bitmap
    uint64_t remaining = config.total_size - layout.free_map_offset;

    // We don't know number of blocks yet because bitmap size depends on it.
    // So: start from the max possible blocks and shrink until it fits:
    uint64_t max_blocks = remaining / block_sz;
    uint64_t blocks = max_blocks;

    while (blocks > 0) {
        uint64_t fm_size   = (blocks + 7) / 8;      // bitmap: 1 bit per block
        uint64_t data_size = blocks * block_sz;
        if (fm_size + data_size <= remaining) {
            break;  // fits!
        }
        --blocks;
    }

    layout.blocks_count  = static_cast<uint32_t>(blocks);
    layout.free_map_size = (layout.blocks_count + 7) / 8;
    layout.data_offset   = layout.free_map_offset + layout.free_map_size;
    layout.data_size     = layout.blocks_count * block_sz;

    return true;
}

// ==========================================================
// LOAD HEADER
// ==========================================================
bool FileSystem::load_header() {
    open_stream(true);

    stream.seekg(0);
    stream.read(reinterpret_cast<char*>(&header), sizeof(OMNIHeader));
    return true;
}

// ==========================================================
// USERS
// ==========================================================
bool FileSystem::load_users_from_disk() {
    users.resize(config.max_users);
    stream.seekg(layout.user_table_offset);
    stream.read(reinterpret_cast<char*>(users.data()), layout.user_table_size);
    return true;
}

bool FileSystem::flush_users_to_disk() {
    stream.seekp(layout.user_table_offset);
    stream.write(reinterpret_cast<const char*>(users.data()), layout.user_table_size);
    stream.flush();
    return true;
}

int FileSystem::find_user_index(const char* username) const {
    for (size_t i = 0; i < users.size(); i++) {
        if (users[i].is_active &&
            std::strcmp(users[i].username, username) == 0)
            return (int)i;
    }
    return -1;
}

bool FileSystem::session_is_admin(void* session) const {
    ActiveSession* s = find_session(session);
    if (!s) return false;
    return s->info.user.role == UserRole::ADMIN;
}

ActiveSession* FileSystem::find_session(void* session) const {
    for (auto* s : sessions)
        if (s == session) return s;
    return nullptr;
}

// ==========================================================
// FORMAT NEW FILESYSTEM
// ==========================================================
bool FileSystem::format_new(const FSConfig& cfg, const char* path) {
    config = cfg;
    omni_path = path;

    // remove old
    std::ofstream del(path, std::ios::binary | std::ios::trunc);
    del.close();

    // open
    if (!open_stream(true))
        return false;

    // build header
    memset(&header, 0, sizeof(header));
    std::memcpy(header.magic, "OMNIFS01", 8);
    header.format_version = 0x00010000;
    header.total_size = config.total_size;
    header.header_size = 512;
    header.block_size  = config.block_size;
    header.max_users   = config.max_users;

    compute_layout();

    // write header
    stream.seekp(0);
    stream.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // init user table
    users.clear();
    users.resize(config.max_users);

    // create admin
    std::string admin_hash = "adminHASH";
    users[0] = UserInfo("admin", admin_hash, UserRole::ADMIN, now_timestamp());
    users[0].is_active = 1;

    flush_users_to_disk();

    // init metadata table (zeroed)
    stream.seekp(layout.meta_offset);
    {
        std::vector<uint8_t> zero(layout.meta_size);
        stream.write((char*)zero.data(), zero.size());
    }
    // ----- CREATE ROOT DIRECTORY (meta index 0) -----
{
    MetadataEntry root{};
    root.valid_flag = 1;
    root.type_flag = 1;             // directory
    root.parent_index = 0;
    strncpy(root.short_name, "/", 10);
    root.short_name[1] = '\0';
    root.created_time = now_timestamp();
    root.modified_time = now_timestamp();

    stream.seekp(layout.meta_offset);
    stream.write((char*)&root, sizeof(MetadataEntry));
}

    // init bitmap
    stream.seekp(layout.free_map_offset);
    {
        std::vector<uint8_t> zero(layout.free_map_size);
        stream.write((char*)zero.data(), zero.size());
    }

    // init data region
    stream.seekp(layout.data_offset);
    {
        std::vector<uint8_t> zero(config.total_size - layout.data_offset);
        stream.write((char*)zero.data(), zero.size());
    }

    stream.flush();
    return true;
}

// ==========================================================
// LOAD EXISTING
// ==========================================================
bool FileSystem::load_existing(const FSConfig& cfg, const char* path) {
    config = cfg;
    omni_path = path;

    if (!open_stream(true)) return false;

    load_header();
    compute_layout();
    load_users_from_disk();

    // init managers
    stream.seekg(0);
    std::vector<uint8_t> file_image(config.total_size);
    stream.read((char*)file_image.data(), file_image.size());

    // metadata manager
    meta.init(file_image.data(), layout.meta_offset, config.max_files);

    // directory tree
    tree.init(&meta);
    tree.rebuild();

    // FreeSpace manager
    fsm.init(file_image.data(), layout.free_map_offset, layout.blocks_count);

    // Block manager
    blockman.init(file_image.data(),
                  layout.data_offset,
                  config.block_size,
                 layout.blocks_count,
                  &fsm);

    return true;
}

// ==========================================================
// SHUTDOWN
// ==========================================================
void FileSystem::shutdown() {
    for (auto* s : sessions) delete s;
    sessions.clear();
    close_stream();
}

// ==========================================================
// USER LOGIN
// ==========================================================
OFSErrorCodes FileSystem::user_login(const char* username,
                                     const char* password,
                                     void** out_session)
{
    int idx = find_user_index(username);
    if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    if (!users[idx].is_active)
        return OFSErrorCodes::ERROR_NOT_FOUND;

    ActiveSession* s = new ActiveSession;
    s->info = SessionInfo("session-ID", users[idx], now_timestamp());
    sessions.push_back(s);

    *out_session = s;
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystem::user_logout(void* session) {
    for (size_t i = 0; i < sessions.size(); i++) {
        if (sessions[i] == session) {
            delete sessions[i];
            sessions.erase(sessions.begin() + i);
            return OFSErrorCodes::SUCCESS;
        }
    }
    return OFSErrorCodes::ERROR_INVALID_SESSION;
}

OFSErrorCodes FileSystem::user_create(void* admin,
                                      const char* username,
                                      const char* password,
                                      UserRole role)
{
    if (!session_is_admin(admin))
        return OFSErrorCodes::ERROR_PERMISSION_DENIED;

    if (find_user_index(username) >= 0)
        return OFSErrorCodes::ERROR_FILE_EXISTS;

    for (auto& u : users) {
        if (!u.is_active) {
            u = UserInfo(username, "HASH", role, now_timestamp());
            u.is_active = 1;
            flush_users_to_disk();
            return OFSErrorCodes::SUCCESS;
        }
    }
    return OFSErrorCodes::ERROR_NO_SPACE;
}

OFSErrorCodes FileSystem::user_delete(void* admin, const char* username) {
    if (!session_is_admin(admin))
        return OFSErrorCodes::ERROR_PERMISSION_DENIED;

    int idx = find_user_index(username);
    if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    users[idx].is_active = 0;
    flush_users_to_disk();
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystem::user_list(void* admin,
                                    UserInfo** out_users,
                                    int* out_count)
{
    if (!session_is_admin(admin))
        return OFSErrorCodes::ERROR_PERMISSION_DENIED;

    *out_users = users.data();
    *out_count = users.size();
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystem::get_session_info(void* session,
                                           SessionInfo* out)
{
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;

    *out = s->info;
    return OFSErrorCodes::SUCCESS;
}

// ==========================================================
// INTERNAL HELPERS
// ==========================================================

int FileSystem::resolve_path(const char* path) {
    std::string p(path);
    return tree.resolve(p);
}

bool FileSystem::is_dir(int idx) {
    MetadataEntry e;
    meta.read_entry(idx, e);
    return (e.valid_flag == 1 && e.type_flag == 1);
}

bool FileSystem::is_file(int idx) {
    MetadataEntry e;
    meta.read_entry(idx, e);
    return (e.valid_flag == 1 && e.type_flag == 0);
}

bool FileSystem::has_permission(const ActiveSession* sess,
                                const MetadataEntry& e,
                                bool write_needed)
{
    // For now simplified: all can read, admin can write
    if (!write_needed) return true;

    if (sess->info.user.role == UserRole::ADMIN)
        return true;

    return false;
}

OFSErrorCodes FileSystem::allocate_file_entry(
    int parent_idx,
    const std::string& name,
    uint32_t& out_idx)
{
    int idx = meta.allocate_entry();
    if (idx < 0) return OFSErrorCodes::ERROR_NO_SPACE;

    MetadataEntry e{};
    e.valid_flag = 1;
    e.type_flag = 0;
    e.parent_index = parent_idx;

    strncpy(e.short_name, name.c_str(), 10);
    e.short_name[10] = '\0';

    e.created_time = now_timestamp();
    e.modified_time = now_timestamp();

    out_idx = idx;
    meta.write_entry(idx, e);

    return OFSErrorCodes::SUCCESS;
}

// ==========================================================
// DIRECTORY CREATE
// ==========================================================
// ==========================================================
// DIRECTORY CREATE
// ==========================================================
OFSErrorCodes FileSystem::dir_create(void* session, const char* path) {
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;

    std::string p(path);
    if (p == "/" || p.empty())
        return OFSErrorCodes::ERROR_INVALID_PATH;

    size_t pos = p.find_last_of('/');
    if (pos == std::string::npos)
        return OFSErrorCodes::ERROR_INVALID_PATH;

    std::string parent = p.substr(0, pos);
    std::string name   = p.substr(pos + 1);

    // ❗ NEW: reject paths where there is no name (like "////", "/docs/")
    if (name.empty())
        return OFSErrorCodes::ERROR_INVALID_PATH;

    if (parent.empty())
        parent = "/";

    int parent_idx = resolve_path(parent.c_str());
    if (parent_idx < 0)
        return OFSErrorCodes::ERROR_NOT_FOUND;

    if (!is_dir(parent_idx))
        return OFSErrorCodes::ERROR_INVALID_OPERATION;

    // duplicate?
    if (meta.find_in_dir(parent_idx, name) >= 0)
        return OFSErrorCodes::ERROR_FILE_EXISTS;

    int idx = meta.allocate_entry();
    if (idx < 0) return OFSErrorCodes::ERROR_NO_SPACE;

    MetadataEntry e{};
    e.valid_flag   = 1;
    e.type_flag    = 1;           // directory
    e.parent_index = parent_idx;

    strncpy(e.short_name, name.c_str(), 10);
    e.short_name[10] = '\0';

    e.created_time  = now_timestamp();
    e.modified_time = now_timestamp();

    meta.write_entry(idx, e);
    tree.rebuild();

    return OFSErrorCodes::SUCCESS;
}

// ==========================================================
// DIRECTORY DELETE
// ==========================================================
OFSErrorCodes FileSystem::dir_delete(void* session, const char* path) {
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;

    int idx = resolve_path(path);
    if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    if (!is_dir(idx))
        return OFSErrorCodes::ERROR_INVALID_OPERATION;

    if (!tree.is_empty_dir(idx))
        return OFSErrorCodes::ERROR_DIRECTORY_NOT_EMPTY;

    meta.free_entry(idx);
    tree.rebuild();
    return OFSErrorCodes::SUCCESS;
}

// ==========================================================
// DIRECTORY LIST
// ==========================================================
OFSErrorCodes FileSystem::dir_list(void* session,
                                   const char* path,
                                   FileEntry** entries,
                                   int* count)
{
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;

    int dir_idx = resolve_path(path);
    if (dir_idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    if (!is_dir(dir_idx))
        return OFSErrorCodes::ERROR_INVALID_OPERATION;

    // gather children
    std::vector<FileEntry> out;
    MetadataEntry e;

    for (uint32_t i = 0; i < meta.capacity(); i++) {
        meta.read_entry(i, e);
        if (!e.valid_flag) continue;
        if (e.parent_index != (uint32_t)dir_idx) continue;

        FileEntry fe;
        strncpy(fe.name, e.short_name, 255);
        fe.name[255] = '\0';

        fe.type = e.type_flag;
        fe.size = e.total_size;
        fe.permissions = e.permissions;
        fe.created_time = e.created_time;
        fe.modified_time = e.modified_time;
        fe.inode = i;

        out.push_back(fe);
    }

    *count = out.size();
    *entries = (FileEntry*)malloc(out.size() * sizeof(FileEntry));
    memcpy(*entries, out.data(), out.size() * sizeof(FileEntry));

    return OFSErrorCodes::SUCCESS;
}

// ==========================================================
// FILE CREATE
// ==========================================================
OFSErrorCodes FileSystem::file_create(void* session,
                                      const char* path,
                                      const char* data,
                                      size_t size)
{
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;

    std::string p(path);
    size_t pos = p.find_last_of('/');
    if (pos == std::string::npos)
        return OFSErrorCodes::ERROR_INVALID_PATH;

    std::string dir = p.substr(0, pos);
    std::string name = p.substr(pos + 1);
    if (dir == "") dir = "/";

    int dir_idx = resolve_path(dir.c_str());
    if (dir_idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    if (!is_dir(dir_idx))
        return OFSErrorCodes::ERROR_INVALID_OPERATION;

    if (meta.find_in_dir(dir_idx, name) >= 0)
        return OFSErrorCodes::ERROR_FILE_EXISTS;

    // allocate metadata
    int idx = meta.allocate_entry();
    if (idx < 0) return OFSErrorCodes::ERROR_NO_SPACE;

    MetadataEntry e{};
    e.valid_flag = 1;
    e.type_flag  = 0;
    e.parent_index = dir_idx;
    strncpy(e.short_name, name.c_str(), 10);
e.short_name[10] = '\0';

    e.created_time = now_timestamp();
    e.modified_time = now_timestamp();

    // allocate first block
    int blk = blockman.allocate_block();
    if (blk < 0) return OFSErrorCodes::ERROR_NO_SPACE;

    e.start_index = blk;
    e.total_size = 0;

    if (size > 0) {
        if (blockman.write_file(blk, 0, (uint8_t*)data, size) < 0) {
            blockman.free_block_chain(blk);
            return OFSErrorCodes::ERROR_IO_ERROR;
        }
        e.total_size = size;
    }

    meta.write_entry(idx, e);
    tree.rebuild();

    return OFSErrorCodes::SUCCESS;
}

// ==========================================================
// FILE READ
// ==========================================================
OFSErrorCodes FileSystem::file_read(
    void* session,
    const char* path,
    char** out_buffer,
    size_t* out_size)
{
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;

    int idx = resolve_path(path);
    if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    if (!is_file(idx))
        return OFSErrorCodes::ERROR_INVALID_OPERATION;

    MetadataEntry e;
    meta.read_entry(idx, e);

    *out_size = e.total_size;
    *out_buffer = (char*)malloc(e.total_size + 1);

    if (blockman.read_file(e.start_index, 0,
                           (uint8_t*)*out_buffer,
                           e.total_size) < 0)
        return OFSErrorCodes::ERROR_IO_ERROR;

    (*out_buffer)[e.total_size] = '\0';
    return OFSErrorCodes::SUCCESS;
}

// ==========================================================
// FILE EDIT (overwrite region)
// ==========================================================
OFSErrorCodes FileSystem::file_edit(
    void* session,
    const char* path,
    const char* data,
    size_t size,
    unsigned int index)
{
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;

    int idx = resolve_path(path);
    if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    MetadataEntry e;
    meta.read_entry(idx, e);

    if (blockman.write_file(e.start_index, index,
                            (uint8_t*)data, size) < 0)
        return OFSErrorCodes::ERROR_IO_ERROR;

    e.total_size = std::max<uint64_t>(e.total_size, index + size);
    e.modified_time = now_timestamp();
    meta.write_entry(idx, e);

    return OFSErrorCodes::SUCCESS;
}

// ==========================================================
// FILE DELETE
// ==========================================================
OFSErrorCodes FileSystem::file_delete(void* session,
                                      const char* path)
{
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;

    int idx = resolve_path(path);
    if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    if (!is_file(idx))
        return OFSErrorCodes::ERROR_INVALID_OPERATION;

    MetadataEntry e;
    meta.read_entry(idx, e);

    // free block chain
    blockman.free_block_chain(e.start_index);

    // free metadata
    meta.free_entry(idx);

    tree.rebuild();
    return OFSErrorCodes::SUCCESS;
}

// ==========================================================
// FILE TRUNCATE
// ==========================================================
OFSErrorCodes FileSystem::file_truncate(void* session,
                                        const char* path)
{
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;

    int idx = resolve_path(path);
    if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    if (!is_file(idx))
        return OFSErrorCodes::ERROR_INVALID_OPERATION;

    MetadataEntry e;
    meta.read_entry(idx, e);

    blockman.free_block_chain(e.start_index);

    // allocate fresh empty block
    int blk = blockman.allocate_block();
    if (blk < 0) return OFSErrorCodes::ERROR_NO_SPACE;

    e.start_index = blk;
    e.total_size = 0;
    e.modified_time = now_timestamp();

    meta.write_entry(idx, e);

    return OFSErrorCodes::SUCCESS;
}

// ==========================================================
// METADATA
// ==========================================================
OFSErrorCodes FileSystem::get_metadata(void* session,
                                       const char* path,
                                       FileMetadata* meta_out)
{
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;

    int idx = resolve_path(path);
    if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    MetadataEntry e;
    meta.read_entry(idx, e);

    FileEntry fe;
    strncpy(fe.name, e.short_name, 255);
    fe.type = e.type_flag;
    fe.size = e.total_size;
    fe.permissions = e.permissions;
    fe.created_time = e.created_time;
    fe.modified_time = e.modified_time;
    fe.inode = idx;

    FileMetadata out(path, fe);
    out.actual_size = e.total_size;

    *meta_out = out;
    return OFSErrorCodes::SUCCESS;
}

// ==========================================================
// SET PERMISSIONS
// ==========================================================
OFSErrorCodes FileSystem::set_permissions(
    void* session,
    const char* path,
    uint32_t perms)
{
    if (!session_is_admin(session))
        return OFSErrorCodes::ERROR_PERMISSION_DENIED;

    int idx = resolve_path(path);
    if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;

    MetadataEntry e;
    meta.read_entry(idx, e);

    e.permissions = perms;
    meta.write_entry(idx, e);
    return OFSErrorCodes::SUCCESS;
}

// ==========================================================
// FILESYSTEM STATS
// ==========================================================
OFSErrorCodes FileSystem::get_stats(void* session,
                                    FSStats* st)
{
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;

    FSStats stats(header.total_size,
                  header.total_size - layout.data_size,
                  layout.data_size);

    stats.total_users = header.max_users;
    stats.total_files = 0;
    stats.total_directories = 0;

    MetadataEntry e;
    for (uint32_t i = 0; i < meta.capacity(); i++) {
        meta.read_entry(i, e);
        if (!e.valid_flag) continue;
        if (e.type_flag == 0) stats.total_files++;
        else stats.total_directories++;
    }

    *st = stats;
    return OFSErrorCodes::SUCCESS;
}
