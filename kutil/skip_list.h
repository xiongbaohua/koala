#ifndef KUTIL_WORK_SKIP_LIST
#define KUTIL_WORK_SKIP_LIST

#include <cstdlib>
#include <ctime>
#include <vector>
#include <memory>
#include <iostream>

namespace kutil {
// max level of skip list
const int SKIPLIST_MAXLEVEL = 32;
// random number probability
const float SKIPLIST_P = 0.25;

// a skip list 
template<typename _T>
class SkipList {
public:
    class Node {
    public:
        _T value;
        std::vector<std::shared_ptr<Node>> forward;

        Node(_T val, int level) : value(val), forward(level, nullptr) {}
    };
    typedef std::shared_ptr<Node> NodePtr;
    // constructor
    SkipList();
    // deconstructor
    ~SkipList();
    // find node
    NodePtr find(_T value);
    // insert node
    void insert(_T value);
    // remove node
    void remove(_T value);
    // display list 
    void print();
    
private:
    NodePtr _header;
    int _level;
    // generate random level
    int random_level();

};

template<typename _T>
SkipList<_T>::SkipList() {
    _level = 1;
    _header = std::make_shared<Node>(_T(), SKIPLIST_MAXLEVEL);
    std::srand(std::time(nullptr));
}

template<typename _T>
SkipList<_T>::~SkipList() {
}

template<typename _T>
int SkipList<_T>::random_level() {
    int lvl = 1;
    while ((std::rand() / (float)RAND_MAX) < SKIPLIST_P && lvl < SKIPLIST_MAXLEVEL) {
        lvl++;
    }
    return lvl;
}

template<typename _T>
typename SkipList<_T>::NodePtr SkipList<_T>::find(_T value) {
    NodePtr current = _header;
    for (int i = _level - 1; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->value < value) {
            current = current->forward[i];
        }
        current = current->forward[0];
        if (current != nullptr && current->value == value) {
            return current;
        }
    }
    return nullptr;
}

template<typename _T>
void SkipList<_T>::insert(_T value) {
    NodePtr current = _header;
    std::vector<NodePtr> update(SKIPLIST_MAXLEVEL, nullptr);
    // 查找每一层的插入位置
    for (int i = _level - 1; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->value < value) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    current = current->forward[0];
    // 节点不存在则插入
    if (current == nullptr || current->value != value) {
        int new_level = random_level();
        if (new_level > _level) {
            for (int i = _level; i < new_level; i++) {
                update[i] = _header;
            }
            _level = new_level;
        }
        NodePtr new_node = std::make_shared<Node>(value, _level);
        for (int i = 0; i < new_level; i++) {
            new_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = new_node;
        }
    }
}

template<typename _T>
void SkipList<_T>::remove(_T value) {
    NodePtr current = _header;
    std::vector<NodePtr> update(SKIPLIST_MAXLEVEL, nullptr);
    // 查找每一层的删除位置
    for (int i = _level - 1; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->value < value) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    current = current->forward[0];
    // 如果找到目标节点，执行删除
    if (current && current->value == value) {
        for (int i = 0; i < _level; i++) {
            if (update[i]->forward[i] != current) {
                break;
            }
            update[i]->forward[i] = current->forward[i];
        }
        // 如果最高层已经没有节点，减少跳表的层数
        while (_level > 1 && _header->forward[_level - 1] == nullptr) {
            _level--;
        }
    }
}

template<typename _T>
void SkipList<_T>::print() {
    std::cout << "\n***** Skip List *****" << std::endl;
    for (int i = _level - 1; i >= 0; i--) {
        NodePtr current = _header->forward[i];
        std::cout << "Level " << i + 1 << ": ";
        while (current != nullptr) {
            std::cout << current->value << " ";
            current = current->forward[i];
        }
        std::cout << std::endl;
    }
}

} // namespace kutil


#endif