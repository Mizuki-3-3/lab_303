#include "bp_tree.hpp"
#include <iostream>

int main(){
    bp_tree<int, int> tree;
    for (int i = 0; i<1100; ++i){
        if(tree.insert(i, i)){
        std::cout<<"inserted "<<i<<std::endl;}
    }
    if(tree.insert(6, 6)){
        std::cout<<"inserted "<<6<<std::endl;}
    for (int i = 0; i<1100; ++i){
        if (tree.search(i)!=i){std::cout<<"error in search"<<std::endl;}
        else{std::cout<<"found "<<i<<std::endl;}
    }
    return 0;
}