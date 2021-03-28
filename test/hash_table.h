// =============================================================================
// Created by yangb on 2021/3/28.
// =============================================================================

#ifndef WEBSERVERCPP11_TEST_HASH_TABLE_H_
#define WEBSERVERCPP11_TEST_HASH_TABLE_H_

#include <string>
#include <vector>

#if 0
static size_t BKDRHash(const char *str) { // BKDRHash算法获取哈希值
  uint32_t seed = 131;
  uint32_t hash = 0;
  while (*str) {
    hash = hash * seed + (*str++);
  }
  return (hash & 0x7FFFFFFF);
}

enum State { EMPTY, DELETE, EXIST };

template <typename K>
struct _HashFunc_ {
  size_t operator[](const K &key) {
    return key;
  }
};

// 模板特化
template <>
struct _HashFunc_<std::string> {  // key为字符串时候的哈希函数
  static size_t BKDRHash(const char *str) { // BKDRHash算法获取哈希值
    uint32_t seed = 131;
    uint32_t hash = 0;
    while (*str) {
      hash = hash * seed + (*str++);
    }
    return (hash & 0x7FFFFFFF);
  }
};

template <typename K, typename V>
struct HashNode {
  HashNode() : m_s(EMPTY) {}

 public:
  std::pair<K, V> m_kv;
  State m_s;
};

template <typename K, typename V, typename HashFunc = _HashFunc_<K>, bool IsLine = true>
class HashTable {
  using Node = HashNode<K, V>;
  using Self = HashTable<K, V, HashFunc, IsLine>;

 public:
  HashTable(size_t size = 10) : m_size_(0) {
    m_table_.resize(GetNextPrime_(size));
  }

  bool Insert(const K &key, const V &value) { // 插入
    CheckSize_(); // 检测容量， 装填因子是否>0.6
    size_t hash_addr = HashFunc_(key);
    size_t index = hash_addr;
    while (m_table_[index].m_s == EXIST) {
      if (m_table_[index].m_kv.first == key) { // 要插入的节点已经存在
        return false;
      }

      if (IsLine) {  // 线性探测
        index = DetectedLine_(hash_addr);
      } else {  // 二次探测
        size_t i = 1;
        index = DetectedSquare_(hash_addr, i);
      }

      if (index == m_table_.size()) {  //如果探测到最后，就返回第一个结点继续探测
        index = 0;
      }
    } // while

    m_table_[index].m_kv.first = key;
    m_table_[index].m_kv.second = value;
    m_table_[index].m_s = EXIST;
    ++m_size_;
    return true;
  } // Insert

  std::pair<Node *, bool> Find(const K &key) const {
    size_t index = HashFunc_(key);
    size_t hash_addr = index;
    Node& elem = m_table_[index];

    if(elem.m_kv.first != key) {
      ++index;
      if(index == m_table_.size()) {
        index = 0;
      }
      if(index == hash_addr) {
        return std::make_pair(&elem, false);
      }
    }

    if(m_table_[index].m_s == EXIST) {
      return std::make_pair(&elem, true);
    } else {
      return std::make_pair(&elem, false);
    }
  }

  bool Remove(const K& key) {
    auto ret = this->Find(key);
    if(ret.second == true) {
      ret.first->m_s = DELETE;
      --m_size_;
      return true;
    }
    return false;
  }

  size_t Size() const {
    return m_size_;
  }

 private:
  size_t DetectedLine_(size_t hash_addr) { // 线性探测
    return (hash_addr + 1) % this->m_table_.size();
  }

  size_t DetectedSquare_(size_t hash_addr, size_t i) { // 二次探测
    return (hash_addr + (i << 2) + 1) % this->m_table_.size();
  }

  size_t GetNextPrime_(size_t size) { // 使用素数表对齐做哈希表的容量，降低哈希冲突
    const int kPrimeSize = 28;
    static const uint64_t kPrimeList[kPrimeSize] = {
        52ul, 87ul, 193ul, 389ul, 769ul,
        1543ul, 3079ul, 6151ul, 12289ul, 24593ul,
        49157ul, 98317ul, 196613ul, 393241ul, 786433ul,
        1572869ul, 3145739ul, 6291469ul, 12582917ul, 24165843ul,
        50331653ul, 100663319ul, 201326611ul, 402653189ul, 805306457ul,
        1610612741ul, 3221225473ul, 4294967291ul
    };

    for (int i = 0; i < kPrimeSize; ++i) {
      if (kPrimeList[i] > size) {
        return kPrimeList[i];
      }
    }

    return kPrimeList[kPrimeSize - 1];
  }

  void CheckSize_() {
    if (m_size_ * 10 / m_table_.size() >= 6) { // 装填因子 >= 0.6
      m_table_.resize(GetNextPrime_(m_size_));

      HashTable<K, V> ht;
      for (size_t idx = 0; idx < m_table_.size(); ++idx) {
        if (m_table_[idx].m_s == EXIST) {
          ht.Insert(m_table_[idx].m_kv.first, m_table_[idx].m_kv.second);
        } // if
      } // for
      this->Swap_(ht);
    } // if
  } // CheckSize_

  void Swap_(HashTable<K, V> &ht) {
    std::swap(m_size_, ht.m_size_);
    m_table_.swap(ht.m_table_);
  }

  size_t HashFunc_(const K &key) {
    return key % m_table_.size();
  }

 protected:
  std::vector<Node> m_table_;
  size_t m_size_; // 当前哈希表中的元素个数
};

#endif
#endif //WEBSERVERCPP11_TEST_HASH_TABLE_H_
