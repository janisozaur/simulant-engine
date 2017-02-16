//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../deps/kazlog/kazlog.h"

#include "../sdl2_window.h"
#include "../stage.h"
#include "../mesh.h"
#include "../types.h"
#include "../nodes/light.h"
#include "../camera.h"
#include "../procedural/texture.h"
#include "../controllers/material/flowing.h"
#include "../controllers/material/warp.h"

#include "q2bsp_loader.h"

#include "simulant/shortcuts.h"

namespace smlt  {
namespace loaders {

namespace Q2 {

enum LumpType {
    ENTITIES = 0,
    PLANES,
    VERTICES,
    VISIBILITY,
    NODES,
    TEXTURE_INFO,
    FACES,
    LIGHTMAPS,
    LEAVES,
    LEAF_FACE_TABLE,
    LEAF_BRUSH_TABLE,
    EDGES,
    FACE_EDGE_TABLE,
    MODELS,
    BRUSHES,
    BRUSH_SIDES,
    POP,
    AREAS,
    AREA_PORTALS,
    MAX_LUMPS
};

typedef kmVec3 Point3f;

struct Point3s {
    int16_t x;
    int16_t y;
    int16_t z;
};

struct Edge {
    uint16_t a;
    uint16_t b;
};

enum SurfaceFlag {
    SURFACE_FLAG_NONE = 0x0,
    SURFACE_FLAG_LIGHT = 0x1,
    SURFACE_FLAG_SLICK = 0x2,
    SURFACE_FLAG_SKY = 0x4,
    SURFACE_FLAG_WARP = 0x8,
    SURFACE_FLAG_TRANS_33 = 0x10,
    SURFACE_FLAG_TRANS_66 = 0x20,
    SURFACE_FLAG_FLOWING = 0x40,
    SURFACE_FLAG_NO_DRAW = 0x80
};

struct TextureInfo {
    Point3f u_axis;
    float u_offset;
    Point3f v_axis;
    float v_offset;

    SurfaceFlag flags;
    uint32_t value;

    char texture_name[32];

    uint32_t next_tex_info;
};

struct Face {
    uint16_t plane;             // index of the plane the face is parallel to
    uint16_t plane_side;        // set if the normal is parallel to the plane normal
    uint32_t first_edge;        // index of the first edge (in the face edge array)
    uint16_t num_edges;         // number of consecutive edges (in the face edge array)
    uint16_t texture_info;      // index of the texture info structure
    uint8_t lightmap_syles[4]; // styles (bit flags) for the lightmaps
    uint32_t lightmap_offset;   // offset of the lightmap (in bytes) in the lightmap lump
};

struct Lump {
    uint32_t offset;
    uint32_t length;
};

struct Header {
    uint8_t magic[4];
    uint32_t version;

    Lump lumps[MAX_LUMPS];
};

}

void parse_actors(const std::string& actor_string, Q2EntityList& actors) {
    bool inside_actor = false;
    Q2Entity current;
    unicode key, value;
    bool inside_key = false, inside_value = false, key_done_for_this_line = false;
    for(char c: actor_string) {
        if(c == '{' && !inside_actor) {
            inside_actor = true;
            current.clear();
        }
        else if(c == '}' && inside_actor) {
            assert(inside_actor);
            inside_actor = false;
            actors.push_back(current);
        }
        else if(c == '\n' || c == '\r') {
            key_done_for_this_line = false;
            if(!key.empty() && !value.empty()) {
                key = key.strip();
                value = value.strip();

                current[key.encode()] = value.encode();
                std::cout << "Storing {" << key << " : " << value << "}" << std::endl;
            }
            key = "";
            value = "";
        }
        else if (c == '"') {
            if(!inside_key && !inside_value) {
                if(!key_done_for_this_line) {
                    inside_key = true;
                } else {
                    inside_value = true;
                }

            }
            else if(inside_key) {
                inside_key = false;
                key_done_for_this_line = true;
            } else {
                inside_value = false;
            }
        }
        else {
            if(inside_key) {
                key.push_back(c);
            } else {
                value.push_back(c);
            }

        }
    }

}

unicode locate_texture(ResourceLocator& locator, const unicode& filename) {
    std::vector<unicode> extensions = { ".wal", ".jpg", ".tga", ".jpeg", ".png" };
    for(auto& ext: extensions) {
        try {
            return locator.locate_file(filename + ext);
        } catch(std::runtime_error&) {
            continue;
        }
    }

    return "";
}

template<typename T>
uint32_t read_lump(std::istream& file, const Q2::Header& header, Q2::LumpType type, std::vector<T>& lumpout) {
    uint32_t count = header.lumps[type].length / sizeof(T);
    lumpout.resize(count);
    file.seekg((std::istream::pos_type) header.lumps[type].offset);
    file.read((char*)&lumpout[0], (int) sizeof(T) * count);
    return count;
}

void Q2BSPLoader::into(Loadable& resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;
    Mesh* mesh = dynamic_cast<Mesh*>(res_ptr);

    // Make sure the passed mesh is empty and using the default vertex spec
    mesh->reset(smlt::VertexSpecification::DEFAULT);

    auto assets = &mesh->resource_manager();
    auto& locator = assets->window->resource_locator;

    assert(mesh && "You passed a Resource that is not a mesh to the QBSP loader");

    std::map<std::string, TextureID> texture_lookup;
    TextureID checkerboard = assets->new_texture_from_file(Texture::BuiltIns::CHECKERBOARD);

    auto texture_info_visible = [](Q2::TextureInfo& info) -> bool {
        /* Don't draw invisible things */
        if((info.flags & Q2::SURFACE_FLAG_NO_DRAW) == Q2::SURFACE_FLAG_NO_DRAW) {
            return false;
        }

        /* Don't use sky faces (might change this... could create a skybox) */
        if((info.flags & Q2::SURFACE_FLAG_SKY) == Q2::SURFACE_FLAG_SKY) {
            return false;
        }

        return true;
    };

    auto find_or_load_texture = [&](const std::string& texture_name) -> TextureID {
        if(texture_lookup.count(texture_name)) {
            return texture_lookup[texture_name];
        } else {
            TextureID new_texture_id;
            auto texture_filename = locate_texture(*locator.get(), texture_name);
            if(!texture_filename.empty()) {
                new_texture_id = assets->new_texture_from_file(texture_filename);
            } else {
                L_DEBUG(_F("Texture {0} was missing").format(texture_name));
                new_texture_id = checkerboard;
            }
            texture_lookup[texture_name] = new_texture_id;
            return new_texture_id;
        }
    };

    auto& file = *this->data_;

    //Needed because the Quake 2 coord system is weird
    kmMat4 rotation;
    kmMat4RotationX(&rotation, kmDegreesToRadians(-90.0f));

    Q2::Header header;
    file.read((char*)&header, sizeof(Q2::Header));

    if(std::string(header.magic, header.magic + 4) != "IBSP") {
        throw std::runtime_error("Not a valid Q2 map");
    }

    std::vector<char> actor_buffer;
    read_lump(file, header, Q2::LumpType::ENTITIES, actor_buffer);
    std::string actor_string(actor_buffer.begin(), actor_buffer.end());

    Q2EntityList entities;
    parse_actors(actor_string, entities);
    mesh->data->stash(entities, "entities");

    std::vector<Q2::Point3f> vertices;
    std::vector<Q2::Edge> edges;
    std::vector<Q2::TextureInfo> textures;
    std::vector<Q2::Face> faces;
    std::vector<uint32_t> face_edges;
    std::vector<uint8_t> lightmap_data;

    auto num_vertices = read_lump(file, header, Q2::LumpType::VERTICES, vertices);
    auto num_edges = read_lump(file, header, Q2::LumpType::EDGES, edges);
    auto num_textures = read_lump(file, header, Q2::LumpType::TEXTURE_INFO, textures);
    auto num_faces = read_lump(file, header, Q2::LumpType::FACES, faces);
    auto num_face_edges = read_lump(file, header, Q2::LumpType::FACE_EDGE_TABLE, face_edges);
    auto lightmap_length = read_lump(file, header, Q2::LumpType::LIGHTMAPS, lightmap_data);


    std::for_each(vertices.begin(), vertices.end(), [&](Q2::Point3f& vert) {
        // Dirty casts, but it should work...
        kmVec3Transform((kmVec3*)&vert, (kmVec3*)&vert, &rotation);
    });

    /* Lightmaps are a bunch of fun! There is one lightmap per-face, and that might be up to 16x16 - but
     * can be smaller. Each face has the byte offset to the lightmap in the file, and the width and height of
     * the lightmap is defined by some weird calculation. So you can't just read all the lightmaps without first
     * reading the faces. So we read the lump into memory, then while we process the faces we pack the lightmap into
     * a texture which is sqrt(num_faces) * 16 wide and tall and then manipulate the texture coords appropriately.
     */


    /**
     *  Load the textures and generate materials
     */

    std::vector<MaterialID> materials;
    std::vector<std::pair<uint32_t, uint32_t>> texture_dimensions;
    std::unordered_map<MaterialID, SubMesh*> submeshes_by_material;

    materials.resize(textures.size());
    texture_dimensions.resize(textures.size());

    uint32_t tex_idx = 0;
    for(Q2::TextureInfo& tex: textures) {
        if(!texture_info_visible(tex)) {
            ++tex_idx;
            continue;
        }

        kmVec3 u_axis, v_axis;
        kmVec3Fill(&u_axis, tex.u_axis.x, tex.u_axis.y, tex.u_axis.z);
        kmVec3Fill(&v_axis, tex.v_axis.x, tex.v_axis.y, tex.v_axis.z);
        kmVec3Transform(&u_axis, &u_axis, &rotation);
        kmVec3Transform(&v_axis, &v_axis, &rotation);
        tex.u_axis.x = u_axis.x;
        tex.u_axis.y = u_axis.y;
        tex.u_axis.z = u_axis.z;

        tex.v_axis.x = v_axis.x;
        tex.v_axis.y = v_axis.y;
        tex.v_axis.z = v_axis.z;

        TextureID new_texture_id = find_or_load_texture(tex.texture_name);

        MaterialID new_material_id;

        bool uses_lightmap = !(tex.flags & (Q2::SURFACE_FLAG_SKY | Q2::SURFACE_FLAG_WARP));
        if(uses_lightmap) {
            new_material_id = assets->new_material_from_file(Material::BuiltIns::TEXTURE_WITH_LIGHTMAP);
        } else {
            new_material_id = assets->new_material_from_file(Material::BuiltIns::TEXTURE_ONLY);
        }

        {
            auto mat = assets->material(new_material_id);
            mat->pass(0)->set_texture_unit(0, new_texture_id);
/*
            if(uses_lightmap) {
                mat->pass(0)->set_texture_unit(1, lightmap_texture);
            }*/

            if(tex.flags & Q2::SURFACE_FLAG_FLOWING) {
                mat->new_controller<controllers::material::Flowing>();
            } else if(tex.flags & Q2::SURFACE_FLAG_WARP) {
                mat->new_controller<controllers::material::Warp>();
            }

            if(tex.flags & Q2::SURFACE_FLAG_TRANS_33) {
                mat->pass(0)->set_diffuse(smlt::Colour(1.0, 1.0, 1.0, 0.33));
                mat->pass(0)->set_blending(BLEND_ALPHA);
            }

            if(tex.flags & Q2::SURFACE_FLAG_TRANS_66) {
                mat->pass(0)->set_diffuse(smlt::Colour(1.0, 1.0, 1.0, 0.66));
                mat->pass(0)->set_blending(BLEND_ALPHA);
            }
        }

        auto texture = assets->texture(new_texture_id);
        texture_dimensions[tex_idx].first = texture->width();
        texture_dimensions[tex_idx].second = texture->height();

        materials[tex_idx] = new_material_id;
        submeshes_by_material[new_material_id] = mesh->new_submesh_with_material(std::to_string(tex_idx), new_material_id);
        tex_idx++;
    }

    std::cout << "Num textures: " << texture_lookup.size() << std::endl;
    std::cout << "Num submeshes: " << mesh->submesh_count() << std::endl;


    typedef uint32_t Offset;
    typedef uint32_t FaceIndex;


    int32_t face_index = -1;
    for(Q2::Face& f: faces) {
        Q2::TextureInfo& tex = textures[f.texture_info];

        if(!texture_info_visible(tex)) {
            continue;
        }

        ++face_index;

        std::vector<uint32_t> indexes;
        for(uint32_t i = f.first_edge; i < f.first_edge + f.num_edges; ++i) {
            int32_t edge_idx = face_edges[i];
            if(edge_idx > 0) {
                Q2::Edge& e = edges[edge_idx];
                indexes.push_back(e.a);
                //indexes.push_back(e.b);
            } else {
                edge_idx = -edge_idx;
                Q2::Edge& e = edges[edge_idx];
                indexes.push_back(e.b);
                //indexes.push_back(e.a);
            }
        }

        auto material_id = materials[f.texture_info];
        SubMesh* sm = submeshes_by_material[material_id];

        /*
         *  A unique vertex is defined by a combination of the position ID and the
         *  texture_info index (because texture coordinates depend on both and some
         *  some vertices must be duplicated.
         *
         *  Here we store a mapping so that we don't create duplicate vertices if we don't need to!
         */

        /*
         * Build the triangles for this "face"
         */

        std::map<uint32_t, uint32_t> index_lookup;
        kmVec3 normal;
        kmVec3Fill(&normal, 0, 1, 0);

        short min_u = 10000, max_u = -10000;
        short min_v = 10000, max_v = -10000;

        for(int16_t i = 1; i < (int16_t) indexes.size() - 1; ++i) {
            uint32_t tri_idx[] = {
                indexes[0],
                indexes[i+1],
                indexes[i]
            };

            //Calculate the surface normal for this triangle
            kmVec3 normal;
            kmVec3 vec1, vec2;
            kmVec3& v1 = vertices[tri_idx[0]];
            kmVec3& v2 = vertices[tri_idx[1]];
            kmVec3& v3 = vertices[tri_idx[2]];

            kmVec3Subtract(&vec1, &v2, &v1);
            kmVec3Subtract(&vec2, &v3, &v1);
            kmVec3Cross(&normal, &vec1, &vec2);
            kmVec3Normalize(&normal, &normal);

            for(uint8_t j = 0; j < 3; ++j) {
                if(index_lookup.count(tri_idx[j])) {
                    //We've already processed this vertex
                    sm->index_data->index(index_lookup[tri_idx[j]]);
                    continue;
                }


                kmVec3& pos = vertices[tri_idx[j]];

                //We haven't done this before so calculate the vertex
                float u = pos.x * tex.u_axis.x
                        + pos.y * tex.u_axis.y
                        + pos.z * tex.u_axis.z + tex.u_offset;

                float v = pos.x * tex.v_axis.x
                        + pos.y * tex.v_axis.y
                        + pos.z * tex.v_axis.z + tex.v_offset;

                if(u < min_u) min_u = u;
                if(u > max_u) max_u = u;
                if(v < min_v) min_v = v;
                if(v > max_v) max_v = v;

                float w = float(texture_dimensions[f.texture_info].first);
                float h = float(texture_dimensions[f.texture_info].second);

                mesh->shared_data->position(pos);
                mesh->shared_data->normal(normal);
                mesh->shared_data->diffuse(smlt::Colour::WHITE);
                mesh->shared_data->tex_coord0(u / w, v / h);
                mesh->shared_data->tex_coord1(u / w, v / h);
                mesh->shared_data->move_next();

                sm->index_data->index(mesh->shared_data->count() - 1);

                //Cache this new vertex in the lookup
                index_lookup[tri_idx[j]] = mesh->shared_data->count() - 1;
            }
        }

    }

    mesh->shared_data->done();
    mesh->each([&](const std::string& name, SubMesh* submesh) {
        //Delete empty submeshes
        /*if(!submesh->index_data->count()) {
            mesh->delete_submesh(submesh->id());
            return;
        }*/
        submesh->index_data->done();
    });


    //FIXME: mark mesh as uncollected
}

}
}
