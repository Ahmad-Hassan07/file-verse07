#pragma once
#include <vector>
#include "SinglyLinkedList.h"

template <typename K>
struct DefaultKeyEq {
    bool operator()(const K& a, const K& b) const { return !(a < b) && !(b < a); }
};

template <typename K>
struct DefaultHasher {
    unsigned long long operator()(const K& v) const {
        const unsigned char* p = reinterpret_cast<const unsigned char*>(&v);
        unsigned long long h = 5381;
        for (unsigned long long i = 0; i < sizeof(K); i++) h = ((h << 5) + h) + p[i];
        return h;
    }
};

template <>
struct DefaultHasher<std::vector<unsigned char>> {
    unsigned long long operator()(const std::vector<unsigned char>& v) const {
        unsigned long long h = 5381;
        for (unsigned long long i = 0; i < v.size(); i++) h = ((h << 5) + h) + v[i];
        return h;
    }
};

template <typename K, typename V, typename Hasher = DefaultHasher<K>, typename KeyEq = DefaultKeyEq<K>>
class HashTable {
    struct Pair { K k; V v; Pair(const K& kk, const V& vv): k(kk), v(vv) {} };
    std::vector<SinglyLinkedList<Pair>> buckets;
    unsigned long long n;
    Hasher hasher;
    KeyEq eq;
    unsigned long long idx(const K& k) const { return hasher(k) % buckets.size(); }
public:
    HashTable(unsigned long long cap = 128): buckets(cap), n(0) {}
    bool put(const K& k, const V& v) {
        unsigned long long i = idx(k);
        bool updated = false;
        std::vector<Pair> temp;
        buckets[i].for_each([&](Pair p) { temp.push_back(p); });
        buckets[i].clear();
        for (unsigned long long j = 0; j < temp.size(); j++) {
            if (eq(temp[j].k, k)) {
                buckets[i].push_back(Pair(k, v));
                updated = true;
            } else {
                buckets[i].push_back(temp[j]);
            }
        }
        if (!updated) {
            buckets[i].push_back(Pair(k, v));
            n++;
        }
        return true;
    }
    bool get(const K& k, V& out) const {
        unsigned long long i = idx(k);
        bool found = false;
        buckets[i].for_each([&](Pair p) { if (!found && eq(p.k, k)) { out = p.v; found = true; } });
        return found;
    }
    bool contains(const K& k) const {
        V v;
        return get(k, v);
    }
    bool erase(const K& k) {
        unsigned long long i = idx(k);
        std::vector<Pair> temp;
        bool removed = false;
        buckets[i].for_each([&](Pair p) { if (!eq(p.k, k)) temp.push_back(p); else removed = true; });
        buckets[i].clear();
        for (unsigned long long j = 0; j < temp.size(); j++) buckets[i].push_back(temp[j]);
        if (removed) n--;
        return removed;
    }
    unsigned long long size() const { return n; }
    std::vector<K> keys() const {
        std::vector<K> ks;
        for (unsigned long long i = 0; i < buckets.size(); i++) {
            buckets[i].for_each([&](Pair p) { ks.push_back(p.k); });
        }
        return ks;
    }
};
