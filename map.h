#ifndef MAP_H
#define MAP_H

#include <vector>
#include <utility>
#include <QList>
template<typename K, typename V>
class MyMap {
public:
    MyMap() = default;

    // 插入或更新
    void insert(const K& key, const V& value);

    // 是否包含 key
    bool contains(const K& key) const;

    // 下标访问（和 QMap 一样）
    V& operator[](const K& key);

    // 只读 value（带默认值）
    V value(const K& key, const V& defaultValue = V()) const;

    // 清空
    void clear();

    // 所有 key
    QList<K> keys() const;

private:
    std::vector<std::pair<K, V>> data_;
};

#endif // MAP_H
