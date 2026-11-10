#pragma once
#include "hash_table.hpp"

template <typename T, typename K>
class dictionary{
    hash_table<T, K> table;
};
