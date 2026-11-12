#pragma once

#include <array_seq.hpp>
#include <utility>
#include "shared_ptr.hpp"


template <typename T, typename K, typename HASH = std::hash<K>>
class hash_table{
private:
    struct node{
        std::pair<K, T> kv;
        shared_ptr<node> next;
        unsigned int cash;
        node(std::pair<K, T> kv_, shared_ptr<node> next_, unsigned int cash_):kv(kv_), next(next_), cash(cash_){}
    };
    static constexpr float max_load_factor = 0.75;
    HASH hash_func;
    array_seq<shared_ptr<node>> table;//будет хранить указатели на ноду идущую перед искомым бакетом
    shared_ptr<node> fake_node;//по совместительству она идет перед первой настоящей нодой
    size_t node_count=0;


    float load_factor()const{
        return static_cast<float>(node_count) / table.size();
    }
    unsigned int hash(const K& key) const{
        return hash_func(key) % table.size();
    }
    void rehash(size_t new_size){
        if (new_size <= table.size()) {
            return;
        }
        hash_table new_table(new_size);
        auto current = fake_node->next;
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
        std::pair<K, T>& operator*() const{return cur->kv;}
        std::pair<K, T>* operator->()  const{return &cur->kv;}
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
        const std::pair<K, T>& operator*() const{return cur->kv;}
        const std::pair<K, T>* operator->()  const{return &cur->kv;}
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
        auto current = other.fake_node->next;
        while (current){
            this->insert(current->kv.first, current->kv.second);
        }
    }

    hash_table& operator=(const hash_table& other){
        hash_func = other.hash_func;
        auto current = other.fake_node->next;
        while (current){
            this->insert(current->kv.first, current->kv.second);
        }
        return *this;
    }
    //move
    hash_table(hash_table&& other): node_count(other.node_count){
        std::swap(fake_node, other.fake_node);
        std::swap(table, other.table);
        std::swap(hash_func, other.hash_func);
    }
    hash_table& operator=(hash_table&& other){
        std::swap(node_count, other.node_count);
        std::swap(fake_node, other.fake_node);
        std::swap(table, other.table);
        std::swap(hash_func, other.hash_func);
        return *this;
    }

    ~hash_table() = default;//на этом правило пяти ВСЁ

    bool insert(const K& key, const T& value){
        unsigned int id = hash(key);
        if (!fake_node){
            auto tmp = shared_ptr<node>::make_shared(std::pair<K, T>(key, value), shared_ptr<node>(), id);
            fake_node = shared_ptr<node>::make_shared(std::pair<K, T>(), tmp, 0);
            table[id] = fake_node;
            node_count=1;
            return true;
        }
        if (search(key) != end()){return false;}
        if (!table[id]){//вааааааааааааааай мама, надо еще проверять что такого ключа нет
            auto tmp = shared_ptr<node>::make_shared(std::pair<K, T>(key, value), fake_node->next, id);
            table[fake_node->next->cash] = tmp;
            fake_node->next = tmp;
            table[id] = fake_node;
        }else{
            shared_ptr<node> cur = table[id];
            while(cur->next && cur->next->cash==id){
                cur = cur->next;
            }
            auto tmp = shared_ptr<node>::make_shared(std::pair<K, T>(key, value), cur->next, id);
            if (cur->next) table[cur->next->cash] = tmp;
            cur->next = tmp;
        }
        if (load_factor()>=max_load_factor){rehash(table.size()*2);}
        node_count++;
        return true;
    }

    iterator search(const K& key)const{
        if (!fake_node){return iterator();}
        unsigned int id = hash(key);
        shared_ptr<node> cur = table[id];
        if (!cur){return iterator();}
        cur = cur->next;
        while (cur && cur->kv.first != key && cur->cash==id){
            cur= cur->next;
        }
        if (!cur || cur->kv.first != key){return iterator();}
        return iterator(cur);
    }



    bool remove(const K& key){//возвращаем смогли удалить или нет
        unsigned int id = hash(key);
        if (!table[id]){return false;}
        shared_ptr<node> prev = table[id], cur = prev->next;
        
        while (cur && cur->kv.first != key && cur->cash==id){
            prev = cur;
            cur= cur->next;
        }
        if (!cur || cur&&cur->kv.first!=key){return false;}//не можем удалить если не найдено, а если не найдено то  мы перешли в некст бакет или перешагнули конец
        if (prev == table[id]){
            if (!cur->next || cur->cash!=cur->next->cash){//вроде это все условия для одноэлем бакета
                table[id].reset();
            }
        }
        prev->next = cur->next;
        node_count--;
        return true;
    }  

    iterator begin(){return iterator(fake_node?fake_node->next:shared_ptr<node>());}
    const_iterator begin()const{return const_iterator(fake_node?fake_node->next:shared_ptr<node>());}

    iterator end(){return iterator();}
    const_iterator end()const{return const_iterator();}
    
};