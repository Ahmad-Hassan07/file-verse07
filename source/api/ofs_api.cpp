#include <vector>
#include "../core/FileSystem.h"
#include "../core/config_parser.h"
#include "../include/ofs_internal.h"
#include "../include/odf_types.hpp"

extern "C" {

int fs_init(void** instance, const char* omni_path, const char* config_path) {
    if (!instance) return (int)OFSErrorCodes::ERROR_INVALID_OPERATION;
    FileSystem* fs = new FileSystem();
    FSConfig cfg;
    const char* cfg_path = config_path;
    if (!cfg_path || cfg_path[0] == 0) cfg_path = "compiled/default.uconf";
    if (!parse_uconf(cfg_path, cfg)) {
        delete fs;
        return (int)OFSErrorCodes::ERROR_INVALID_CONFIG;
    }
    if (!fs->load_empty(cfg)) {
        delete fs;
        return (int)OFSErrorCodes::ERROR_INVALID_CONFIG;
    }
    *instance = fs;
    return (int)OFSErrorCodes::SUCCESS;
}

void fs_shutdown(void* instance) {
    FileSystem* fs = (FileSystem*)instance;
    delete fs;
}

int fs_format(const char* omni_path, const char* config_path) {
    if (!omni_path) return (int)OFSErrorCodes::ERROR_INVALID_OPERATION;
    FSConfig cfg;
    const char* cfg_path = config_path;
    if (!cfg_path || cfg_path[0] == 0) cfg_path = "compiled/default.uconf";
    if (!parse_uconf(cfg_path, cfg)) return (int)OFSErrorCodes::ERROR_INVALID_CONFIG;
    FileSystem fs;
    if (!fs.format_new(cfg, omni_path)) return (int)OFSErrorCodes::ERROR_IO_ERROR;
    return (int)OFSErrorCodes::SUCCESS;
}

}
