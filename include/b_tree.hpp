#pragma once
#include "array_seq.hpp"
#include <vector>
#include "errors.hpp"
#include <iostream>

template <typename K>
class b_tree {
private:
    struct Node {
        bool leaf;
        std::vector<K> keys;
        std::vector<Node*> children;

        explicit Node(bool is_leaf) : leaf(is_leaf) {}

        ~Node() {
            for (Node* child : children) {
                delete child;
            }
        }
    };

    Node* root_ = nullptr;
    int t_;

    void split_child(Node* x, int i) {
        Node* y = x->children[i];
        Node* z = new Node(y->leaf);

        z->keys.assign(y->keys.begin() + t_, y->keys.end());
        K midKey = y->keys[t_ - 1];
        y->keys.resize(t_ - 1);

        if (!y->leaf) {
            z->children.assign(y->children.begin() + t_, y->children.end());
            y->children.resize(t_);
        }

        x->children.insert(x->children.begin() + i + 1, z);
        x->keys.insert(x->keys.begin() + i, midKey);
    }

    void insertNonFull(Node* x, const K& k) {
        int i = static_cast<int>(x->keys.size()) - 1;

        if (x->leaf) {
            x->keys.push_back(k);
            while (i >= 0 && k < x->keys[i]) {
                x->keys[i + 1] = x->keys[i];
                --i;
            }
            x->keys[i + 1] = k;
        } else {
            while (i >= 0 && k < x->keys[i]) {
                --i;
            }
            ++i;
            if (static_cast<int>(x->children[i]->keys.size()) == 2 * t_ - 1) {
                split_child(x, i);
                if (k > x->keys[i]) {
                    ++i;
                }
            }
            insertNonFull(x->children[i], k); //рекурсией сделала
        }
    }


    Node* search(Node* x, const K& k, int& outIndex) const {
        if (!x) return nullptr;
        int i = 0;
        while (i < static_cast<int>(x->keys.size()) && k > x->keys[i]) {
            ++i;
        }
        if (i < static_cast<int>(x->keys.size()) && k == x->keys[i]) {
            outIndex = i;
            return x;
        }
        if (x->leaf) {
            return nullptr;
        }
        return search(x->children[i], k, outIndex);
    }

    int findKeyIndex(Node* x, const K& k) const {
        int idx = 0;
        while (idx < static_cast<int>(x->keys.size()) && x->keys[idx] < k) {
            ++idx;
        }
        return idx;
    }

    K getPredecessor(Node* x, int idx) {
        Node* cur = x->children[idx];
        while (!cur->leaf) {
            cur = cur->children.back();
        }
        return cur->keys.back();
    }

    K getSuccessor(Node* x, int idx) {
        Node* cur = x->children[idx + 1];
        while (!cur->leaf) {
            cur = cur->children.front();
        }
        return cur->keys.front();
    }

    void borrowFromPrev(Node* x, int idx) {
        Node* child = x->children[idx];
        Node* sibling = x->children[idx - 1];

        child->keys.insert(child->keys.begin(), x->keys[idx - 1]);
        if (!child->leaf) {
            child->children.insert(child->children.begin(), sibling->children.back());
            sibling->children.pop_back();
        }
        x->keys[idx - 1] = sibling->keys.back();
        sibling->keys.pop_back();
    }

    void borrowFromNext(Node* x, int idx) {
        Node* child = x->children[idx];
        Node* sibling = x->children[idx + 1];

        child->keys.push_back(x->keys[idx]);
        if (!child->leaf) {
            child->children.push_back(sibling->children.front());
            sibling->children.erase(sibling->children.begin());
        }
        x->keys[idx] = sibling->keys.front();
        sibling->keys.erase(sibling->keys.begin());
    }

    void merge(Node* x, int idx) {
        Node* child = x->children[idx];
        Node* sibling = x->children[idx + 1];

        child->keys.push_back(x->keys[idx]);
        child->keys.insert(child->keys.end(), sibling->keys.begin(), sibling->keys.end());
        if (!child->leaf) {
            child->children.insert(child->children.end(),
                                    sibling->children.begin(), sibling->children.end());
        }

        x->keys.erase(x->keys.begin() + idx);
        x->children.erase(x->children.begin() + idx + 1);

        sibling->children.clear();
        delete sibling;
    }

    void fill(Node* x, int idx) {
        if (idx != 0 && static_cast<int>(x->children[idx - 1]->keys.size()) >= t_) {
            borrowFromPrev(x, idx);
        } else if (idx != static_cast<int>(x->keys.size()) &&
                   static_cast<int>(x->children[idx + 1]->keys.size()) >= t_) {
            borrowFromNext(x, idx);
        } else {
            if (idx != static_cast<int>(x->keys.size())) {
                merge(x, idx);
            } else {
                merge(x, idx - 1);
            }
        }
    }

    void removeFromLeaf(Node* x, int idx) {
        x->keys.erase(x->keys.begin() + idx);
    }

    void removeFromNonLeaf(Node* x, int idx) {
        K k = x->keys[idx];

        if (static_cast<int>(x->children[idx]->keys.size()) >= t_) {
            K pred = getPredecessor(x, idx);
            x->keys[idx] = pred;
            removeImpl(x->children[idx], pred);
        } else if (static_cast<int>(x->children[idx + 1]->keys.size()) >= t_) {
            K succ = getSuccessor(x, idx);
            x->keys[idx] = succ;
            removeImpl(x->children[idx + 1], succ);
        } else {
            merge(x, idx);
            removeImpl(x->children[idx], k);
        }
    }

    void removeImpl(Node* x, const K& k) {
        int idx = findKeyIndex(x, k);

        if (idx < static_cast<int>(x->keys.size()) && x->keys[idx] == k) {
            if (x->leaf) {
                removeFromLeaf(x, idx);
            } else {
                removeFromNonLeaf(x, idx);
            }
        } else {
            if (x->leaf) {
                return; // ключа нет в дереве
            }

            bool lastChild = (idx == static_cast<int>(x->keys.size()));

            if (static_cast<int>(x->children[idx]->keys.size()) < t_) {
                fill(x, idx);
            }

            if (lastChild && idx > static_cast<int>(x->keys.size())) {
                removeImpl(x->children[idx - 1], k);
            } else {
                removeImpl(x->children[idx], k);
            }
        }
    }

    void traverse(Node* x, int depth = 0) const {
        if (!x) return;
        std::cout << std::string(depth * 2, ' ') << "[";
        for (size_t i = 0; i < x->keys.size(); ++i) {
            std::cout << x->keys[i] << (i + 1 < x->keys.size() ? " " : "");
        }
        std::cout << "]\n";
        for (Node* child : x->children) {
            traverse(child, depth + 1);
        }
    }

public:
    explicit b_tree(int minimumDegree) : t_(minimumDegree) {
        if (t_ < 2) {
            throw invalid_argument("Минимальная степень t должна быть >= 2");
        }
    }

    ~b_tree() {
        delete root_;
    }

    b_tree* insert(const K& k) {
        if (!root_) {
            root_ = new Node(true);
            root_->keys.push_back(k);
            return this;
        }

        if (static_cast<int>(root_->keys.size()) == 2 * t_ - 1) {
            Node* newRoot = new Node(false);
            newRoot->children.push_back(root_);
            root_ = newRoot;
            split_child(root_, 0);
            insertNonFull(root_, k);
        } else {
            insertNonFull(root_, k);
        }
        return this;
    }

    bool search(const K& k) const {
        int idx = -1;
        return search(root_, k, idx) != nullptr;
    }
//что такое std::function

    void remove(const K& k) {
        if (!root_) return;

        removeImpl(root_, k);

        if (root_->keys.empty() && !root_->leaf) {
            Node* oldRoot = root_;
            root_ = root_->children[0];
            oldRoot->children.clear();
            delete oldRoot;
        } else if (root_->keys.empty() && root_->leaf) {
            delete root_;
            root_ = nullptr;
        }
    }
};