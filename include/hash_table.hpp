#pragma once

#include <array_seq.hpp>
#include <utility>
#include "shared_ptr.hpp"
#include "pair.hpp"

template <typename T, typename K, typename H = std::hash<K>>
class hash_table{
private:
    struct node{
        pair<K, T> kv;
        shared_ptr<node> next;
        unsigned int cache;
        node(pair<K, T> kv_, shared_ptr<node> next_, unsigned int cash_):kv(kv_), next(next_), cache(cash_){}
    };
    static inline const float max_load_factor = 0.75;
    H hash_func;
    array_seq<shared_ptr<node>> table;//будет хранить указатели на ноду идущую перед искомым бакетом
    
    // указатель на ноду, которая указывает на последний
    // часть механизма эффективной итерации по элементам
    shared_ptr<node> prev_head;//по совместительству она идет перед первой настоящей нодой
    size_t node_count=0;


    float load_factor()const{
        return static_cast<float>(node_count) / table.size();
    }
    unsigned int get_table_idx(const K& key) const{
        return hash_func(key) % table.size();
    }
    void rehash(size_t new_size){
        if (new_size <= table.size()) {
            return;
        }
        hash_table new_table(new_size);
        auto current = prev_head->next;
        while (current){
            new_table.insert(current->kv.first, current->kv.second);
            current = current->next;
        }
        (*this) = std::move(new_table);
    }
public:

    class const_iterator;
    class iterator{
        shared_ptr<node> cur = shared_ptr<node>();
    public:
        explicit iterator(const shared_ptr<node>& noda = shared_ptr<node>()):cur(noda){}
        pair<K, T>& operator*() const{return cur->kv;}
        pair<K, T>* operator->()  const{return &cur->kv;}
        iterator& operator++(){
            cur=cur->next;
            return *this;
        }
        iterator operator++(int){
            iterator tmp = *this;
            cur=cur->next;
            return tmp;
        }
        iterator operator+(int right){
            int i = 0;
            while(cur && i<right){
                cur = cur->next;
                i++;
            }
            return *this;
        }
        bool operator!=(const iterator& other) const{return !(*this==other);}
        bool operator==(const iterator& other) const{return cur==other.cur;}
        friend class const_iterator;
        };
    class const_iterator{
        shared_ptr<node> cur = shared_ptr<node>();
    public:
        explicit const_iterator(const shared_ptr<node>& ptr = shared_ptr<node>()) noexcept: cur(ptr) {}
        const_iterator(const const_iterator& other) : cur(other.cur) {}
        const pair<K, T>& operator*() const{return cur->kv;}
        const pair<K, T>* operator->()  const{return &cur->kv;}
        const_iterator& operator++(){
            cur=cur->next;
            return *this;
        }
        const_iterator operator++(int){
            const_iterator tmp = *this;
            cur=cur->next;
            return tmp;
        }
        const_iterator operator+(const int right){
            int i = 0;
            while(cur && i<right){
                cur = cur->next;
                i++;
            }
        }
        bool operator!=(const const_iterator& other) const{return !(*this==other);}
        bool operator==(const const_iterator& other) const{return cur==other.cur;}
    };
    hash_table() : table(16) {}
    hash_table (size_t initial_size):table(initial_size){}

    //copy
    hash_table(const hash_table& other): hash_func(other.hash_func){
        auto current = other.prev_head->next;
        while (current){
            this->insert(current->kv.first, current->kv.second);
        }
    }

    hash_table& operator=(const hash_table& other){
        hash_func = other.hash_func;
        auto current = other.prev_head->next;
        while (current){
            this->insert(current->kv.first, current->kv.second);
        }
        return *this;
    }
    //move
    hash_table(hash_table&& other): node_count(other.node_count){
        std::swap(prev_head, other.prev_head);
        std::swap(table, other.table);
        std::swap(hash_func, other.hash_func);
    }
    hash_table& operator=(hash_table&& other){
        std::swap(node_count, other.node_count);
        std::swap(prev_head, other.prev_head);
        std::swap(table, other.table);
        std::swap(hash_func, other.hash_func);
        return *this;
    }

    ~hash_table(){
        shared_ptr<node> cur = prev_head;
        prev_head.reset();
        while (cur){
            shared_ptr<node> next = cur->next;
            cur->next.reset();
            cur = next;
        }
    }//на этом правило пяти ВСЁ

    bool insert(const K& key, const T& value){
        unsigned int idx = get_table_idx(key);
        if (!prev_head){
            auto tmp = shared_ptr<node>::make_shared(pair<K, T>(key, value), shared_ptr<node>(), idx);
            prev_head = shared_ptr<node>::make_shared(pair<K, T>(), tmp, 0);
            table[idx] = prev_head;
            node_count=1;
            return true;
        }
        if (search(key) != end()){return false;}
        if (!table[idx]){//вааааааааааааааай мама, надо еще проверять что такого ключа нет
            auto tmp = shared_ptr<node>::make_shared(pair<K, T>(key, value), prev_head->next, idx);
            table[prev_head->next->cache] = tmp;
            prev_head->next = tmp;
            table[idx] = prev_head;
        }else{
            shared_ptr<node> cur = table[idx];
            while(cur->next && cur->next->cache==idx){
                cur = cur->next;
            }
            auto tmp = shared_ptr<node>::make_shared(pair<K, T>(key, value), cur->next, idx);
            if (cur->next) table[cur->next->cache] = tmp;
            cur->next = tmp;
        }
        node_count++;
        if (load_factor()>=max_load_factor){rehash(table.size()*2);}
        return true;
    }

    iterator search(const K& key)const{
        if (!prev_head){return iterator();}
        unsigned int id = get_table_idx(key);
        shared_ptr<node> cur = table[id];
        if (!cur){return iterator();}
        cur = cur->next;
        while (cur && cur->kv.first != key && cur->cache==id){
            cur= cur->next;
        }
        if (!cur || cur->kv.first != key){return iterator();}
        return iterator(cur);
    }



    bool remove(const K& key){//возвращаем смогли удалить или нет
        unsigned int id = get_table_idx(key);
        if (!table[id]){return false;}
        shared_ptr<node> prev = table[id], cur = prev->next;
        
        while (cur && cur->kv.first != key && cur->cache==id){
            prev = cur;
            cur= cur->next;
        }
        if (!cur || cur->cache != id){return false;}//не можем удалить если не найдено, а если не найдено то  мы перешли в некст бакет или перешагнули конец
        shared_ptr<node> next = cur->next;
        bool single = (prev == table[id]) && (!next || next->cache != id);  // cur единственный в корзине

        if (next && next->cache != id) table[next->cache] = prev;  // у следующей корзины новый предшественник
        if (single) table[id].reset();
        prev->next = next;

        --node_count;
        if (!prev_head->next){prev_head.reset();}
        return true;
    }  

    iterator begin(){return iterator(prev_head?prev_head->next:shared_ptr<node>());}
    const_iterator begin()const{return const_iterator(prev_head?prev_head->next:shared_ptr<node>());}

    iterator end(){return iterator();}
    const_iterator end()const{return const_iterator();}
    
};