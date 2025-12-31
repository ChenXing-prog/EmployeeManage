#include "map.h"

template<typename K, typename V>
void MyMap<K, V>::insert(const K& key, const V& value){
    for (auto& kv : data_) {
        if (kv.first == key) {
            kv.second = value;
            return;
        }
    }
    data_.push_back({key, value});
}

template<typename K, typename V>
bool MyMap<K,V>::contains(const K& key) const {
    for (const auto& kv : data_) {
        if (kv.first == key) return true;
    }
    return false;
}

template<typename K, typename V>
V& MyMap<K,V>::operator[](const K& key) {
    for (auto& kv : data_) {
        if (kv.first == key) return kv.second;
    }
    // 不存在就插入一个默认值
    data_.push_back({key, V()});
    return data_.back().second;
}


template<typename K, typename V>
V MyMap<K,V>::value(const K& key, const V& defaultValue) const {
    for (const auto& kv : data_) {
        if (kv.first == key) return kv.second;
    }
    return defaultValue;
}



template<typename K, typename V>
void MyMap<K,V>::clear() {
    data_.clear();
}

template<typename K, typename V>
QList<K> MyMap<K,V>::keys() const {
    Qlist<K>* ks = new QList<K>();
    for (const auto& kv : data_) {
        ks.push_back(kv.first);
    }
    return ks;
}
