#include "ofs_api.h"
#include "../core/FileSystem.h"
#include "../core/config_parser.h"

static FileSystem* g_fs = 0;

static int to_int(OFSErrorCodes c) {
    return (int)c;
}

extern "C" {

int fs_format(const char* omni_path, const char* config_path) {
    if (!omni_path || !config_path) 
    return (int)OFSErrorCodes::ERROR_INVALID_CONFIG;
    FSConfig cfg;
    std::memset(&cfg, 0, sizeof(cfg));
    bool ok = parse_uconf(config_path, cfg);
    if (!ok) return (int)OFSErrorCodes::ERROR_INVALID_CONFIG;
    FileSystem fs;
    bool r = fs.format_new(cfg, omni_path);
    if (!r) return (int)OFSErrorCodes::ERROR_IO_ERROR;
    return (int)OFSErrorCodes::SUCCESS;
}

int fs_init(void** instance, const char* omni_path, const char* config_path) {
    if (!instance || !omni_path || !config_path) return (int)OFSErrorCodes::ERROR_INVALID_CONFIG;
    FSConfig cfg;
    std::memset(&cfg, 0, sizeof(cfg));
    bool ok = parse_uconf(config_path, cfg);
    if (!ok) return (int)OFSErrorCodes::ERROR_INVALID_CONFIG;
    FileSystem* fs = new FileSystem();
    bool r = fs->load_existing(cfg, omni_path);
    if (!r) {
        delete fs;
        return (int)OFSErrorCodes::ERROR_IO_ERROR;
    }
    g_fs = fs;
    *instance = fs;
    return (int)OFSErrorCodes::SUCCESS;
}

void fs_shutdown(void* instance) {
    FileSystem* fs = (FileSystem*)instance;
    if (!fs) return;
    if (fs == g_fs) {
        g_fs = 0;
    }
    fs->shutdown();
    delete fs;
}

int user_login(void** session, const char* username, const char* password) {
    if (!g_fs) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    OFSErrorCodes c = g_fs->user_login(username, password, session);
    return to_int(c);
}

int user_logout(void* session) {
    if (!g_fs) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    OFSErrorCodes c = g_fs->user_logout(session);
    return to_int(c);
}

int user_create(void* admin_session, const char* username, const char* password, UserRole role) {
    if (!g_fs) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    OFSErrorCodes c = g_fs->user_create(admin_session, username, password, role);
    return to_int(c);
}

int user_delete(void* admin_session, const char* username) {
    if (!g_fs) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    OFSErrorCodes c = g_fs->user_delete(admin_session, username);
    return to_int(c);
}

int user_list(void* admin_session, UserInfo** users, int* count) {
    if (!g_fs) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    OFSErrorCodes c = g_fs->user_list(admin_session, users, count);
    return to_int(c);
}

int get_session_info(void* session, SessionInfo* info) {
    if (!g_fs) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    OFSErrorCodes c = g_fs->get_session_info(session, info);
    return to_int(c);
}

int file_create(void* session, const char* path, const char* data, size_t size) {
    return (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
}

int file_read(void* session, const char* path, char** buffer, size_t* size) {
    return (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
}

int file_edit(void* session, const char* path, const char* data, size_t size, unsigned int index) {
    return (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
}

int file_delete(void* session, const char* path) {
    return (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
}

int file_truncate(void* session, const char* path) {
    return (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
}

int dir_create(void* session, const char* path) {
    return (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
}

int dir_list(void* session, const char* path, FileEntry** entries, int* count) {
    return (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
}

int dir_delete(void* session, const char* path) {
    return (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
}

int get_metadata(void* session, const char* path, FileMetadata* meta) {
    return (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
}

int set_permissions(void* session, const char* path, uint32_t permissions) {
    return (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
}

int get_stats(void* session, FSStats* stats) {
    return (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
}

void free_buffer(void* buffer) {
    if (!buffer) return;
    char* p = (char*)buffer;
    delete[] p;
}

const char* get_error_message(int error_code) {
    OFSErrorCodes c = (OFSErrorCodes)error_code;
    switch (c) {
        case OFSErrorCodes::SUCCESS: return "Success";
        case OFSErrorCodes::ERROR_NOT_FOUND: return "Not found";
        case OFSErrorCodes::ERROR_PERMISSION_DENIED: return "Permission denied";
        case OFSErrorCodes::ERROR_IO_ERROR: return "I/O error";
        case OFSErrorCodes::ERROR_INVALID_PATH: return "Invalid path";
        case OFSErrorCodes::ERROR_FILE_EXISTS: return "File or directory already exists";
        case OFSErrorCodes::ERROR_NO_SPACE: return "No space";
        case OFSErrorCodes::ERROR_INVALID_CONFIG: return "Invalid config";
        case OFSErrorCodes::ERROR_NOT_IMPLEMENTED: return "Not implemented";
        case OFSErrorCodes::ERROR_INVALID_SESSION: return "Invalid session";
        case OFSErrorCodes::ERROR_DIRECTORY_NOT_EMPTY: return "Directory not empty";
        case OFSErrorCodes::ERROR_INVALID_OPERATION: return "Invalid operation";
        default: return "Unknown error";
    }
}

}






