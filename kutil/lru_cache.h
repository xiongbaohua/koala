#ifndef KUTIL_LRU_CACHE_H
#define KUTIL_LRU_CACHE_H

#include <unordered_map>
#include <list>

namespace kutil {
    
template <typename _K, typename _T>
class LRUCache {
public:
    // types
    typedef _K key_type;
    typedef _T mapped_type;
    typedef std::pair<key_type, mapped_type> value_type;

    LRUCache(size_t n);

    LRUCache(const LRUCache& rhs);

    LRUCache& operator=(const LRUCache& rhs);

    ~LRUCache();

    // get value by key, throws a runtime error if key not found
    const mapped_type& get(const key_type& key);

    // put {key, value} into LRUCache
    void put(const key_type& key, const mapped_type& value);

private:
    size_t _capacity;
    std::list<value_type> _cache_list;
    std::unordered_map<key_type, typename std::list<value_type>::iterator> _cache_map;

};

template <typename _K, typename _T>
LRUCache<_K, _T>::LRUCache(size_t n) : _capacity(n) {}

template <typename _K, typename _T>
LRUCache<_K, _T>::LRUCache(const LRUCache& rhs) {
    operator=(rhs);
}

template <typename _K, typename _T>
LRUCache<_K, _T>& LRUCache<_K, _T>::operator=(const LRUCache& rhs) {
    _capacity = rhs._capacity;
    _cache_list = rhs._cache_list;
    for (auto itr = _cache_list.begin(); itr != _cache_list.end(); itr++) {
        _cache_map[itr->first] = itr;
    }
}

template <typename _K, typename _T>
LRUCache<_K, _T>::~LRUCache() {}

template <typename _K, typename _T>
const _T& LRUCache<_K, _T>::get(const _K& key) {
    auto itr = _cache_map.find(key);
    if (itr == _cache_map.end()) {
        throw std::runtime_error("key not found");
    }
    _cache_list.splice(_cache_list.begin(), _cache_list, itr->second);
    return itr->second->second;
}

template <typename _K, typename _T>
void LRUCache<_K, _T>::put(const _K& key, const _T& value) {
    auto itr = _cache_map.find(key);
    if (itr == _cache_map.end()) {
        if (_capacity == _cache_map.size()) {
            _K& to_del = _cache_list.back().first;
            _cache_map.erase(to_del);
            _cache_list.pop_back();
        }
        _cache_list.emplace_front(key, value);
        _cache_map[key] = _cache_list.begin();
    } else {
        _cache_list.splice(_cache_list.begin(), _cache_list, itr->second);
        itr->second->second = value;
    }
}

} // namespace koala::kutil


#endif