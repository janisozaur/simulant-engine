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

#include "tiled_loader.h"
#include "../meshes/mesh.h"
#include "../types.h"
#include "../extra/tiled/TmxParser/Tmx.h"
#include "../resource_manager.h"

namespace smlt {
namespace loaders {

struct TilesetInfo {
    uint32_t total_width;
    uint32_t total_height;

    uint32_t tile_width;
    uint32_t tile_height;

    uint32_t spacing;
    uint32_t margin;

    uint32_t num_tiles_wide() const {
        return (total_width - (margin * 2) + spacing) / (tile_width + spacing);
    }

    uint32_t num_tiles_high() const {
        return (total_height - (margin * 2) + spacing) / (tile_height + spacing);
    }
};

void TiledLoader::into(Loadable &resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;
    Mesh* mesh = dynamic_cast<Mesh*>(res_ptr);

    if(!mesh) {
        throw std::runtime_error("Tried to load a TMX file into something that wasn't a mesh");
    }

    Tmx::Map map;

    map.ParseFile(this->filename_.encode());

    unicode layer_name = smlt::any_cast<unicode>(options.at("layer"));
    float tile_render_size = smlt::any_cast<float>(options.at("render_size"));

    auto layers = map.GetLayers();
    auto it = std::find_if(layers.begin(), layers.end(), [=](Tmx::Layer* layer) { return layer->GetName() == layer_name.encode(); });

    if(it == layers.end()) {
        throw std::runtime_error(_u("Unable to find the layer with name: {0}").format(layer_name).encode());
    }

    Tmx::Layer* layer = (*it);

    auto parent_dir = kfs::path::abs_path(kfs::path::dir_name(filename_.encode()));

    std::map<int32_t, TilesetInfo> tileset_info;

    std::map<int32_t, MaterialID> tileset_materials;


    //Load all of the tilesets from the map
    for(int32_t i = 0; i < map.GetNumTilesets(); ++i) {
        const Tmx::Tileset* tileset = map.GetTileset(i);
        const Tmx::Image* image = tileset->GetImage();
        std::string rel_path = image->GetSource();

        auto final_path = kfs::path::join(parent_dir, rel_path);
        L_DEBUG(_F("Loading tileset from: {0}").format(final_path));

        TextureID tid = mesh->resource_manager().new_texture_from_file(
            final_path,
            TextureFlags(MIPMAP_GENERATE_NONE, TEXTURE_WRAP_CLAMP_TO_EDGE, TEXTURE_FILTER_POINT)
        );

        tileset_materials[i] = mesh->resource_manager().new_material_from_texture(tid);

        TilesetInfo& info = tileset_info[i];
        info.margin = tileset->GetMargin();
        info.spacing = tileset->GetSpacing();
        info.tile_height = tileset->GetTileHeight();
        info.tile_width = tileset->GetTileWidth();
        info.total_height = image->GetHeight();
        info.total_width = image->GetWidth();
    }

    //Store useful information on the mesh
    mesh->data->stash(layer->GetHeight(), "TILED_LAYER_HEIGHT");
    mesh->data->stash(layer->GetWidth(), "TILED_LAYER_WIDTH");
    mesh->data->stash(map.GetTileWidth(), "TILED_MAP_TILE_WIDTH");
    mesh->data->stash(map.GetTileHeight(), "TILED_MAP_TILE_HEIGHT");
    mesh->data->stash(tile_render_size, "TILED_TILE_RENDER_SIZE");

    /*
      Now go through the layer and build up a tile submesh for each grid square. Originally I chunked these
      tiles into groups for nicer culling but this is the wrong place for that. If rendering a lot of submeshes
      is inefficient then that needs to be tackled in the partitioner.
    */

    for(int32_t y = 0; y < layer->GetHeight(); ++y) {
        for(int32_t x = 0; x < layer->GetWidth(); ++x) {
            int32_t tileset_index = layer->GetTileTilesetIndex(x, y);
            if(tileset_index < 0) {
                continue;
            }

            TilesetInfo& tileset = tileset_info[tileset_index];

            int32_t tile_id = layer->GetTileId(x, y);

            int32_t num_tiles_wide = tileset.num_tiles_wide();

            int32_t x_offset = tile_id % num_tiles_wide;
            int32_t y_offset = tile_id / num_tiles_wide;

            float x0 = x_offset * (tileset.tile_width + tileset.spacing) + tileset.margin;
            float y0 = tileset.total_height - y_offset * (tileset.tile_height + tileset.spacing) - tileset.margin;

            float x1 = x0 + tileset.tile_width;
            float y1 = y0 - tileset.tile_height;

            Vec3 offset;
            offset.x = (float(x) * tile_render_size) + (0.5 * tile_render_size);
            offset.y = (float(layer->GetHeight() - y) * tile_render_size) - (0.5 * tile_render_size);

            std::string name = layer->GetName() + " (" + std::to_string(offset.x) + "," + std::to_string(offset.y) + ")";

            //Create the submesh as a rectangle, the offset determines the location on the map
            auto submesh = mesh->new_submesh_as_rectangle(name, tileset_materials.at(tileset_index), tile_render_size, tile_render_size, Vec3(offset.x, offset.y, 0));

            //Set texture coordinates appropriately for the tileset
            float tx0 = x0 / tileset.total_width + (0.5 / tileset.total_width);
            float ty0 = y0 / tileset.total_height - (0.5 / tileset.total_height);
            float tx1 = x1 / tileset.total_width - (0.5 / tileset.total_width);
            float ty1 = y1 / tileset.total_height + (0.5 / tileset.total_height);

            submesh->vertex_data->move_to(submesh->index_data->at(0));
            submesh->vertex_data->tex_coord0(smlt::Vec2(tx0, ty1));

            submesh->vertex_data->move_next();
            submesh->vertex_data->tex_coord0(smlt::Vec2(tx1, ty1));

            submesh->vertex_data->move_next();
            submesh->vertex_data->tex_coord0(smlt::Vec2(tx1, ty0));

            submesh->vertex_data->move_next();
            submesh->vertex_data->tex_coord0(smlt::Vec2(tx0, ty0));

            submesh->vertex_data->done();
        }
    }
}

}
}
