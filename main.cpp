#include "bp_tree.hpp"
#include <iostream>

int main(){
    bp_tree<int, int, 3> tree;
    for (int i = 0; i<30; ++i){
        tree.insert(i, i);
    }
    tree.print_tree();
    std::cout<<std::endl;

    for (int i = 0; i<30; i += 2){
        std::cout<<"remove "<<i<<" -> "<<tree.remove(i)<<" | ";
        tree.print_tree();
        std::cout<<std::endl;
    }

    std::cout<<"remove 0 again -> "<<tree.remove(0)<<std::endl;

    for (int i = 1; i<29; i += 2){
        std::cout<<"remove "<<i<<" -> "<<tree.remove(i)<<" | ";
        tree.print_tree();
        std::cout<<std::endl;
    }

    std::cout<<"search 29 -> "<<tree.search(29)->second<<std::endl;
    std::cout<<"remove 29 -> "<<tree.remove(29)<<std::endl;
    std::cout<<"remove 29 again -> "<<tree.remove(29)<<std::endl;

    tree.insert(7, 7);
    tree.insert(3, 3);
    tree.print_tree();
    std::cout<<std::endl;
    return 0;
}