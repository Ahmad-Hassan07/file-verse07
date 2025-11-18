#include "FileSystem.h"
#include <cstring>

FileSystem::FileSystem() : is_open(false) {
    std::memset(&header, 0, sizeof(header));
    std::memset(&layout, 0, sizeof(layout));
    std::memset(&config, 0, sizeof(config));
}

FileSystem::~FileSystem() {
    shutdown();
}

bool FileSystem::compute_layout() {
    if (config.total_size == 0 || config.block_size == 0 || config.max_users == 0 || config.max_inodes == 0) {
        return false;
    }
    uint64_t total = config.total_size;
    layout.user_table_offset = config.header_size;
    layout.user_table_size = (uint64_t)config.max_users * (uint64_t)sizeof(UserInfo);
    layout.meta_offset = layout.user_table_offset + layout.user_table_size;
    layout.meta_size = (uint64_t)config.max_inodes * (uint64_t)sizeof(MetadataEntry);
    layout.free_map_offset = layout.meta_offset + layout.meta_size;
    if (layout.free_map_offset >= total) return false;
    uint64_t remaining = total - layout.free_map_offset;
    uint64_t blocks = remaining / config.block_size;
    if (blocks == 0) return false;
    layout.blocks_count = (uint32_t)blocks;
    uint64_t bitmap_bytes = (blocks + 7) / 8;
    layout.free_map_size = bitmap_bytes;
    layout.data_offset = layout.free_map_offset + layout.free_map_size;
    if (layout.data_offset > total) return false;
    layout.data_size = total - layout.data_offset;
    return true;
}

bool FileSystem::open_stream(bool write) {
    if (is_open) return true;
    std::ios::openmode mode = std::ios::binary;
    if (write) mode |= std::ios::in | std::ios::out;
    else mode |= std::ios::in | std::ios::out;
    omni.open(omni_path.c_str(), mode);
    if (!omni.is_open()) return false;
    is_open = true;
    return true;
}

void FileSystem::close_stream() {
    if (is_open) {
        omni.close();
        is_open = false;
    }
}

bool FileSystem::format_new(const FSConfig& cfg, const char* path) {
    if (!path) return false;
    config = cfg;
    omni_path = std::string(path);
    if (!compute_layout()) return false;
    std::vector<unsigned char> buffer;
    buffer.assign(config.total_size, 0);

    OMNIHeader h(0x00010000u, config.total_size, config.header_size, config.block_size);
    std::memcpy(h.magic, "OMNIFS01", 8);
    std::memcpy(h.student_id, config.student_id, 32);
    std::memcpy(h.submission_date, config.submission_date, 16);
    h.user_table_offset = (uint32_t)layout.user_table_offset;
    h.max_users = config.max_users;
    h.file_state_storage_offset = 0;
    h.change_log_offset = 0;

    std::memcpy(buffer.data(), &h, sizeof(OMNIHeader));

    unsigned char* user_ptr = buffer.data() + layout.user_table_offset;
    std::memset(user_ptr, 0, layout.user_table_size);

    std::string admin_user;
    for (int i = 0; i < 32; i++) {
        char c = cfg.admin_username[i];
        if (c == 0) break;
        admin_user.push_back(c);
    }
    std::string admin_pass;
    for (int i = 0; i < 32; i++) {
        char c = cfg.admin_password[i];
        if (c == 0) break;
        admin_pass.push_back(c);
    }
    UserInfo admin(admin_user, admin_pass, UserRole::ADMIN, 0);
    std::memcpy(user_ptr, &admin, sizeof(UserInfo));

    unsigned char* meta_ptr = buffer.data() + layout.meta_offset;
    std::memset(meta_ptr, 0, layout.meta_size);
    MetadataEntry root;
    std::memset(&root, 0, sizeof(root));
    root.valid_flag = 0;
    root.type_flag = 1;
    root.parent_index = 0;
    root.short_name[0] = '/';
    root.short_name[1] = 0;
    std::memcpy(meta_ptr, &root, sizeof(MetadataEntry));

    unsigned char* bitmap_ptr = buffer.data() + layout.free_map_offset;
    std::memset(bitmap_ptr, 0, layout.free_map_size);

    std::ofstream out(omni_path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    out.write((const char*)buffer.data(), (std::streamsize)buffer.size());
    if (!out.good()) {
        out.close();
        return false;
    }
    out.close();
    header = h;
    users.clear();
    users.push_back(admin);
    sessions.clear();
    return true;
}

bool FileSystem::load_header() {
    if (!open_stream(false)) return false;
    omni.seekg(0, std::ios::beg);
    omni.read((char*)&header, sizeof(OMNIHeader));
    if (!omni.good()) return false;
    return true;
}

bool FileSystem::load_users_from_disk() {
    if (!compute_layout()) return false;
    if (!open_stream(false)) return false;
    users.clear();
    users.reserve(config.max_users);
    omni.seekg((std::streamoff)layout.user_table_offset, std::ios::beg);
    for (uint32_t i = 0; i < config.max_users; i++) {
        UserInfo u;
        omni.read((char*)&u, sizeof(UserInfo));
        if (!omni.good()) return false;
        users.push_back(u);
    }
    return true;
}

bool FileSystem::flush_users_to_disk() {
    if (!compute_layout()) return false;
    if (!open_stream(true)) return false;
    omni.seekp((std::streamoff)layout.user_table_offset, std::ios::beg);
    for (uint32_t i = 0; i < config.max_users && i < users.size(); i++) {
        omni.write((const char*)&users[i], sizeof(UserInfo));
        if (!omni.good()) return false;
    }
    for (uint32_t i = users.size(); i < config.max_users; i++) {
        UserInfo empty;
        std::memset(&empty, 0, sizeof(empty));
        omni.write((const char*)&empty, sizeof(UserInfo));
        if (!omni.good()) return false;
    }
    omni.flush();
    if (!omni.good()) return false;
    return true;
}

bool FileSystem::load_existing(const FSConfig& cfg, const char* path) {
    if (!path) return false;
    config = cfg;
    omni_path = std::string(path);
    if (!load_header()) return false;
    if (header.total_size != config.total_size) return false;
    if (header.block_size != config.block_size) return false;
    if (!compute_layout()) return false;
    if (!load_users_from_disk()) return false;
    sessions.clear();
    return true;
}

void FileSystem::shutdown() {
    for (size_t i = 0; i < sessions.size(); i++) {
        delete sessions[i];
    }
    sessions.clear();
    close_stream();
}

int FileSystem::find_user_index(const char* username) const {
    if (!username) return -1;
    for (size_t i = 0; i < users.size(); i++) {
        if (users[i].is_active != 0) {
            if (std::strncmp(users[i].username, username, sizeof(users[i].username)) == 0) {
                return (int)i;
            }
        }
    }
    return -1;
}

ActiveSession* FileSystem::find_session(void* session) const {
    ActiveSession* s = (ActiveSession*)session;
    for (size_t i = 0; i < sessions.size(); i++) {
        if (sessions[i] == s) return s;
    }
    return 0;
}

bool FileSystem::session_is_admin(void* session) const {
    ActiveSession* s = find_session(session);
    if (!s) return false;
    if (s->info.user.role == UserRole::ADMIN) return true;
    return false;
}

OFSErrorCodes FileSystem::user_login(const char* username, const char* password, void** session) {
    if (!session || !username || !password) return OFSErrorCodes::ERROR_INVALID_OPERATION;

    if (config.require_auth != 0) {
        int idx = find_user_index(username);
        if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;
        UserInfo u = users[(size_t)idx];
        if (u.is_active == 0) return OFSErrorCodes::ERROR_NOT_FOUND;
        if (std::strncmp(u.password_hash, password, sizeof(u.password_hash)) != 0) {
            return OFSErrorCodes::ERROR_PERMISSION_DENIED;
        }
        ActiveSession* s = new ActiveSession();
        std::memset(s, 0, sizeof(ActiveSession));
        s->info.user = u;
        std::snprintf(s->info.session_id, sizeof(s->info.session_id), "sess_%zu", sessions.size() + 1);
        s->info.login_time = 0;
        s->info.last_activity = 0;
        s->info.operations_count = 0;
        sessions.push_back(s);
        *session = s;
        return OFSErrorCodes::SUCCESS;
    } else {
        ActiveSession* s = new ActiveSession();
        std::memset(s, 0, sizeof(ActiveSession));
        int idx = find_user_index(username);
        if (idx >= 0) {
            s->info.user = users[(size_t)idx];
        } else {
            UserInfo temp;
            std::memset(&temp, 0, sizeof(temp));
            std::strncpy(temp.username, username, sizeof(temp.username) - 1);
            temp.username[sizeof(temp.username) - 1] = 0;
            temp.role = UserRole::NORMAL;
            temp.is_active = 1;
            s->info.user = temp;
        }
        std::snprintf(s->info.session_id, sizeof(s->info.session_id), "sess_%zu", sessions.size() + 1);
        s->info.login_time = 0;
        s->info.last_activity = 0;
        s->info.operations_count = 0;
        sessions.push_back(s);
        *session = s;
        return OFSErrorCodes::SUCCESS;
    }
}


OFSErrorCodes FileSystem::user_logout(void* session) {
    ActiveSession* s = (ActiveSession*)session;
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;
    bool found = false;
    for (size_t i = 0; i < sessions.size(); i++) {
        if (sessions[i] == s) {
            delete sessions[i];
            sessions[i] = sessions.back();
            sessions.pop_back();
            found = true;
            break;
        }
    }
    if (!found) return OFSErrorCodes::ERROR_INVALID_SESSION;
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystem::user_create(void* admin_session, const char* username, const char* password, UserRole role) {
    if (!admin_session || !username || !password) return OFSErrorCodes::ERROR_INVALID_OPERATION;
    if (!session_is_admin(admin_session)) return OFSErrorCodes::ERROR_PERMISSION_DENIED;
    if (find_user_index(username) >= 0) return OFSErrorCodes::ERROR_FILE_EXISTS;
    if (users.size() >= config.max_users) return OFSErrorCodes::ERROR_NO_SPACE;
    UserInfo u;
    std::memset(&u, 0, sizeof(u));
    std::strncpy(u.username, username, sizeof(u.username) - 1);
    u.username[sizeof(u.username) - 1] = 0;
    std::strncpy(u.password_hash, password, sizeof(u.password_hash) - 1);
    u.password_hash[sizeof(u.password_hash) - 1] = 0;
    u.role = role;
    u.created_time = 0;
    u.last_login = 0;
    u.is_active = 1;
    users.push_back(u);
    if (!flush_users_to_disk()) return OFSErrorCodes::ERROR_IO_ERROR;
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystem::user_delete(void* admin_session, const char* username) {
    if (!admin_session || !username) return OFSErrorCodes::ERROR_INVALID_OPERATION;
    if (!session_is_admin(admin_session)) return OFSErrorCodes::ERROR_PERMISSION_DENIED;
    if (std::strncmp(username, "admin", 5) == 0) return OFSErrorCodes::ERROR_INVALID_OPERATION;
    int idx = find_user_index(username);
    if (idx < 0) return OFSErrorCodes::ERROR_NOT_FOUND;
    size_t i = (size_t)idx;
    users[i].is_active = 0;
    users[i].username[0] = 0;
    users[i].password_hash[0] = 0;
    if (!flush_users_to_disk()) return OFSErrorCodes::ERROR_IO_ERROR;
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystem::user_list(void* admin_session, UserInfo** users_out, int* count_out) {
    if (!admin_session || !users_out || !count_out) return OFSErrorCodes::ERROR_INVALID_OPERATION;
    if (!session_is_admin(admin_session)) return OFSErrorCodes::ERROR_PERMISSION_DENIED;
    int active_count = 0;
    for (size_t i = 0; i < users.size(); i++) {
        if (users[i].is_active != 0) active_count++;
    }
    if (active_count == 0) {
        *users_out = 0;
        *count_out = 0;
        return OFSErrorCodes::SUCCESS;
    }
    UserInfo* arr = new UserInfo[(size_t)active_count];
    int pos = 0;
    for (size_t i = 0; i < users.size(); i++) {
        if (users[i].is_active != 0) {
            arr[pos] = users[i];
            pos++;
        }
    }
    *users_out = arr;
    *count_out = active_count;
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystem::get_session_info(void* session, SessionInfo* info) {
    if (!session || !info) return OFSErrorCodes::ERROR_INVALID_OPERATION;
    ActiveSession* s = find_session(session);
    if (!s) return OFSErrorCodes::ERROR_INVALID_SESSION;
    *info = s->info;
    return OFSErrorCodes::SUCCESS;
}

const FSConfig& FileSystem::get_config() const {
    return config;
}

const OMNIHeader& FileSystem::get_header() const {
    return header;
}

const FSLayout& FileSystem::get_layout() const {
    return layout;
}
