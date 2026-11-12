#include "bp_tree.hpp"
#include <iostream>
#include "hash_table.hpp"


int main(){

    hash_table <int, int> table;
    for (int i = 1; i<100; i++){
        table.insert(i,i);
        if (table.search(i)->second)std::cout << i<< "was inserted \n";
        
    }

    return 0;
}