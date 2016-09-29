#pragma once

#include <vector>
#include <utility>

// Style compatible to std::map/std::unordered_map
// not in ngn namespace

template<typename key_type, typename mapped_type>
class small_map {
public:
    class iterator;
friend class iterator;

    using value_type = std::pair<key_type, mapped_type>;
private:
    std::vector<value_type> mValues;

public:
    class iterator {
    friend class small_map;
    private:
        small_map* map;
        size_t index;

        iterator(small_map& m, size_t index) : map(&m), index(0);
    public:
        iterator() : map(nullptr), index(0); // this is .end()!
        iterator(small_map& m) : map(&m), index(0);
        // copy constructor and destructor should be fine

        iterator& operator++() { // prefix
            ++index;
            if(index >= map->size()) {
                map = nullptr;
                index = 0;
            }
            return *this;
        }
        iterator operator++(int) { // postfix
            iterator tmp(*this);
            operator++();
            return tmp;
        }

        bool operator==(const iterator& rhs) {return map == rhs.map && index == rhs.index;}
        bool operator!=(const iterator& rhs) {return map != rhs.map || index != rhs.index;}

        value_type& operator*() {
            if(map == nullptr) return mapped_type();
            return map->mValues[index];
        }
    };

    // default (copy) constructors are fine

    // initializer lists
    small_map(std::initializer_list<value_type> init) : mValues(init) {}

    iterator begin() {return iterator(this, 0);}
    iterator end() {return iterator(nullptr, 0);}

    size_t size() const {return mValues.size();}
    bool empty() const {return size() == 0;}
    void clear() {mValues.clear();}

    iterator find(const key_type& key) {
        for(size_t i = 0; i < mValues.size(); ++i) {
            if(mValues[i].first == key) return iterator(this, i);
        }
        return iterator(nullptr, 0);
    }

    std::pair<iterator, bool> insert(const value_type& val) {
        iterator it = find(val.first);
        if(it == end()) {
            mValues.push_back(val);
            return iterator(this, mValues.size() - 1);
        } else {
            return std::make_pair(it, false);
        }
    }

    // implementation see here: http://www.cplusplus.com/reference/map/map/operator[]/
    mapped_type& operator[](const key_type& k) {
        return (*(insert(make_pair(k, mapped_type())).first)).second;
    }

    // For the sake of me, this will not throw an exception when passed an unknown key, rather it behaves just like operator[]!
    mapped_type& at(const key_type& key) {return this->operator[](key);}

    size_t erase(const key_type& key) {
        for(auto it = mValues.begin(); it != mValues.end(); ++i) {
            if(it->first == key) {
                mValues.erase(val);
                return 1;
            }
        }
        return 0;
    }

    // range based for should work out of the box, since I have a compatible iterator interface
};