#pragma once

#include <shared_ptr.hpp>
#include "array_seq.hpp"
#include <cstddef>
#include <functional>
#include "errors.hpp"

template <typename comparator, typename T>
concept compare = std::strict_weak_order<comparator, T, T>;

template<typename T, typename K, size_t degree = 1024, compare<K> compare_ = std::less<K>>//если true то влево, а если false то вправо
class bp_tree{
//хз как но надо сделать чтобы все значения только в листьях были (ну а зач нам пустой массив во внутренних узлах? только память жрать лишнюю)
//короче степень это минимальное количество детей в узле, t-1 ключей соотв, max количество клчей 2t-1, max количество детей 2t, в корне от 1 до 2t-1 ключей
private:
    struct node{
        bool is_leaf;
        int  key_num;
        array_seq<K>  keys;
        weak_ptr<node> parent = shared_ptr<node>();
        array_seq<shared_ptr<node>> children;
        array_seq<T> data;
        shared_ptr<node> left= shared_ptr<node>();
        shared_ptr<node> right= shared_ptr<node>();
        node(bool leaf, int num_of_keys): is_leaf(leaf), key_num(num_of_keys){}
    };
    shared_ptr<node> root;
    compare_ comp;
    bool equal(const K& key1, const K& key2){
        return !comp(key1, key2)&&!comp(key2, key1);
    }
    shared_ptr<node> down_to_leaf(const K& key){
        if (!root){throw empty_container("this tree is empty");}
        auto current = root;
        while (!current->is_leaf){
            for (int i = 0; i < current->key_num; ++i){
                if (i == current->key_num ||comp(key, current->keys[i])){
                    current = current->children[i];
                    break;
                }
            }
        }
        return current;
    }
    void split_node(shared_ptr<node> to_split){
        
        shared_ptr<node> new_node = shared_ptr<node>();

        new_node->right = to_split->right;
        to_split->right->left = new_node;
        to_split->right = new_node;
        new_node->left = to_split;

        K mid = to_split->keys[degree];
        new_node->key_num=degree-1;
        to_split->key_num = degree;
        for (size_t i = 0; i<degree-1;++i){
            new_node->keys[i] = to_split->keys[i + degree];
            new_node->data[i] = to_split->data[i + degree];
            new_node->children[i] = to_split->children[i + degree];
        }
        new_node->children[new_node.key_num] = to_split->children[2 * degree];
    
        if (to_split->is_leaf){
            ++new_node.key_num;
            new_node->is_leaf = true;
        // Перемещаем в new_node оставшийся при разбиении элемент mid_key 
            for (size_t i = new_node->key_num - 1; i>1; --i){
                new_node->keys[i] = new_node->keys[i - 1];
                new_node->data[i] = new_node->data[i - 1];
            }
            new_node->keys[0] = to_split->keys[degree];
            new_node->data[0] = to_split->data[degree];
        }
        if (to_split == root){
            root = shared_ptr<node>(false, 1);                  // Создаем новый корень T.root 
            root->keys[0] = mid;
            root->children[0] = to_split;
            root->children[1] = new_node;
            root->key_num = 1;
            to_split->parent = root;
            new_node->parent = root;

        }else{
            new_node->parent = to_split->parent;
            shared_ptr<node> parent = to_split->parent.lock();
            
            // Ищем позицию mid_key в отце 
            size_t pos = 0;
            while (pos < parent.key_num and !comp(parent.key[pos], mid)){++pos;}
            
            // Добавляем mid_key в отца и направляем ссылку из него на new_node 
            for (size_t i = parent->key_num; i> pos; --i){parent->keys[i] = parent->keys[i - 1];}
            for (size_t i = parent->key_num + 1; i>pos+1; --i){ parent->children[i] = parent->children[i - 1];}
                
            parent->keys[pos] = mid;
            parent->children[pos + 1] = new_node;
            ++parent->key_num;
            
            if (parent->key_num == 2 * degree){split(parent);}
        }
    }
    void delete_in_node(shared_ptr<node> leaf, const K& key){
        size_t pos = 0;
        while (pos < leaf->key_num and !equal(leaf->keys[pos], key)){++pos;}
        if (leaf->keys[pos] != key){return;}
        for (size_t i = pos; i < leaf->key_num; ++i){
            leaf->keys[i] = leaf->keys[i + 1];
            leaf->data[i] = leaf->data[i + 1];
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

public:
    bp_tree():root(){}
    

    T search(const K& key){
        auto leaf = down_to_leaf(key);
        int l = 0, r = leaf.key_num;
        while (r!=l){
            int pos = (r+l)/2;
            if (equal(leaf.keys[pos],key)){return leaf.data[pos];}
            else if (comp(leaf.keys[pos],key)){r = pos;}
            else{l = pos;}
        }
        throw not_found("no such key in bp_tree");
    }
    bool insert(const K& key, const T& value){
        if (!root){
            root = shared_ptr<node>(true, 1);
            root->keys = array_seq<K>(degree);
            root->keys[1] = key;
            root->data = array_seq<T>(degree);
            root->data[1] = value;
            return true;
        }
        shared_ptr<node> noda = down_to_leaf(key);
        size_t pos = 0;
        while (pos < noda->key_num and !comp(noda->keys[pos], key))++pos;
        for (size_t i = noda->key_num; i >pos ;--i){
            noda->keys[i]= noda->keys[i-1];
            noda->data[i] = noda->data[i-1];
        }
        noda->keys[pos] = key;
        noda->data[pos]= value;
        ++noda->key_num;

        if (noda->key_num == 2*degree) split_node(noda);                   // Разбиваем узел
        return true;
    }
    bool remove(const K& key){
        auto leaf = down_to_leaf(key);
        int pos = -1;//если обновится то будет больше равен нулю
        size_t l = 0, r = leaf->key_num;
        while (r!=l){
            size_t pos_in_l = (r+l)/2;
            if (equal(leaf.keys[pos_in_l],key)){return pos = pos_in_l;}
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