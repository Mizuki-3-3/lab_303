#pragma once

#include "shared_ptr.hpp"
#include "weak_ptr.hpp"
#include "array_seq.hpp"
#include <cstddef>
#include <functional>
#include "pair.hpp"
#include <iostream>

template <typename comparator, typename T>
concept compare = std::strict_weak_order<comparator, T, T>;

//надо добавить реквайр на степень >=2
template<typename T, typename K, size_t degree = 512, compare<K> compare_ = std::less<K>>//если true то влево, а если false то вправо
class bp_tree{
    //хз как но надо сделать чтобы все значения только в листьях были (ну а зач нам пустой массив во внутренних узлах? только память жрать лишнюю)
    //короче степень это минимальное количество детей в узле, t-1 ключей соотв, max количество клчей 2t-1, max количество детей 2t, в корне от 1 до 2t-1 ключей
    private:
    struct node{
        bool is_leaf;
        int  key_num;

        array_seq<pair<K,T>> for_leaf;//2t-1 ключей максимум
        array_seq<pair<K, shared_ptr<node>>> for_internal;
        
        weak_ptr<node> parent;
        shared_ptr<node> left;
        weak_ptr<node> right;//теперь слабый потому что ЦИКЛЫ СРАНЫЕ НЕ СХОПЫВАЕТСЯ ДЕРЕВО КАК ЗАДУМЫВАЛОСЬ ААААААААААААААААААААААААААА
        node(bool leaf, int num_of_keys): is_leaf(leaf), key_num(num_of_keys){
            if (is_leaf){
                for_leaf = array_seq<pair<K,T>>(2*degree);
            }else{
                for_internal = array_seq<pair<K, shared_ptr<node>>>(2*degree+1);
            }
        }
        node(node& other): is_leaf(other.is_leaf), key_num(other.key_num), parent(other.parent), for_leaf(other.for_leaf), for_internal(other.for_internal){}
    };
    //жесть наконец-то внутрянка дерева
    shared_ptr<node> root;
    compare_ comp;

    bool equal(const K& key1, const K& key2)const{
        return !comp(key1, key2)&&!comp(key2, key1);
    }
    shared_ptr<node> down_to_leaf(const K& key) const {//получаем лист В КОТОРОМ МОЖЕТ НЕ БЫТЬ КЛЮЧА 
        if (!root){throw empty_container("this tree is empty");}
        auto current = root;
        while (!current->is_leaf){
            size_t i = 0;
            while (i < current->key_num && !comp(key, current->for_internal[i].first))++i;
            current = current->for_internal[i].second;
        }
        return current;
    }
    void split_node(shared_ptr<node>& to_split){//чето  тут за листьями и внутренними следить надо, тяжело
        shared_ptr<node> new_node = shared_ptr<node>::make_shared(to_split->is_leaf, 0);
        if (to_split->is_leaf){
            new_node->right = to_split->right;
            if (to_split->right)to_split->right->left = new_node;
            to_split->right = new_node;
            new_node->left = to_split;
        }

        K mid = to_split->is_leaf? to_split->for_leaf[degree].first: to_split->for_internal[degree].first;
        to_split->key_num = degree;
        new_node->key_num=degree-1;

        if (to_split->is_leaf){//тут перемещаем данные. для листа это t последних значений а во внутренних t-1 последних
            ++new_node->key_num;
            for (size_t i = 0; i<degree;++i){
                new_node->for_leaf[i] = to_split->for_leaf[i + degree];//тут вроде норм, надо толкьо вытолкнуть средний ключ
            }
        }else{
            for (size_t i = 0; i < degree - 1; ++i)
                new_node->for_internal[i] = to_split->for_internal[i + 1 + degree];
            new_node->for_internal[degree - 1].second = to_split->for_internal[2 * degree].second;

            for (size_t i = 0; i < degree; ++i)
                new_node->for_internal[i].second->parent = new_node;

            for (size_t i = degree + 1; i <= 2 * degree; ++i)
                to_split->for_internal[i] = pair<K, shared_ptr<node>>(K(), shared_ptr<node>());
        }

        if (to_split == root){
            root = shared_ptr<node>::make_shared(false, 1);//почему-то у корня количество ключей 2....
            root->for_internal[0] = pair<K, shared_ptr<node>>(mid, to_split);
            root->for_internal[1].second = new_node;
            to_split->parent = root;
            new_node->parent = root;

        }else{
            new_node->parent = to_split->parent;
            shared_ptr<node> parent = to_split->parent.lock();
            
            size_t pos = 0;
            while (pos < parent->key_num && !comp(mid, parent->for_internal[pos].first)){++pos;}//??? какая тут позиия на одну раньше позже или прям она?
            
            for (size_t i = parent->key_num+1; i> pos+1; --i){parent->for_internal[i] = parent->for_internal[i - 1];}
                
            parent->for_internal[pos+1]= pair<K, shared_ptr<node>>(parent->for_internal[pos].first, new_node);
            parent->for_internal[pos].first = mid;
            ++parent->key_num;
            
            if (parent->key_num == 2 * degree){split_node(parent);}//где-то тут зацикливается госпдииииииииииии
        }
    }

    //функция для поднятия ключа в родителя
    //сюда передаем именно родителя и позицию ключа, который надо будет провернуть
    void rotate_right_internal_node(shared_ptr<node> noda, size_t pos_in_parent){
        
        shared_ptr<node> left = noda->for_internal[pos_in_parent-1].second;
        shared_ptr<node> actual = noda->for_internal[pos_in_parent].second;
        for (size_t i = actual->key_num+1;i>0;--i){
            actual->for_internal[i]=actual->for_internal[i-1];
        }
        actual->for_internal[0] = pair<K,node>({noda->for_internal[pos_in_parent-1], left->for_internal[left->key_num].second});//спустили ключ из родителя и закинули ребенка
        actual->for_internal[0].second->parent = actual;//ребенку родителя обновили
        noda->for_internal[pos_in_parent-1].first = left->for_internal[left->key_num-1].first;//подняли ключ
        ++actual->key_num;
        --left->key_num;
    }
    void rotate_left_internal_node(shared_ptr<node> noda, size_t pos_in_parent){
        shared_ptr<node> actual = noda->for_internal[pos_in_parent].second;
        shared_ptr<node> right = noda->for_internal[pos_in_parent+1].second;
        actual->for_internal[actual->key_num].first = noda->for_internal[pos_in_parent].first;//ключ родителя стал последним ключом actual
        actual->for_internal[actual->key_num+1].second = right->for_internal[0].second;//первый ребенок брата стал последним ребенком actual
        actual->for_internal[actual->key_num+1].second->parent = actual;
        noda->for_internal[pos_in_parent].first = right->for_internal[0].first;//первый ключ брата ушел в родителя
        for (size_t i = 0; i<right->key_num; ++i){
            right->for_internal[i] = right->for_internal[i+1];
        }
        right->for_internal[right->key_num] = pair<K, shared_ptr<node>>(K(), shared_ptr<node>());
        --right->key_num;
        ++actual->key_num;
    }
    //функции переноса крайнего ключа в нужную нам ноду
    //закидываем сюда лист в который перекидываем ключ и его индекс в родителе
    void take_key_from_left_leaf(shared_ptr<node> noda, size_t pos_in_p){
        shared_ptr<node> left = noda->left;
        shared_ptr<node> parent = noda->parent.lock();
        for (size_t i = noda->key_num;i>0;--i){
            noda->for_leaf[i]=noda->for_leaf[i-1];
        }
        noda->for_leaf[0] = left->for_leaf[left->key_num-1];
        --left->key_num;
        ++noda->key_num;
        parent->for_internal[pos_in_p-1].first = noda->for_leaf[0].first;
    }

    void take_key_from_right_leaf(shared_ptr<node> noda, size_t pos_in_p){
        shared_ptr<node> right = noda->right;
        shared_ptr<node> parent = noda->parent.lock();
        noda->for_leaf[noda->key_num] = right->for_leaf[0];
        ++noda->key_num;
        
        for (size_t i = 0; i+1<right->key_num; ++i){
            right->for_leaf[i] = right->for_leaf[i+1];
        }
        --right->key_num;
        parent->for_internal[pos_in_p+1].first = right->for_leaf[0].first;
    }

    void merge_children(shared_ptr<node> noda, size_t i){
        shared_ptr<node> left = noda->for_internal[i].second;
        shared_ptr<node> right = noda->for_internal[i+1].second;
        
        if (left->is_leaf){
            for (size_t j = 0; j<right->key_num; ++j){
                left->for_leaf[left->key_num + j] = right->for_leaf[j];
            }
            left->key_num += right->key_num;
            left->right = right->right;//выкидываем right из списка листьев
            if (left->right) left->right->left = left;
        }else{
            left->for_internal[left->key_num].first = noda->for_internal[i].first;//разделитель из родителя спускается вниз
            for (size_t j = 0; j<=right->key_num; ++j){//переносим все пары right, включая последнего ребенка
                left->for_internal[left->key_num + 1 + j] = right->for_internal[j];
                left->for_internal[left->key_num + 1 + j].second->parent = left;
            }
            left->key_num += right->key_num + 1;
        }
        noda->for_internal[i].first=noda->for_internal[i+1].first;
        for (size_t j = i+1; j<noda->key_num; ++j){
            noda->for_internal[j] = noda->for_internal[j+1];
        }
        noda->for_internal[noda->key_num]=pair<K, shared_ptr<node>>(K(), shared_ptr<node>());
        --noda->key_num;
    }
    //теперь спокойненько рассматриваем случаи в основной балансировке
    void fix_node(shared_ptr<node> noda, const K& key){
        const size_t min_keys = degree-1;
        while (true){
            if (noda == root){
                if (noda->key_num == 0){
                    root = noda->for_internal[0].second;
                    root->parent.reset();
                }
                return;
            }
            //ищем ноду в родителе
            shared_ptr<node> parent = noda->parent.lock();
            size_t k_pos = 0;
            while (k_pos <= parent->key_num && parent->for_internal[k_pos].second != noda) ++k_pos;
            //TODO как закончу надо отследить попадем ли в корень
            
            //теперь случаи: смотрим есть ли брат и какое у него количество ключей
            //1) у обоих более чем t-1 или у левого более чем минимум
            //2)у правого более чем t-1 а левого нет или у него т-1
            //3)у обоих t-1 или правого нет а у левого т-1
            //4)у  правого т-1 а левого нет
            //разобъем по случаям есть правый есть левый и никого нет
            //ща пытаемся сиблингов найт
            shared_ptr<node> l_sib = k_pos? parent->for_internal[k_pos-1].second: shared_ptr<node>();
            auto r_sib = k_pos==parent->key_num+1? shared_ptr<node>(): parent->for_internal[k_pos-1].second;
            if (l_sib&&l_sib->key_num>min_keys){
                if (noda->is_leaf) take_key_from_left_leaf(noda, k_pos);
                else rotate_right_internal_node(parent, k_pos);
                return;
            }
            if (r_sib && r_sib->key_num > min_keys){
                if (noda->is_leaf) take_key_from_right_leaf(noda, k_pos);
                else rotate_left_internal_node(parent, k_pos);
                return;
            }
            merge_children(parent, k_pos? k_pos-1:k_pos);
            noda=parent;
        }
    }
    template<typename U>
    int bin_search(const array_seq<pair<K,U>>& arr, const K& key, unsigned short key_count) const{
        unsigned short l = 0, r = key_count;
        while (r!=l){
            size_t mid = l+(r-l)/2;
            if (equal(arr[mid].first,key)){return mid;}
            else if (!comp(arr[mid].first,key)){r = mid;}
            else{l = mid+1;}
        }
        if (l < key_count && equal(arr[l].first, key)){
            return l;
        }
        return -1;
    }
public:
    class iterator{
        shared_ptr<node> current;
        size_t pos;
    public:
        explicit iterator(shared_ptr<node> ptr = shared_ptr<node>(), size_t position = 0) noexcept: current(ptr), pos(position){}
        iterator(const iterator& other): current(other.current){}
        ~iterator() = default;
        pair<K, T>& operator*(){return current->for_leaf[pos];}
        pair<K, T>* operator->(){return &current->for_leaf[pos];}
        bool operator==(const iterator& other) const {
        return current == other.current && pos == other.pos;
        }
        bool operator!=(const iterator& other) const {
        return !(*this == other);
        }
        iterator& operator++(){
            if (pos + 1< current->key_num) {
                ++pos;
            } else if(current->right) {//не видит правых соседей??????????
                std::cout<< " | ";
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
        explicit const_iterator(shared_ptr<node> ptr, size_t position = 0) noexcept: current(ptr), pos(position){}
        const_iterator(const iterator& other): current(other.current), pos(other.pos){}
        ~const_iterator() = default;
        const pair<K, T>& operator*(){return current->for_leaf[pos];}
        const pair<K,T>* operator->(){return &current->for_leaf[pos];}
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

    bp_tree():root(){}
    
    const_iterator search(const K& key) const{
        auto leaf = down_to_leaf(key);
        int pos = bin_search(leaf->for_leaf, key, leaf->key_num);
        if (pos != -1){return const_iterator(leaf, pos);}
        throw not_found("no such key in bp_tree");
    }
    bool insert(const K& key, const T& value){//не заменяет существующие значения а возвращаем false
        if (!root){
            root = shared_ptr<node>::make_shared(true, 1);
            root->for_leaf[0] = pair(key, value);
            return true;
        }
        shared_ptr<node> noda = down_to_leaf(key);
        size_t pos = 0;
        while (pos < noda->key_num && !comp(key, noda->for_leaf[pos].first))++pos;
        if (pos > 0 && equal(noda->for_leaf[pos-1].first, key)){return false;}
        for (size_t i = noda->key_num; i >pos ;--i){
            noda->for_leaf[i]= noda->for_leaf[i-1];
        }
        noda->for_leaf[pos] = pair(key, value);
        ++noda->key_num;
        //вот только размер массива для листа меньше 2*deg а значит мы теряем одну пару
        if (noda->key_num == 2*degree) split_node(noda);//на этом моменте всё хорошее во вставке кончается...
        return true;
    }
    bool remove(const K& key){//если удалили то тру если ключа не было то ложь
        if (!root) return false;
        auto leaf = down_to_leaf(key);
        int pos = bin_search(leaf->for_leaf, key, leaf->key_num);
        if (pos == -1){return false;}
        for (size_t i = pos; i< leaf->key_num-1;--i){
            leaf->for_leaf[i] = leaf->for_leaf[i+1];
        }
        --leaf->key_num;
        if (root==leaf){
            if (!root->key_num){root.reset();}
            return true;
        }
        if (leaf->key_num<(degree-1))fix_node(leaf);
        return true;
    }
    iterator end(){return iterator();}
    void print_tree(){
        iterator it = iterator(down_to_leaf(1));
        while(it!=end()){
            std::cout<<(*it).second<<" ";
            it++;
        }
    }
};