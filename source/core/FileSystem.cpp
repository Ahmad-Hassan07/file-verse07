#include "FileSystem.h"

#include <chrono>
#include <iostream>
#include <algorithm>


// --------- small internal helpers ----------

static void fs_zero_bytes(std::vector<unsigned char>& buf) {
    if (!buf.empty()) {
        std::memset(buf.data(), 0, buf.size());
    }
}

static void set_omni_magic(OMNIHeader& h) {
    const char magic_str[8] = {'O','M','N','I','F','S','0','1'};
    std::memcpy(h.magic, magic_str, sizeof(h.magic));
}

// =============================================
// FileSystem ctor/dtor
// =============================================
FileSystem::FileSystem() : is_open(false) {
    std::memset(&header, 0, sizeof(header));
    std::memset(&layout, 0, sizeof(layout));
    std::memset(&config, 0, sizeof(config));
}

FileSystem::~FileSystem() {
    shutdown();
}

// =============================================
// Layout computation
// =============================================

bool FileSystem::compute_layout() {
    if (config.total_size == 0 ||
        config.block_size == 0 ||
        config.max_users == 0) {
        return false;
    }

    layout.header_size = config.header_size;  // bytes reserved at start
    if (layout.header_size == 0) {
        // If not specified, default to sizeof(OMNIHeader) rounded up to 512.
        layout.header_size = 512;
    }

    // --- user table area ---
    layout.user_table_offset = layout.header_size;
    layout.user_table_size   = static_cast<uint64_t>(config.max_users) * sizeof(UserInfo);

    // For now, we leave meta area as 0 (Phase 2 / future).
    layout.meta_offset = 0;
    layout.meta_size   = 0;

    // Remaining bytes after header + user table
    if (config.total_size <= layout.header_size + layout.user_table_size) {
        return false;
    }
    uint64_t remaining = config.total_size - layout.header_size - layout.user_table_size;

    // We need to find a self-consistent number of blocks and bitmap size.
    uint64_t block_size = config.block_size;
    if (block_size == 0) return false;

    // Initial naive guess: all remaining is data -> blocks_guess
    uint64_t blocks = remaining / block_size;
    if (blocks == 0) return false;

    while (true) {
        uint64_t bitmap_bytes = (blocks + 7) / 8; // 1 bit per block
        if (remaining <= bitmap_bytes) {
            return false;
        }
        uint64_t data_bytes = remaining - bitmap_bytes;
        uint64_t new_blocks = data_bytes / block_size;
        if (new_blocks == 0) return false;
        if (new_blocks == blocks) {
            // converged
            layout.free_map_size = bitmap_bytes;
            layout.data_size     = data_bytes;
            break;
        }
        blocks = new_blocks;
    }

    layout.blocks_count    = static_cast<uint32_t>(blocks);
    layout.free_map_offset = layout.user_table_offset + layout.user_table_size;
    layout.data_offset     = layout.free_map_offset + layout.free_map_size;

    // Final sanity
    uint64_t end_pos = layout.data_offset + layout.data_size;
    if (end_pos != config.total_size) {
        // We can allow a little slack if rounding reduced a few bytes,
        // but for now we require exact match.
        if (end_pos > config.total_size) {
            return false;
        }
    }

    return true;
}

// =============================================
// Stream handling
// =============================================
bool FileSystem::open_stream(bool write) {
    // If stream not open → open it correctly
    if (!is_open) {
        std::ios::openmode mode = std::ios::binary;
        if (write) {
            mode |= (std::ios::in | std::ios::out);
        } else {
            mode |= std::ios::in;
        }
        stream.open(omni_path.c_str(), mode);
        if (!stream.is_open()) return false;
        is_open = true;
        return true;
    }

    // Stream is already open, but if we now need write → reopen
    if (write) {
        stream.close();
        is_open = false;
        std::ios::openmode mode = std::ios::binary | std::ios::in | std::ios::out;
        stream.open(omni_path.c_str(), mode);
        if (!stream.is_open()) return false;
        is_open = true;
    }

    return true;
}


void FileSystem::close_stream() {
    if (is_open) {
        stream.close();
        is_open = false;
    }
}

// =============================================
// Formatting a new .omni file
// =============================================
bool FileSystem::format_new(const FSConfig& cfg, const char* path) {
    if (!path) return false;

    config = cfg;
    omni_path = path;

    if (!compute_layout()) {
        std::cerr << "compute_layout() failed during format_new\n";
        return false;
    }

    // Prepare a full buffer for the new file
    std::vector<unsigned char> buffer(config.total_size);
    fs_zero_bytes(buffer);

    // --- Header ---
    OMNIHeader h;
    std::memset(&h, 0, sizeof(h));
    set_omni_magic(h);
    h.format_version = 0x00010000; // v1.0 (example)
    h.total_size     = config.total_size;
    h.header_size    = layout.header_size;
    h.block_size     = config.block_size;

    // user-related header fields
    h.user_table_offset = static_cast<uint32_t>(layout.user_table_offset);
    h.max_users         = static_cast<uint32_t>(config.max_users);

    // Phase 2 offsets: 0 for now
    h.file_state_storage_offset = 0;
    h.change_log_offset         = 0;

    // copy header into buffer at start
    std::memcpy(buffer.data(), &h, sizeof(h));
    // remaining header region (layout.header_size - sizeof(h)) is already zero.

    // --- User table area ---
    unsigned char* user_ptr = buffer.data() + layout.user_table_offset;
    std::memset(user_ptr, 0, layout.user_table_size);

    // Add default admin user at index 0
    if (config.admin_username[0] == '\0' || config.admin_password[0] == '\0') {
        std::cerr << "Admin credentials not set in config\n";
        return false;
    }

    UserInfo admin;
    std::memset(&admin, 0, sizeof(admin));
    std::strncpy(admin.username, config.admin_username, sizeof(admin.username) - 1);
    std::strncpy(admin.password_hash, config.admin_password, sizeof(admin.password_hash) - 1);
    admin.role         = UserRole::ADMIN;
    admin.created_time = now_timestamp();
    admin.last_login   = 0;
    admin.is_active    = 1;
    std::memset(admin.reserved, 0, sizeof(admin.reserved));

    std::memcpy(user_ptr, &admin, sizeof(UserInfo));

    // --- Free-space bitmap ---
    unsigned char* bitmap_ptr = buffer.data() + layout.free_map_offset;
    std::memset(bitmap_ptr, 0, layout.free_map_size);  // 0 => free

    // Data region is already zeroed

    // --- Write buffer to disk ---
    std::ofstream out(omni_path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "Failed to open omni file for writing: " << omni_path << "\n";
        return false;
    }
    out.write(reinterpret_cast<const char*>(buffer.data()),
              static_cast<std::streamsize>(buffer.size()));
    if (!out.good()) {
        std::cerr << "Error writing to omni file: " << omni_path << "\n";
        out.close();
        return false;
    }
    out.close();

    // Update in-memory state
    header = h;
    users.clear();
    users.push_back(admin);
    sessions.clear();

    return true;
}

// =============================================
// Loading an existing .omni file
// =============================================
bool FileSystem::load_header() {
    if (!open_stream(false)) return false;

    stream.seekg(0, std::ios::beg);
    OMNIHeader h;
    std::memset(&h, 0, sizeof(h));
    stream.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (!stream.good()) {
        return false;
    }

    // basic sanity checks
    if (std::memcmp(h.magic, "OMNIFS01", 8) != 0) {
        std::cerr << "Invalid magic in OMNI header\n";
        return false;
    }
    if (h.total_size != config.total_size) {
        std::cerr << "Header total_size mismatch\n";
        return false;
    }
    if (h.block_size != config.block_size) {
        std::cerr << "Header block_size mismatch\n";
        return false;
    }

    header = h;
    return true;
}

bool FileSystem::load_users_from_disk() {
    if (!open_stream(false)) return false;

    users.clear();
    users.reserve(config.max_users);

    stream.seekg(static_cast<std::streamoff>(layout.user_table_offset), std::ios::beg);
    for (uint32_t i = 0; i < config.max_users; ++i) {
        UserInfo u;
        std::memset(&u, 0, sizeof(u));
        stream.read(reinterpret_cast<char*>(&u), sizeof(u));
        if (!stream.good()) {
            return false;
        }
        if (u.is_active) {
            users.push_back(u);
        }
    }
    return true;
}

bool FileSystem::flush_users_to_disk() {
    if (!open_stream(true)) return false;

    // We store users in a fixed-size table of config.max_users slots.
    std::vector<UserInfo> table(config.max_users);
    std::memset(table.data(), 0, table.size() * sizeof(UserInfo));

    // Fill sequentially with active users
    size_t count = std::min<size_t>(users.size(), config.max_users);
    for (size_t i = 0; i < count; ++i) {
        table[i] = users[i];
    }

    stream.seekp(static_cast<std::streamoff>(layout.user_table_offset), std::ios::beg);
    stream.write(reinterpret_cast<const char*>(table.data()),
                 static_cast<std::streamsize>(table.size() * sizeof(UserInfo)));
    if (!stream.good()) {
        return false;
    }
    stream.flush();
    return true;
}

bool FileSystem::load_existing(const FSConfig& cfg, const char* path) {
    if (!path) return false;

    config   = cfg;
    omni_path = path;
    sessions.clear();

    // Layout must match the one used in format_new
    if (!compute_layout()) {
        std::cerr << "compute_layout() failed in load_existing\n";
        return false;
    }

    if (!load_header()) {
        std::cerr << "load_header() failed\n";
        return false;
    }
    if (!load_users_from_disk()) {
        std::cerr << "load_users_from_disk() failed\n";
        return false;
    }

    return true;
}

// =============================================
// Shutdown
// =============================================
void FileSystem::shutdown() {
    // Free sessions
    for (ActiveSession* s : sessions) {
        delete s;
    }
    sessions.clear();

    close_stream();
}

// =============================================
// User / Session helpers
// =============================================

int FileSystem::find_user_index(const char* username) const {
    if (!username) return -1;
    for (size_t i = 0; i < users.size(); ++i) {
        if (std::strncmp(users[i].username, username, sizeof(users[i].username)) == 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

ActiveSession* FileSystem::find_session(void* session) const {
    ActiveSession* ptr = reinterpret_cast<ActiveSession*>(session);
    for (ActiveSession* s : sessions) {
        if (s == ptr) return s;
    }
    return nullptr;
}

bool FileSystem::session_is_admin(void* session) const {
    ActiveSession* s = find_session(session);
    if (!s) return false;
    return s->info.user.role == UserRole::ADMIN;
}

uint64_t FileSystem::now_timestamp() {
    using namespace std::chrono;
    return duration_cast<seconds>(
        system_clock::now().time_since_epoch()
    ).count();
}

// =============================================
// User / Session API
// =============================================

OFSErrorCodes FileSystem::user_login(const char* username,
                                     const char* password,
                                     void** out_session) {
    if (!out_session) return OFSErrorCodes::ERROR_INVALID_SESSION;
    *out_session = nullptr;

    if (!config.require_auth) {
        // Auth disabled: create a synthetic session
        ActiveSession* sess = new ActiveSession();
        std::memset(&sess->info, 0, sizeof(sess->info));
        std::strncpy(sess->info.session_id, "noauth_session", sizeof(sess->info.session_id) - 1);

        // If username exists, use it; else create a dummy user struct
        int idx = find_user_index(username ? username : "");
        if (idx >= 0) {
            sess->info.user = users[static_cast<size_t>(idx)];
        } else {
            std::memset(&sess->info.user, 0, sizeof(sess->info.user));
            if (username) {
                std::strncpy(sess->info.user.username, username,
                             sizeof(sess->info.user.username) - 1);
            }
            sess->info.user.role      = UserRole::NORMAL;
            sess->info.user.is_active = 1;
        }

        uint64_t now = now_timestamp();
        sess->info.login_time     = now;
        sess->info.last_activity  = now;
        sess->info.operations_count = 0;
        std::memset(sess->info.reserved, 0, sizeof(sess->info.reserved));

        sessions.push_back(sess);
        *out_session = sess;
        return OFSErrorCodes::SUCCESS;
    }

    // Auth is required: validate username+password
    int idx = find_user_index(username);
    if (idx < 0) {
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    UserInfo& u = users[static_cast<size_t>(idx)];
    if (!u.is_active) {
        return OFSErrorCodes::ERROR_PERMISSION_DENIED;
    }

    if (!password || std::strncmp(u.password_hash, password,
                                  sizeof(u.password_hash)) != 0) {
        return OFSErrorCodes::ERROR_PERMISSION_DENIED;
    }

    // Create session
    ActiveSession* sess = new ActiveSession();
    std::memset(&sess->info, 0, sizeof(sess->info));

    // session_id = "sess_N"
    char sid[64];
    std::snprintf(sid, sizeof(sid), "sess_%zu", sessions.size() + 1);
    std::strncpy(sess->info.session_id, sid, sizeof(sess->info.session_id) - 1);

    sess->info.user = u;
    uint64_t now = now_timestamp();
    sess->info.login_time    = now;
    sess->info.last_activity = now;
    sess->info.operations_count = 0;

    sessions.push_back(sess);
    *out_session = sess;

    // update last_login
    u.last_login = now;
    flush_users_to_disk();

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystem::user_logout(void* session) {
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;

    auto it = std::find(sessions.begin(), sessions.end(), s);
    if (it != sessions.end()) {
        sessions.erase(it);
    }
    delete s;
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystem::user_create(void* admin_session,
                                      const char* username,
                                      const char* password,
                                      UserRole role) {
    if (!session_is_admin(admin_session)) {
        return OFSErrorCodes::ERROR_PERMISSION_DENIED;
    }
    if (!username || !password) {
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    if (find_user_index(username) >= 0) {
        return OFSErrorCodes::ERROR_FILE_EXISTS; // "user exists"
    }

    if (users.size() >= config.max_users) {
        return OFSErrorCodes::ERROR_NO_SPACE;
    }

    UserInfo u;
    std::memset(&u, 0, sizeof(u));
    std::strncpy(u.username, username, sizeof(u.username) - 1);
    std::strncpy(u.password_hash, password, sizeof(u.password_hash) - 1);
    u.role         = role;
    u.created_time = now_timestamp();
    u.last_login   = 0;
    u.is_active    = 1;
    std::memset(u.reserved, 0, sizeof(u.reserved));

    users.push_back(u);

    if (!flush_users_to_disk()) {
        return OFSErrorCodes::ERROR_IO_ERROR;
    }

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystem::user_delete(void* admin_session,
                                      const char* username) {
    if (!session_is_admin(admin_session)) {
        return OFSErrorCodes::ERROR_PERMISSION_DENIED;
    }
    if (!username) return OFSErrorCodes::ERROR_INVALID_OPERATION;

    // Never delete built-in admin
    if (std::strcmp(username, config.admin_username) == 0) {
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    int idx = find_user_index(username);
    if (idx < 0) {
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    users.erase(users.begin() + idx);

    if (!flush_users_to_disk()) {
        return OFSErrorCodes::ERROR_IO_ERROR;
    }

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystem::user_list(void* admin_session,
                                    UserInfo** out_users,
                                    int* out_count) {
    if (!out_users || !out_count) {
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }
    *out_users = nullptr;
    *out_count = 0;

    if (!session_is_admin(admin_session)) {
        return OFSErrorCodes::ERROR_PERMISSION_DENIED;
    }

    if (users.empty()) {
        return OFSErrorCodes::SUCCESS;
    }

    UserInfo* arr = new UserInfo[users.size()];
    for (size_t i = 0; i < users.size(); ++i) {
        arr[i] = users[i];
    }

    *out_users = arr;
    *out_count = static_cast<int>(users.size());
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystem::get_session_info(void* session,
                                           SessionInfo* out_info) {
    if (!out_info) return OFSErrorCodes::ERROR_INVALID_OPERATION;
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;
    *out_info = s->info;
    return OFSErrorCodes::SUCCESS;
}

// =============================================
// Accessors
// =============================================
const FSConfig& FileSystem::get_config() const {
    return config;
}

const OMNIHeader& FileSystem::get_header() const {
    return header;
}

const FSLayout& FileSystem::get_layout() const {
    return layout;
}
