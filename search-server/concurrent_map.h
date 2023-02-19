#pragma once

#include <unordered_map>
#include <mutex>
#include <vector>

template <typename Key, typename Value>
class ConcurrentMap {
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Bucket {
        std::mutex mutex;
        std::unordered_map<Key, Value> map;
    };

    class Access {
        std::lock_guard<std::mutex> guard_;

    public:
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket)
            : guard_(bucket.mutex)
            , ref_to_value(bucket.map[key]) {
        }
    };

public:
    explicit ConcurrentMap(size_t bucket_count)
        : buckets_(bucket_count) {
    }

    Access operator[](const Key& key) {
        auto& bucket = buckets_[key % buckets_.size()];
        return { key, bucket };
    }

    std::unordered_map<Key, Value> BuildOrdinaryMap() {
        std::unordered_map<Key, Value> result;
        for (auto& [mutex, map] : buckets_) {
            std::lock_guard g(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

private:
    std::vector<Bucket> buckets_;
};
