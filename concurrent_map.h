#include <mutex>
#include <future>
#include <map>
#include <vector>
#include <cstddef>

template<typename Key, typename Value>
class ConcurrentMap {
	public:
		static_assert(std::is_integral<Key>::value, "Integral required.");

		struct Bucket {
			std::mutex mutex_;
			std::map<Key, Value> map_;
		};

		struct Access {
			Access(const Key& key, Bucket& bucket)
				: lock_guard(bucket.mutex_)
				, ref_to_value(bucket.map_[key])
			{
			}
			std::lock_guard<std::mutex> lock_guard;
			Value& ref_to_value;
		};

		explicit ConcurrentMap(size_t bucket_count)
			: buckets_(bucket_count)
		{
		}

		Access operator[](const Key& key) {
			return Access{key, buckets_[static_cast<uint64_t>(key) % buckets_.size()]};
		};

		std::map<Key, Value> BuildOrdinaryMap() {
			std::map<Key, Value> ordinary_map;
			for (auto& [mutex_, map_] : buckets_) {
				{
					std::lock_guard<std::mutex> lock_(mutex_);
					ordinary_map.insert(map_.begin(),map_.end());
				}
			}
			return ordinary_map;
		}

	    void erase(const Key& key) {
	    	const size_t i = static_cast<size_t>(key) % buckets_.size();

	    	std::lock_guard<std::mutex> lock_guard(buckets_[i].mutex_);
	    	buckets_[i].map_.erase(key);
	    }

	private:
		std::vector<Bucket> buckets_;
};

