#pragma once
#include <vector>
#include "../data_structures/Stack.h"

class FreeSpaceManager {
    std::vector<unsigned char>* bitmap;
    unsigned int blocks;
    Stack<unsigned int> free_stack;
public:
    FreeSpaceManager() {
        bitmap = nullptr;
        blocks = 0;
    }
    void bind(std::vector<unsigned char>* bm, unsigned int total_blocks);
    bool mark_used(unsigned int i);
    bool mark_free(unsigned int i);
    bool allocate_one(unsigned int& out);
};
