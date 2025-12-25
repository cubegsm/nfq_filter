#ifndef _LRU_CHACHE_H_
#define _LRU_CHACHE_H_

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <map>
#include <functional>
#include <iostream>
#include <mutex>

namespace Cache {

template<class key_t, class value_t> class lru_cache
{
public:
    struct kv_pair_t
    {
        key_t key;
        value_t value;
        time_t time;
        bool operator==(const kv_pair_t &rhs) { return (key == rhs.key && value == rhs.value); }
    };

    typedef typename std::list<kv_pair_t>::iterator iter_t;
    typedef typename std::map<key_t, iter_t>::iterator map_iter_t;

    explicit lru_cache(size_t size, time_t lifetime) : list_size_(0), max_size_(size), lifetime_(lifetime), func_set_(false) { }

    void Reinit(size_t size, time_t lifetime)
    {
        list_size_ = 0;
        max_size_ = size;
        lifetime_ = lifetime;
        func_set_ = false;
    }

    void setFunc(std::function<void(value_t)> fun)
    {
        func_ = fun;
        func_set_ = true;
    }

    size_t getMaxSize() { return max_size_; }

    value_t pop()
    {
        value_t ret;

        if (cache_items_map_.size() > 0)
        {
            auto last = cache_items_list_.end();
            last--;

            if (func_set_)
                func_(last->value);

            ret = last->value;
            cache_items_map_.erase(last->key);
            cache_items_list_.pop_back();
            --list_size_;
        }

        return ret;
    }

    void put(const key_t &key, const value_t &value)
    {
        auto it = cache_items_map_.find(key);
        if (it != cache_items_map_.end())
        {
            cache_items_list_.erase(it->second);
            --list_size_;
            cache_items_map_.erase(it);
        }

        auto current_time = time(NULL);
        cache_items_list_.push_front({key, value, current_time});
        ++list_size_;
        cache_items_map_[key] = cache_items_list_.begin();

        if (cache_items_map_.size() > max_size_)
        {
            auto last = cache_items_list_.end();
            last--;

            if (func_set_)
                func_(last->value);

            cache_items_map_.erase(last->key);
            cache_items_list_.pop_back();
            --list_size_;
        }

        while (list_size_ > 0)
        {
            auto last = cache_items_list_.end();
            last--;

            if (current_time - last->time > lifetime_)
            {
                if (func_set_)
                    func_(last->value);

                cache_items_map_.erase(last->key);
                cache_items_list_.pop_back();
                --list_size_;
            }
            else
            {
                break;
            }
        }
    }

    void put_mt(const key_t &key, const value_t &value)
    {
        std::lock_guard<std::mutex> lock(gmutex);
        put(key, value);
    }

    value_t* get(const key_t& key)
    {
        auto it = cache_items_map_.find(key);

        if(it == cache_items_map_.end())
        {
            return nullptr;
        }
        else
        {
            it->second->time = time(NULL);
            cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, it->second);
            //return &it->second->second;
            return &it->second->value;
        }
    }

    typename std::map<key_t, iter_t>::iterator findAll(const key_t &key)
    {
        auto it = cache_items_map_.find(key);

        if (it == cache_items_map_.end())
        {
            return it;
        }
        else
        {
            cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, it->second);
            return it;
        }
    }

    typename std::map<key_t, iter_t>::iterator find(const key_t &key)
    {
        auto it = cache_items_map_.find(key);

        if (it == cache_items_map_.end())
            return it;
        else
        {
            cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, it->second);
            return it;
        }
    }

    typename std::map<key_t, iter_t>::iterator find_mt(const key_t &key)
    {
        std::lock_guard<std::mutex> lock(gmutex);
        return find(key);
    }

    void clean_outdated()
    {
        auto current_time = time(NULL);
        while (list_size_ > 0)
        {
            auto last = cache_items_list_.end();
            last--;

            if (current_time - last->time >= lifetime_)
            {
                if (func_set_)
                    func_(last->value);

                cache_items_map_.erase(last->key);
                cache_items_list_.pop_back();
                --list_size_;
            }
            else
            {
                break;
            }
        }
    }

    void set_lifetime(time_t val) { lifetime_  = val; }
    time_t get_lifetime() const { return lifetime_; }

    void remove(const key_t &key)
    {
        auto it = cache_items_map_.find(key);
        if (it != cache_items_map_.end())
        {
            cache_items_list_.erase(it->second);
            --list_size_;
            cache_items_map_.erase(it);
        }
    }

    bool exists(const key_t &key) const { return cache_items_map_.find(key) != cache_items_map_.end(); }
    size_t size() const { return cache_items_map_.size(); }
    void clear()
    {
        cache_items_list_.clear();
        list_size_ = 0;
        cache_items_map_.clear();
    }

    bool empty() { return cache_items_map_.empty(); }
    typename std::map<key_t, iter_t>::iterator begin() { return cache_items_map_.begin(); }
    typename std::map<key_t, iter_t>::iterator end() { return cache_items_map_.end(); }
    size_t getListSize() const { return list_size_; }
    void setMtEnabled() { mt_enabled = true; }

    std::list<kv_pair_t> cache_items_list_;
    std::map<key_t, iter_t> cache_items_map_;
    size_t list_size_;

public:
    std::mutex gmutex;

private:
    bool mt_enabled;
    size_t max_size_;
    time_t lifetime_;
    std::function<void(value_t)> func_;
    bool func_set_;
};
}

#endif /* _LRU_CHACHE_H_ */

