#pragma once
#include "bp_tree.hpp"

template <typename T>
class sorted_sequence{
    bp_tree<int, T> tree;
};
