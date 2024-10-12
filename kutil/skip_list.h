#ifndef KUTIL_WORK_SKIP_LIST
#define KUTIL_WORK_SKIP_LIST

#include <cstdlib>
#include <ctime>
#include <vector>
#include <memory>
#include <iostream>
#include <random>

namespace kutil {

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
    SkipList(uint32_t max_level = 32, float p = 0.25f);
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
    // generate random level
    int random_level();

private:
    NodePtr _header;
    int _level;
    const uint32_t kMaxLevel;
    const float kProbability;
};

template<typename _T>
SkipList<_T>::SkipList(uint32_t max_level, float p) :
        kMaxLevel(max_level), kProbability(p) {
    _level = 1;
    _header = std::make_shared<Node>(_T(), kMaxLevel);
}

template<typename _T>
SkipList<_T>::~SkipList() {
}

template<typename _T>
int SkipList<_T>::random_level() {
    thread_local static std::random_device rd; // 随机设备
    thread_local static std::mt19937 gen(rd()); // Mersenne Twister 引擎
    // 定义浮点数范围的均匀分布
    thread_local static std::uniform_real_distribution<float> distrib(0.0, 1.0);
    int lvl = 1;
    while (distrib(gen) < kProbability && lvl < kMaxLevel) {
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
    std::vector<NodePtr> update(kMaxLevel, nullptr);
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
    std::vector<NodePtr> update(kMaxLevel, nullptr);
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