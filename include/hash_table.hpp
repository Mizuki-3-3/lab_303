#pragma once

#include "errors.hpp"
#include <array_seq.hpp>
#include <list_seq.hpp>

template <typename T, typename K, typename HASH = std::hash<K>>
class hash_table{
private:
    static constexpr float load_factor = 0.75;
    HASH hash;
    array_seq<list_seq<std::pair<K, T>>> table;
public:
void insert(const K& key, const T& value){
        size_t index = hash(key) % table.size();
        for (auto& pair: table[index]){
            if (pair.first == key){
                pair.second = value;
                return;
            }
        }
        table[index].push_back({key, value});
    }

    T search(const K& key){
        size_t index = hash(key) % table.size();
        for (auto& pair: table[index]){
            if (pair.first == key){
                return pair.second;
            }
        }
        throw not_found("Key not found");
    }

    void remove(const K& key){
        size_t index = hash(key) % table.size();
        auto& bucket = table[index];
        for (auto it = bucket.begin(); it != bucket.end(); ++it){
            if (it->first == key){
                bucket.erase(it);
                return;
            }
        }
        throw not_found("Key not found");
    }
};