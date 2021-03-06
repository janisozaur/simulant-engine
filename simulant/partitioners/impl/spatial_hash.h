#pragma once

#include <cstring>
#include <map>
#include <unordered_set>
#include <ostream>
#include <unordered_set>
#include "../../interfaces.h"

/*
 * Hierarchical Grid Spatial Hash implementation
 *
 * Objects are inserted into (preferably) one bucket
 */

namespace smlt {


const uint32_t MAX_GRID_LEVELS = 16;
struct Hash {
    Hash() = default;
    Hash(int16_t x, int16_t y, int16_t z):
        x(x), y(y), z(z) {}

    int16_t x = 0;
    int16_t y = 0;
    int16_t z = 0;
};

/*
 * Heirarchical hash key. Each level in the key is a hash of cell_size, x, y, z
 * if a child key is visible then all parent and child keys are visible. Using a multimap
 * we can rapidly gather child key objects (by iterating until the key no longer starts with this one)
 * and we can gather parent ones by checking all levels of the hash_path
 */
struct Key {
    Hash hash_path[MAX_GRID_LEVELS];
    std::size_t ancestors = 0;
    std::size_t hash_code = 0;

    bool operator<(const Key& other) const {
        auto len = std::min(other.ancestors, ancestors) + 1;
        auto ret = memcmp(hash_path, other.hash_path, sizeof(Hash) * len);
        return ret < 0 || (ret == 0 && ancestors < other.ancestors);
    }

    bool operator==(const Key& other) const {
        return (
            ancestors == other.ancestors &&
            memcmp(hash_path, other.hash_path, sizeof(Hash) * (ancestors + 1)) == 0
        );
    }

    bool is_root() const { return ancestors == 0; }
    Key parent_key() const;

    bool is_ancestor_of(const Key& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Key& key);
};

}

namespace std {

    template<>
    struct hash<smlt::Key> {
        typedef smlt::Key argument_type;
        typedef std::size_t result_type;

        result_type operator()(argument_type const& s) const
        {
            return s.hash_code;
        }
    };

}

namespace smlt {

std::ostream& operator<<(std::ostream& os, const Key& key);

Key make_key(int32_t cell_size, float x, float y, float z);
Hash make_hash(int32_t cell_size, float x, float y, float z);

typedef std::unordered_set<Key> KeyList;

class SpatialHashEntry {
public:
    virtual ~SpatialHashEntry() {}

    void set_hash_aabb(const AABB& aabb) {
        hash_aabb_ = aabb;
    }

    void push_key(const Key& key) {
        keys_.insert(key);
    }

    void remove_key(const Key& key) {
        keys_.erase(key);
    }

    void set_keys(const KeyList& keys) {
        keys_ = std::move(keys);
    }

    const KeyList& keys() const {
        return keys_;
    }

    const AABB& hash_aabb() const { return hash_aabb_; }
private:
    KeyList keys_;
    AABB hash_aabb_;
};

typedef std::unordered_set<SpatialHashEntry*> HGSHEntryList;

class SpatialHash {
public:
    SpatialHash();

    void insert_object_for_box(const AABB& box, SpatialHashEntry* object);
    void remove_object(SpatialHashEntry* object);

    void update_object_for_box(const AABB& new_box, SpatialHashEntry* object);

    HGSHEntryList find_objects_within_box(const AABB& box);
    HGSHEntryList find_objects_within_frustum(const Frustum& frustum);

    friend std::ostream &operator<<(std::ostream &os, const SpatialHash &hash);

private:
    void erase_object_from_key(Key key, SpatialHashEntry* object);

    int32_t find_cell_size_for_box(const AABB& box) const;
    void insert_object_for_key(Key key, SpatialHashEntry* entry);

    typedef std::map<Key, std::unordered_set<SpatialHashEntry*>> Index;
    Index index_;
};

std::ostream &operator<<(std::ostream &os, const SpatialHash &hash);

void generate_boxes_for_frustum(const Frustum& frustum, std::vector<AABB>& results);

}



