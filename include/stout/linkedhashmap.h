// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <list>
#include <utility>

#include "stout/foreach.h"
#include "stout/hashmap.h"
#include "stout/option.h"

////////////////////////////////////////////////////////////////////////

// Implementation of a hashmap that maintains the insertion order of
// the keys. Updating a key does not change insertion order.
//
// TODO(vinod/bmahler): Consider extending from stout::hashmap and/or
// having a compatible API with stout::hashmap.
template <typename Key, typename Value>
class LinkedHashMap {
 public:
  typedef std::pair<Key, Value> entry;
  typedef std::list<entry> list;
  typedef hashmap<Key, typename list::iterator> map;

  LinkedHashMap() = default;

  LinkedHashMap(const LinkedHashMap<Key, Value>& other)
    : entries_(other.entries_) {
    // Build up the index.
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
      keys_[it->first] = it;
    }
  }

  LinkedHashMap& operator=(const LinkedHashMap<Key, Value>& other) {
    clear();

    entries_ = other.entries_;

    // Build up the index.
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
      keys_[it->first] = it;
    }

    return *this;
  }

  // TODO(bmahler): Implement move construction / assignment.
  LinkedHashMap(LinkedHashMap<Key, Value>&&) = delete;
  LinkedHashMap& operator=(LinkedHashMap&&) = delete;

  Value& operator[](const Key& key) {
    if (!keys_.contains(key)) {
      // Insert a new entry into the list and get a "pointer" to its
      // location. The initial value is default-constructed.
      typename list::iterator iter =
          entries_.insert(entries_.end(), std::make_pair(key, Value()));

      keys_[key] = iter;
    }

    return keys_[key]->second;
  }

  Option<Value> get(const Key& key) const {
    if (keys_.contains(key)) {
      return keys_.at(key)->second;
    }
    return None();
  }

  Value& at(const Key& key) {
    return keys_.at(key)->second;
  }

  const Value& at(const Key& key) const {
    return keys_.at(key)->second;
  }

  bool contains(const Key& key) const {
    return keys_.contains(key);
  }

  size_t erase(const Key& key) {
    if (keys_.contains(key)) {
      typename list::iterator iter = keys_[key];
      keys_.erase(key);
      entries_.erase(iter);
      return 1;
    }
    return 0;
  }

  // Returns the keys in the map in insertion order.
  std::vector<Key> keys() const {
    std::vector<Key> result;
    result.reserve(entries_.size());

    foreach (const entry& entry, entries_) {
      result.push_back(entry.first);
    }

    return result;
  }

  // Returns the values in the map in insertion order.
  std::vector<Value> values() const {
    std::vector<Value> result;
    result.reserve(entries_.size());

    foreach (const entry& entry, entries_) {
      result.push_back(entry.second);
    }

    return result;
  }

  size_t size() const {
    return keys_.size();
  }

  bool empty() const {
    return keys_.empty();
  }

  void clear() {
    entries_.clear();
    keys_.clear();
  }

  // Support for iteration; this allows using `foreachpair` and
  // related constructs. Note that these iterate over the map in
  // insertion order.
  typename list::iterator begin() {
    return entries_.begin();
  }
  typename list::iterator end() {
    return entries_.end();
  }

  typename list::const_iterator begin() const {
    return entries_.cbegin();
  }
  typename list::const_iterator end() const {
    return entries_.cend();
  }

 private:
  list entries_; // Key-value pairs ordered by insertion order.
  map keys_; // Map from key to "pointer" to key's location in list.
};

////////////////////////////////////////////////////////////////////////
