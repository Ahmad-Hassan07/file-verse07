#pragma once
#include <vector>
#include "../core/FileSystem.h"

extern "C" {
int fs_init(void** instance, const char* omni_path, const char* config_path);
void fs_shutdown(void* instance);
int fs_format(const char* omni_path, const char* config_path);
}
