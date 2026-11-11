#pragma once

#include "shared_ptr.hpp"
#include "weak_ptr.hpp"
#include "array_seq.hpp"
#include <cstddef>
#include <functional>
#include "errors.hpp"


template <typename comparator, typename T>
concept compare = std::strict_weak_order<comparator, T, T>;

template<typename T, typename K, size_t degree = 512, compare<K> compare_ = std::less<K>>//если true то влево, а если false то вправо
class bp_tree{
    //хз как но надо сделать чтобы все значения только в листьях были (ну а зач нам пустой массив во внутренних узлах? только память жрать лишнюю)
    //короче степень это минимальное количество детей в узле, t-1 ключей соотв, max количество клчей 2t-1, max количество детей 2t, в корне от 1 до 2t-1 ключей
    private:
    struct node{
        bool is_leaf;
        int  key_num;

        array_seq<std::pair<K,T>> for_leaf;
        array_seq<std::pair<K, shared_ptr<node>>> for_internal;//TODO отследить какие функции пары использую, чтобы потом написать свою структуру
        
        weak_ptr<node> parent;
        shared_ptr<node> left;
        shared_ptr<node> right;
        node(bool leaf, int num_of_keys): is_leaf(leaf), key_num(num_of_keys){
            if (is_leaf){
                for_leaf = array_seq<std::pair<K,T>>(2*degree);
            }else{
                for_internal = array_seq<std::pair<K, shared_ptr<node>>>(2*degree);
            }
        }
        node(node& other): is_leaf(other.is_leaf), key_num(other.key_num), parent(other.parent), for_leaf(other.for_leaf), for_internal(other.for_internal){}
    };
    //жесть наконец-то внутрянка дерева
    shared_ptr<node> root;
    compare_ comp;

    class iterator{
        shared_ptr<node> current;
        size_t pos;
    public:
        explicit iterator(shared_ptr<node> ptr) noexcept: current(ptr){}
        iterator(const iterator& other): current(other.current){}
        ~iterator() = default;
        std::pair<K, T>& operator*(){return current->for_leaf[pos];}
        std::pair<K, T>* operator->(){return &(current->for_leaf[pos]);}
        bool operator==(const iterator& other) const {
        return current == other.current && pos == other.pos;
        }
        bool operator!=(const iterator& other) const {
        return !(*this == other);
        }
        iterator& operator++(){
            if (pos + 1 < current->key_num) {
                ++pos;
            } else if(current->right) {
                current = current->right;
                pos = 0;
            }else {
                current = shared_ptr<node>();
                pos = 0;
            }
            return *this;
        }
        iterator operator++(int){
            iterator tmp = *this;
            if (pos + 1 < current->key_num) {
                ++pos;
            } else if(current->right) {
                current = current->right;
                pos = 0;
            }else {
                current = shared_ptr<node>();
                pos = 0;
            }
            return tmp;
        }
        iterator operator--(){
            if (!pos) {
                --pos;
            } else if(current->left) {
                current = current->left;
                pos = current->key_num - 1;
            }else {
                current = shared_ptr<node>();
                pos = 0;
            }
            return *this;
        }
        iterator operator--(int){
            iterator tmp = *this;
            if (!pos) {
                --pos;
            } else if(current->left) {
                current = current->left;
                pos = current->key_num - 1;
            }else {
                current = shared_ptr<node>();
                pos = 0;
            }
            return tmp;
        }
    };
    //надо еще подогнать конст итератор
    class const_iterator{
        shared_ptr<node> current;
        size_t pos;
    public:
        explicit const_iterator(shared_ptr<node> ptr) noexcept: current(ptr){}
        const_iterator(const iterator& other): current(other.current), pos(other.pos){}
        ~const_iterator() = default;
        const std::pair<K, T>& operator*(){return current->for_leaf[pos];}
        std::pair<K, T>* operator->(){return &(current->for_leaf[pos]);}
        bool operator==(const const_iterator& other) const {
        return current == other.current && pos == other.pos;
        }
        bool operator!=(const const_iterator& other) const {
        return !(*this == other);
        }
        const_iterator& operator++(){
            if (pos + 1 < current->key_num) {
                ++pos;
            } else if(current->right) {
                current = current->right;
                pos = 0;
            }else {
                current = shared_ptr<node>();
                pos = 0;
            }
            return *this;
        }
        const_iterator operator++(int){
            const_iterator tmp = *this;
            if (pos + 1 < current->key_num) {
                ++pos;
            } else if(current->right) {
                current = current->right;
                pos = 0;
            }else {
                current = shared_ptr<node>();
                pos = 0;
            }
            return tmp;
        }
        const_iterator operator--(){
            if (!pos) {
                --pos;
            } else if(current->left) {
                current = current->left;
                pos = current->key_num - 1;
            }else {
                current = shared_ptr<node>();
                pos = 0;
            }
            return *this;
        }
        const_iterator operator--(int){
            const_iterator tmp = *this;
            if (!pos) {
                --pos;
            } else if(current->left) {
                current = current->left;
                pos = current->key_num - 1;
            }else {
                current = shared_ptr<node>();
                pos = 0;
            }
            return tmp;
        }
    };

    bool equal(const K& key1, const K& key2)const{
        return !comp(key1, key2)&&!comp(key2, key1);
    }
    shared_ptr<node> down_to_leaf(const K& key) const {//получаем лист В КОТОРОМ МОЖЕТ НЕ БЫТЬ КЛЮЧА 
        if (!root){throw empty_container("this tree is empty");}
        auto current = root;
        while (!current->is_leaf){
            for (int i = 0; i < current->key_num; ++i){
                if (i == current->key_num || comp(key, current->for_internal[i].first)){
                    current = current->for_internal[i].second;
                    break;
                    std::pair<K, shared_ptr<node>> p = current->for_internal[i];
                }
            }
        }
        return current;
    }
    void split_node(shared_ptr<node>& to_split){//чето  тут за листьями и внутренними следить надо, тяжело
        shared_ptr<node> new_node = shared_ptr<node>::make_shared(to_split->is_leaf, 0);
        if (to_split->is_leaf){
            new_node->right = to_split->right;
            to_split->right->left = new_node;
            to_split->right = new_node;
            new_node->left = to_split;
        }

        K mid = to_split->is_leaf? to_split->for_leaf[degree].first: to_split->for_internal[degree].first;
        new_node->key_num=degree-1;
        to_split->key_num = degree;

        if (to_split->is_leaf){
            ++new_node->key_num;
            for (size_t i = 0; i<degree;++i){
                new_node->for_leaf[i] = to_split->for_leaf[i + degree];
            }
        }else{
            for (size_t i = 0; i<degree;++i){
                new_node->for_internal[i] = to_split->for_internal[i + degree];
            }
            new_node->for_internal[new_node->key_num] = to_split->for_internal[2 * degree];//тут нам не важен какой ключ, нам нужен только ребенок
        }

        if (to_split == root){
            root = shared_ptr<node>::make_shared(false, 1);
            root->for_internal[0] = std::pair<K, shared_ptr<node>>(mid, to_split);
            root->for_internal[1].second = new_node;
            to_split->parent = root;
            new_node->parent = root;

        }else{
            new_node->parent = to_split->parent;
            shared_ptr<node> parent = to_split->parent.lock();
            
            size_t pos = 0;
            while (pos < parent->key_num and comp(parent->for_internal[pos].first, mid)){++pos;}
            
            for (size_t i = parent->key_num+1; i> pos; --i){parent->for_internal[i] = parent->for_internal[i - 1];}
                
            parent->for_internal[pos].first = mid;
            parent->for_internal[pos + 1].second = new_node;
            ++parent->key_num;
            
            if (parent->key_num == 2 * degree){split_node(parent);}
        }
    }
    void delete_in_node(shared_ptr<node> leaf, const K& key){
        size_t pos = 0;
        while (pos < leaf->key_num and !equal(leaf->keys[pos], key)){++pos;}
        if (leaf->keys[pos] != key){return;}
        for (size_t i = pos; i < leaf->key_num; ++i){
            leaf->keys[i] = leaf->keys[i + 1];
            leaf.get()->data[i] = leaf->data[i + 1];
        }
        if (!leaf->is_leaf){
            for (size_t i = pos+1; i <= leaf->key_num; ++i){
                leaf->children[i] = leaf->children[i + 1];
            }
        }
        --leaf->key_num;
        if (leaf->key_num<degree-1){
            auto right_brother = leaf->right;
            auto left_brother = leaf->left;
            if (left_brother and left_brother->key_num>degree-1){
                --left_brother->key_num;
                ++leaf->key_num;
                for (size_t i = 1; i < leaf->key_num; ++i){
                    leaf->keys[i] = leaf->keys[i - 1];
                    leaf->data[i] = leaf->data[i - 1];
                    leaf->children[i] = leaf->children[i - 1];
                }
                leaf->keys[0] = left_brother->keys[left_brother->key_num];
                leaf->data[0] = left_brother->data[left_brother->key_num];
                leaf->children[leaf->key_num] = leaf->children[leaf->key_num - 1];
                leaf->children[0] = left_brother->children[left_brother->key_num+1];
                update(leaf);
            }else if (right_brother and right_brother->key_num>degree-1){
                --right_brother->key_num;
                ++leaf->key_num;
                for (size_t i = 1; i < leaf->key_num; ++i){
                    leaf->keys[i] = leaf->keys[i - 1];
                    leaf->data[i] = leaf->data[i - 1];
                    leaf->children[i] = leaf->children[i - 1];
                }
                leaf->keys[leaf->key_num-1] = right_brother->keys[0];
                leaf->data[leaf->key_num-1] = right_brother->data[0];
                leaf->children[leaf->key_num-1] = right_brother->children[0];
                update(leaf);
            }else{
                if (left_brother){
                    for (size_t i = 0; i < leaf->key_num;++i){
                        left_brother->keys[left_brother->key_num]=leaf->keys[i];
                        left_brother->data[left_brother->key_num]=leaf->data[i];
                        left_brother->children[left_brother->key_num+1]=leaf->children[i];
                        ++left_brother->key_num;
                    }
                    left_brother->children[left_brother->key_num+1]=leaf->children[leaf->key_num];
                    left_brother->right = leaf->right;
                    if (leaf->right){leaf->right->left = left_brother;}
                    update(left_brother);
                    delete_in_node(leaf->parent.lock(), leaf->keys[0]);
                }else{
                    for (size_t i = 0; i < right_brother->key_num;++i){
                        leaf->keys[leaf->key_num]=right_brother->keys[i];
                        leaf->data[leaf->key_num]=right_brother->data[i];
                        leaf->children[leaf->key_num+1]=right_brother->children[i];
                        ++leaf->key_num;
                    }
                    leaf->children[leaf->key_num+1]=right_brother->children[right_brother->key_num];
                    leaf->right = right_brother->right;
                    if (right_brother->right){right_brother->right->left = leaf;}
                    update(leaf);
                    delete_in_node(right_brother->parent.lock(), right_brother->keys[0]);
                }
            }
            if (root->key_num==1){
                root = root->children[0];
            }
        }
    }
    template<typename U>
    int bin_search(const array_seq<std::pair<K,U>>& arr, const K& key, unsigned short key_count) const{
        unsigned short l = 0, r = key_count;
        while (r!=l){
            size_t mid = l+(r-l)/2;
            if (equal(arr[mid].first,key)){return mid;}
            else if (!comp(arr[mid].first,key)){r = mid-1;}
            else{l = mid+1;}
        }
        if (l < key_count && equal(arr[l].first, key)){
            return l;
        }
        return -1;
    }
public:
    bp_tree():root(){}
    
    T search(const K& key) const{
        auto leaf = down_to_leaf(key);
        int pos = bin_search(leaf->for_leaf, key, leaf->key_num);
        if (pos != -1){return leaf->for_leaf[pos].second;}
        throw not_found("no such key in bp_tree");
    }
    bool insert(const K& key, const T& value){//не заменяет существующие значения а возвращаем false
        if (!root){
            root = shared_ptr<node>::make_shared(true, 1);
            root->for_leaf[0] = std::pair(key, value);
            return true;
        }
        shared_ptr<node> noda = down_to_leaf(key);
        size_t pos = 0;
        while (pos < noda->key_num && !comp(key, noda->for_leaf[pos].first))++pos;
        if (pos > 0 && equal(noda->for_leaf[pos-1].first, key)){return false;}
        for (size_t i = noda->key_num; i >pos ;--i){
            noda->for_leaf[i]= noda->for_leaf[i-1];
        }
        noda->for_leaf[pos] = std::pair(key, value);
        ++noda->key_num;
        //вот только размер массива для листа меньше 2*deg а значит мы теряем одну пару
        if (noda->key_num == 2*degree) split_node(noda);//на этом моменте всё хорошее во вставке кончается...
        return true;
    }
    bool remove(const K& key){//если удалили то тру если ключа не было то ложь
        auto leaf = down_to_leaf(key);
        int pos = -1;//если обновится то будет больше равен нулю
        size_t l = 0, r = leaf->key_num;
        while (r!=l){
            size_t pos_in_l = (r+l)/2;
            if (equal(leaf.keys[pos_in_l],key)){pos = pos_in_l;}
            else if (comp(leaf.keys[pos_in_l],key)){r = pos_in_l;}
            else{l = pos_in_l;}
        }
        if (pos == -1){return false;}
        else {
            delete_in_node(leaf, key);
            return true;
        }
    }
};