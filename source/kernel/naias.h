/*
 *
 * Copyright (c) 2015 Raphine Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: Liva
 * 
 */

#ifndef __RAPH_KERNEL_RAPH_H__
#define __RAPH_KERNEL_RAPH_H__

#include "global.h"
#include <assert.h>

void kernel_panic(char *class_name, char *err_str);

template<class T>
static inline T align(T val, int base) {
  return (val / base) * base;
}

template<class T>
static inline T alignUp(T val, int base) {
  return align(val + base - 1, base);
}

inline void *operator new(size_t, void *p)     throw() { return p; }
inline void *operator new[](size_t, void *p)   throw() { return p; }
inline void  operator delete  (void *, void *) throw() { };
inline void  operator delete[](void *, void *) throw() { };

#ifdef __UNIT_TEST__
#define UTEST_VIRTUAL virtual
#define kassert(flag) if (!(flag)) {throw #flag;}

#include <map>
template <class K, class V>
class StdMap {
public:
  StdMap() : _flag(0) {}
  void Set(K key, V value) {
    while(!__sync_bool_compare_and_swap(&_flag, 0, 1)) {}
    _map[key] = value;
    __sync_bool_compare_and_swap(&_flag, 1, 0);
  }
  // keyが存在しない場合は、initを返す
  V Get (K key, V init) {
    V value = init;
    while(!__sync_bool_compare_and_swap(&_flag, 0, 1)) {}
    if (_map.find(key) == _map.end()) {
      _map.insert(make_pair(key, init));
      value = init;
    } else {
      value = _map.at(key);
    }
    __sync_bool_compare_and_swap(&_flag, 1, 0);
    return value;
  }
  bool Exist(K key) {
    bool flag = false;
    while(!__sync_bool_compare_and_swap(&_flag, 0, 1)) {}
    if (_map.find(key) != _map.end()) {
      flag = true;
    }
    __sync_bool_compare_and_swap(&_flag, 1, 0);
    return flag;
  }
  // mapへの直接アクセス
  // 並列実行しないように
  std::map<K, V> &Map() {
    return _map;
  }
private:
  std::map<K, V> _map;
  volatile int _flag;
};
#else
#define UTEST_VIRTUAL
#define kassert assert
#endif // __UNIT_TEST__

#endif // __RAPH_KERNEL_RAPH_H__
