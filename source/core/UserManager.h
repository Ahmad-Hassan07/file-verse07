#pragma once
#include <vector>
#include "../include/odf_types.hpp"
#include "../include/ofs_internal.h"
#include "../data_structures/HashTable.h"

class UserManager {
    std::vector<UserInfo> users;
    HashTable<unsigned long long, unsigned int> uname_to_index;
    HashTable<unsigned long long, SessionInfo> sessions;
    unsigned long long key_of_username(const char name[32]) const {
        unsigned long long h = 1469598103934665603ull;
        for (int i = 0; i < 32; i++) { unsigned char b = (unsigned char)name[i]; h ^= b; h *= 1099511628211ull; }
        return h;
    }
public:
    UserManager() {}
    void load(const std::vector<UserInfo>& list) {
        users = list;
        for (unsigned int i = 0; i < users.size(); i++) {
            unsigned long long k = key_of_username(users[i].username);
            uname_to_index.put(k, i);
        }
    }
    bool find_user(const char name[32], UserInfo& out) const {
        unsigned long long k = key_of_username(name);
        unsigned int idx = 0;
        if (!uname_to_index.get(k, idx)) return false;
        out = users[idx];
        return true;
    }
    bool all(std::vector<UserInfo>& out) const {
        out = users;
        return true;
    }
    bool add_user(const UserInfo& u) {
        unsigned long long k = key_of_username(u.username);
        unsigned int dummy = 0;
        if (uname_to_index.get(k, dummy)) return false;
        users.push_back(u);
        uname_to_index.put(k, (unsigned int)(users.size() - 1));
        return true;
    }
    bool remove_user(const char name[32]) {
        unsigned long long k = key_of_username(name);
        unsigned int idx = 0;
        if (!uname_to_index.get(k, idx)) return false;
        unsigned int last = (unsigned int)(users.size() - 1);
        users[idx] = users[last];
        users.pop_back();
        uname_to_index.erase(k);
        if (idx < users.size()) {
            unsigned long long k2 = key_of_username(users[idx].username);
            uname_to_index.put(k2, idx);
        }
        return true;
    }
    bool open_session(const SessionInfo& s) {
        unsigned long long h = 1469598103934665603ull;
        for (int i = 0; i < 64; i++) { unsigned char b = (unsigned char)s.session_id[i]; h ^= b; h *= 1099511628211ull; }
        sessions.put(h, s);
        return true;
    }
    bool get_session(const char sid[64], SessionInfo& out) const {
        unsigned long long h = 1469598103934665603ull;
        for (int i = 0; i < 64; i++) { unsigned char b = (unsigned char)sid[i]; h ^= b; h *= 1099511628211ull; }
        return sessions.get(h, out);
    }
    bool close_session(const char sid[64]) {
        unsigned long long h = 1469598103934665603ull;
        for (int i = 0; i < 64; i++) { unsigned char b = (unsigned char)sid[i]; h ^= b; h *= 1099511628211ull; }
        return sessions.erase(h);
    }
};
