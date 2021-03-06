#include <cmath>
#include "../../frustum.h"
#include "spatial_hash.h"
#include "../../utils/endian.h"

namespace smlt {

SpatialHash::SpatialHash() {

}


void SpatialHash::insert_object_for_box(const AABB &box, SpatialHashEntry *object) {
    auto cell_size = find_cell_size_for_box(box);

    object->set_hash_aabb(box);
    for(auto& corner: box.corners()) {
        auto key = make_key(cell_size, corner.x, corner.y, corner.z);
        insert_object_for_key(key, object);
    }
}

void SpatialHash::remove_object(SpatialHashEntry *object) {
    for(auto key: object->keys()) {
        erase_object_from_key(key, object);
    }
    object->set_keys(KeyList());
}

void SpatialHash::erase_object_from_key(Key key, SpatialHashEntry* object) {
    auto it = index_.find(key);
    if(it != index_.end()) {
        it->second.erase(object);

        if(it->second.empty()) {
            index_.erase(it);
        }
    }
}

void SpatialHash::update_object_for_box(const AABB& new_box, SpatialHashEntry* object) {
    auto cell_size = find_cell_size_for_box(new_box);

    KeyList new_keys;
    const KeyList& old_keys = object->keys();

    for(auto& corner: new_box.corners()) {
        auto key = make_key(cell_size, corner.x, corner.y, corner.z);
        new_keys.insert(key);
    }

    KeyList keys_to_add, keys_to_remove;

    std::set_difference(
        new_keys.begin(), new_keys.end(), old_keys.begin(), old_keys.end(),
        std::inserter(keys_to_add, keys_to_add.end())
    );

    std::set_difference(
        old_keys.begin(), old_keys.end(), new_keys.begin(), new_keys.end(),
        std::inserter(keys_to_remove, keys_to_remove.end())
    );

    if(new_keys.empty() && old_keys.empty()) {
        return;
    }

    for(auto& key: keys_to_remove) {
        erase_object_from_key(key, object);
    }

    for(auto& key: keys_to_add) {
        insert_object_for_key(key, object);
    }

    object->set_hash_aabb(new_box);
    object->set_keys(new_keys);
}

void generate_boxes_for_frustum(const Frustum& frustum, std::vector<AABB>& results) {
    results.clear(); // Required

    auto corners = frustum.near_corners();
    auto far_corners = frustum.far_corners();
    for(auto& corner: far_corners) {
        corners.push_back(corner);
    }
    results.push_back(AABB(corners.data(), corners.size()));
    return;

    // start at the center of the near plane
    Vec3 start_point = Vec3::find_average(frustum.far_corners());

    // We want to head in the reverse direction of the frustum
    Vec3 direction = -frustum.direction().normalized();

    // Get the normals of the up and right planes so we can generated boxes
    Vec3 up = frustum.plane(FRUSTUM_PLANE_BOTTOM).normal();
    Vec3 right = frustum.plane(FRUSTUM_PLANE_LEFT).normal();

    auto near_plane = frustum.plane(FRUSTUM_PLANE_NEAR);

    // Project the up and right normals onto the near plane (otherwise they might be skewed)
    up = near_plane.project(up).normalized();
    right = near_plane.project(right).normalized();

    float distance_left = frustum.depth();
    auto p = start_point;
    while(distance_left > 0) {
        float box_size = std::max(frustum.width_at_distance(distance_left), frustum.height_at_distance(distance_left));
        float hw = box_size / 2.0;

        std::array<Vec3, 8> corners;

        corners[0] = p - (direction * box_size) - (right * hw) - (up * hw);
        corners[1] = p - (direction * box_size) + (right * hw) - (up * hw);
        corners[2] = p - (direction * box_size) + (right * hw) + (up * hw);
        corners[3] = p - (direction * box_size) - (right * hw) + (up * hw);

        corners[4] = p - (right * hw) - (up * hw);
        corners[5] = p + (right * hw) - (up * hw);
        corners[6] = p + (right * hw) + (up * hw);
        corners[7] = p - (right * hw) + (up * hw);


        AABB new_box(corners.data(), corners.size());
        results.push_back(new_box);

        distance_left -= box_size;
        p += direction * box_size;
    }
}

HGSHEntryList SpatialHash::find_objects_within_frustum(const Frustum &frustum) {
    static std::vector<AABB> boxes; // Static to avoid repeated allocations

    generate_boxes_for_frustum(frustum, boxes);

    HGSHEntryList results;

    for(auto& box: boxes) {
        for(auto& result: find_objects_within_box(box)) {
            if(frustum.intersects_aabb(result->hash_aabb())) {
                results.insert(result);
            }
        }
    }

    return results;
}

HGSHEntryList SpatialHash::find_objects_within_box(const AABB &box) {
    HGSHEntryList objects;

    auto cell_size = find_cell_size_for_box(box);

    auto gather_objects = [](Index& index, const Key& key, HGSHEntryList& objects) {
        auto it = index.lower_bound(key);
        if(it == index.end()) {
            return;
        }

        // First, iterate the index to find a key which isn't a descendent of this one
        // then break
        while(it != index.end() && key.is_ancestor_of(it->first)) {
            objects.insert(it->second.begin(), it->second.end());
            ++it;
        }

        // Now, go up the tree looking for objects which are ancestors of this key
        auto path = key;
        while(!path.is_root()) {
            path = path.parent_key();
            it = index.find(path);
            if(it != index.end() && path.is_ancestor_of(it->first)) {
                objects.insert(it->second.begin(), it->second.end());
            }
        }
    };

    KeyList seen;

    for(auto& corner: box.corners()) {
        auto key = make_key(
            cell_size,
            corner.x,
            corner.y,
            corner.z
        );
        seen.insert(key);
    }

    for(auto& key: seen) {
        gather_objects(index_, key, objects);
    }

    return objects;
}

int32_t SpatialHash::find_cell_size_for_box(const AABB &box) const {
    /*
     * We find the nearest hash size which is greater than double the max dimension of the
     * box. This increases the likelyhood that the object will not wastefully span cells
     */

    auto maxd = box.max_dimension();
    if(maxd < 1.0f) {
        return 1;
    } else {
        return 1 << uint32_t(std::ceil(::log2(maxd)));
    }
}

void SpatialHash::insert_object_for_key(Key key, SpatialHashEntry *entry) {
    auto it = index_.find(key);
    if(it != index_.end()) {
        it->second.insert(entry);
    } else {
        HGSHEntryList list;
        list.insert(entry);
        index_.insert(std::make_pair(key, list));
    }
    entry->push_key(key);
}

Key make_key(int32_t cell_size, float x, float y, float z) {
    static const int32_t MAX_PATH_SIZE = pow(2, MAX_GRID_LEVELS - 1);

    Key key;
    key.ancestors = 15 - (int(::log2(cell_size)));

    key.hash_path[0] = make_hash(MAX_PATH_SIZE, x, y, z);
    key.hash_path[1] = (key.ancestors > 0) ? make_hash(MAX_PATH_SIZE / 2, x, y, z) : Hash();
    key.hash_path[2] = (key.ancestors > 1) ? make_hash(MAX_PATH_SIZE / 4, x, y, z) : Hash();
    key.hash_path[3] = (key.ancestors > 2) ? make_hash(MAX_PATH_SIZE / 8, x, y, z) : Hash();
    key.hash_path[4] = (key.ancestors > 3) ? make_hash(MAX_PATH_SIZE / 16, x, y, z) : Hash();
    key.hash_path[5] = (key.ancestors > 4) ? make_hash(MAX_PATH_SIZE / 32, x, y, z) : Hash();
    key.hash_path[6] = (key.ancestors > 5) ? make_hash(MAX_PATH_SIZE / 64, x, y, z) : Hash();
    key.hash_path[7] = (key.ancestors > 6) ? make_hash(MAX_PATH_SIZE / 128, x, y, z) : Hash();
    key.hash_path[8] = (key.ancestors > 7) ? make_hash(MAX_PATH_SIZE / 256, x, y, z) : Hash();
    key.hash_path[9] = (key.ancestors > 8) ? make_hash(MAX_PATH_SIZE / 512, x, y, z) : Hash();
    key.hash_path[10] = (key.ancestors > 9) ? make_hash(MAX_PATH_SIZE / 1024, x, y, z) : Hash();
    key.hash_path[11] = (key.ancestors > 10) ? make_hash(MAX_PATH_SIZE / 2048, x, y, z) : Hash();
    key.hash_path[12] = (key.ancestors > 11) ? make_hash(MAX_PATH_SIZE / 4096, x, y, z) : Hash();
    key.hash_path[13] = (key.ancestors > 12) ? make_hash(MAX_PATH_SIZE / 8192, x, y, z) : Hash();
    key.hash_path[14] = (key.ancestors > 13) ? make_hash(MAX_PATH_SIZE / 16384, x, y, z) : Hash();
    key.hash_path[15] = (key.ancestors > 14) ? make_hash(1, x, y, z) : Hash();

    /* Precalculate the hash_code for speed */
    key.hash_code = 0;
    for(uint8_t i = 0; i < smlt::MAX_GRID_LEVELS; ++i) {
        std::hash_combine(key.hash_code, key.hash_path[i].x);
        std::hash_combine(key.hash_code, key.hash_path[i].y);
        std::hash_combine(key.hash_code, key.hash_path[i].z);
    }

    return key;
}

Hash make_hash(int32_t cell_size, float x, float y, float z) {
    Hash hash;

    hash.x = int16_t(std::floor(x / cell_size));
    hash.y = int16_t(std::floor(y / cell_size));
    hash.z = int16_t(std::floor(z / cell_size));

    return hash;
}

Key Key::parent_key() const {
    assert(!is_root());

    Key ret;
    memcpy(ret.hash_path, hash_path, sizeof(Hash) * ancestors);
    ret.ancestors = ancestors - 1;
    return ret;
}

bool Key::is_ancestor_of(const Key &other) const {
    if(ancestors > other.ancestors) return false;

    return memcmp(hash_path, other.hash_path, sizeof(Hash) * (ancestors + 1)) == 0;
}

std::ostream &operator<<(std::ostream &os, const Key &key) {
    for(uint32_t i = 0; i < key.ancestors + 1; ++i) {
        os << key.hash_path[i].x << key.hash_path[i].y << key.hash_path[i].z;
        if(i != key.ancestors) {
            os << " / ";
        }
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const SpatialHash &hash) {
    for(auto& pair: hash.index_) {
        os << pair.first << " : " << pair.second.size() << " items" << std::endl;
    }

    return os;
}


}
