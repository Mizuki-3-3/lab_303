#pragma once
#include "dictionary.hpp"
#include "sorted_sequence.hpp"

class memory_manager {
private:
struct block{
    unsigned int address;
    size_t size;
};
    char* buffer;
    dictionary<void*, size_t> free_memory_adresses;
    sorted_sequence<block> allocated_memory;
public:
    memory_manager();
    ~memory_manager();

    void* operator new(size_t size, void* ptr){
    }

    void operator delete(void* ptr){
    }
};