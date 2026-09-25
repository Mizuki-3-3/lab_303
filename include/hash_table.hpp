#pragma once
#include <cstddef>
#include "array_seq.hpp"
#include <list_seq.hpp>
#include "errors.hpp"


template <typename KEY, typename ELEMENT, typename Hash = std::hash<KEY>>
class hash_table {
private:
    struct Node {
        KEY key;
        ELEMENT value;
        Node (const KEY& k, const ELEMENT& v) : key(k), value(v) {}
    };

    array_seq<list_seq<Node>> table;
    size_t count_ = 0;
    Hash hasher_;
    static constexpr double MaxLoadFactor = 0.75;

    size_t bucketIndex(const KEY& key, size_t bucketCount) const {
        return hasher_(key) % bucketCount;
    }

    void rehash() {
        size_t new_cap = table.size() * 2;
        array_seq<list_seq<Node>> new_table(new_cap);

        for (auto& bucket : table) {
            for (auto& node : bucket) {
                size_t idx = bucketIndex(node.key, new_cap);
                new_table[idx].push_back(std::move(node));
            }
        }
        table = std::move(new_table);
    }

public:
    explicit hash_table(size_t initialCapacity = 1024)
        : table(initialCapacity) {}

    void insert(const KEY& key, const ELEMENT& value) {
        if (static_cast<double>(count_ + 1) / table.size() > MaxLoadFactor) {
            rehash();
        }

        size_t idx = bucketIndex(key, table.size());
        for (auto& node : table[idx]) {
            if (node.key == key) {
                node.value = value;
                return;
            }
        }
        table[idx].push_back(Node{key, value});
        ++count_;
    }

    ELEMENT find(const KEY& key) const {
        size_t idx = bucketIndex(key, table.size());
        for (const auto& node : table[idx]) {
            if (node.key == key) {
                return node.value;
            }
        }
        throw not_found("Key not found in hash table");
    }

    bool contains(const KEY& key) const {
        try {
            find(key);
            return true;
        }catch(not_found&){return false;}
    }

    void remove(const KEY& key) {
        size_t idx = bucketIndex(key, table.size());
        list_seq<Node>& bucket = table[idx];
        for (auto it = bucket.begin(); it != bucket.end(); it++) {
            if (it->key == key) {
                bucket.erase(it);
                --count_;
                return;
            }
        }
        throw not_found("No such key in hash-table to remove");
    }

    size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }
    size_t bucketCount() const { return table.size(); }
    double loadFactor() const {
        return static_cast<double>(count_) / table.size();
    }

};

#include "hash_table.cpp"