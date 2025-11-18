#ifndef OFS_API_H
#define OFS_API_H

#include"../include/odf_types.hpp"

extern "C" {

int fs_format(const char* omni_path, const char* config_path);
int fs_init(void** instance, const char* omni_path, const char* config_path);
void fs_shutdown(void* instance);

int user_login(void** session, const char* username, const char* password);
int user_logout(void* session);
int user_create(void* admin_session, const char* username, const char* password, UserRole role);
int user_delete(void* admin_session, const char* username);
int user_list(void* admin_session, UserInfo** users, int* count);
int get_session_info(void* session, SessionInfo* info);

int file_create(void* session, const char* path, const char* data, size_t size);
int file_read(void* session, const char* path, char** buffer, size_t* size);
int file_edit(void* session, const char* path, const char* data, size_t size, unsigned int index);
int file_delete(void* session, const char* path);
int file_truncate(void* session, const char* path);

int dir_create(void* session, const char* path);
int dir_list(void* session, const char* path, FileEntry** entries, int* count);
int dir_delete(void* session, const char* path);

int get_metadata(void* session, const char* path, FileMetadata* meta);
int set_permissions(void* session, const char* path, uint32_t permissions);
int get_stats(void* session, FSStats* stats);

void free_buffer(void* buffer);
const char* get_error_message(int error_code);

}

#endif
