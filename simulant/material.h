/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MATERIAL_H
#define MATERIAL_H

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <set>

#include "deps/kazsignal/kazsignal.h"
#include "generic/managed.h"
#include "resource.h"
#include "loadable.h"
#include "generic/identifiable.h"
#include "generic/cloneable.h"
#include "generic/property.h"
#include "generic/refcount_manager.h"
#include "behaviours/behaviour.h"
#include "types.h"
#include "interfaces.h"
#include "material_constants.h"
#include "interfaces/updateable.h"

#include "materials/attribute_manager.h"
#include "materials/uniform_manager.h"

namespace smlt {

class MaterialPass;
class Material;


class TextureUnit:
    public Updateable {
public:
    TextureUnit(MaterialPass& pass);
    TextureUnit(MaterialPass& pass, TextureID tex_id);
    TextureUnit(MaterialPass& pass, std::vector<TextureID> textures, double duration);

    TextureUnit(const TextureUnit& rhs) = default;
    TextureUnit& operator=(const TextureUnit& rhs) = default;

    bool is_animated() const { return !animated_texture_units_.empty(); }

    TextureID texture_id() const;

    void update(float dt) {
        if(!is_animated()) return;

        time_elapsed_ += dt;
        if(time_elapsed_ >= (animated_texture_duration_ / double(animated_texture_units_.size()))) {
            current_texture_++;
            time_elapsed_ = 0.0;

            if(current_texture_ == animated_texture_units_.size()) {
                current_texture_ = 0;
            }
        }
    }

    void scroll_x(float amount) {
        Mat4 diff = Mat4::as_translation(Vec3(amount, 0, 0));
        texture_matrix_ = texture_matrix_ * diff;
    }

    void scroll_y(float amount) {
        Mat4 diff = Mat4::as_translation(Vec3(0, amount, 0));
        texture_matrix_ = texture_matrix_ * diff;
    }

    const Mat4& matrix() const {
        return texture_matrix_;
    }

    Mat4& matrix() {
        return texture_matrix_;
    }

private:
    MaterialPass* pass_;

    std::vector<TexturePtr> animated_texture_units_;
    double animated_texture_duration_;
    double time_elapsed_;
    uint32_t current_texture_;

    TexturePtr texture_unit_;
    Mat4 texture_matrix_;

    friend class MaterialPass;
    TextureUnit new_clone(MaterialPass& owner) const;
};

enum IterationType {
    ITERATE_ONCE,
    ITERATE_N,
    ITERATE_ONCE_PER_LIGHT
};


enum MaterialPropertyType {
    MATERIAL_PROPERTY_TYPE_INT,
    MATERIAL_PROPERTY_TYPE_FLOAT,
    MATERIAL_PROPERTY_TYPE_VEC2,
    MATERIAL_PROPERTY_TYPE_VEC3,
    MATERIAL_PROPERTY_TYPE_VEC4
};

struct MaterialProperty {
    MaterialProperty():
        int_value(0) {}

    MaterialPropertyType type;
    bool is_set = false; // Has this property been set (either by shader or the user)

    // FIXME: Union? Works fine on newer compilers, fails on GCC 4.7.3 (Dreamcast)
    int int_value;
    float float_value;
    Vec2 vec2_value;
    Vec3 vec3_value;
    Vec4 vec4_value;
};

typedef std::unordered_map<std::string, MaterialProperty> MaterialProperties;

class MaterialPass:
    public Managed<MaterialPass>,
    public Updateable {
public:
    MaterialPass(Material* material);

    void set_shininess(float s) { shininess_ = s; }
    void set_ambient(const smlt::Colour& colour) { ambient_ = colour; }
    void set_diffuse(const smlt::Colour& colour) { diffuse_ = colour; }
    void set_specular(const smlt::Colour& colour) { specular_ = colour; }

    void set_texture_unit(uint32_t texture_unit_id, TextureID tex);
    void set_animated_texture_unit(uint32_t texture_unit_id, const std::vector<TextureID> textures, double duration);

    const Colour& diffuse() const { return diffuse_; }
    const Colour& ambient() const { return ambient_; }
    const Colour& specular() const { return specular_; }
    float shininess() const { return shininess_; }

    uint32_t texture_unit_count() const { return texture_units_.size(); }
    TextureUnit& texture_unit(uint32_t index) { return texture_units_.at(index); }
    const TextureUnit& texture_unit(uint32_t index) const { return texture_units_.at(index); }

    void update(float dt) {
        for(TextureUnit& t: texture_units_) {
            t.update(dt);
        }
    }

    IterationType iteration() const { return iteration_; }
    uint32_t max_iterations() const { return max_iterations_; }
    void set_iteration(IterationType iter_type, uint32_t max=8);
    void set_blending(BlendType blend) { blend_ = blend; }
    BlendType blending() const { return blend_; }

    bool is_blended() const { return blend_ != BLEND_NONE; }

    void set_depth_write_enabled(bool value=true) {
        depth_writes_enabled_ = value;
    }

    bool depth_write_enabled() const { return depth_writes_enabled_; }

    void set_depth_test_enabled(bool value=true) {
        depth_test_enabled_ = value;
    }
    bool depth_test_enabled() const { return depth_test_enabled_; }

    void set_lighting_enabled(bool value=true) {
        lighting_enabled_ = value;
    }

    bool lighting_enabled() const { return lighting_enabled_; }

    void set_texturing_enabled(bool value=true) {
        texturing_enabled_ = value;
    }

    bool texturing_enabled() const { return texturing_enabled_; }

    void set_point_size(float ps) { point_size_ = ps; }

    float point_size() const { return point_size_; }

    void set_albedo(float reflectiveness);
    float albedo() const { return albedo_; }
    bool is_reflective() const { return albedo_ > 0.0; }
    void set_reflection_texture_unit(uint8_t i) { reflection_texture_unit_ = i; }
    uint8_t reflection_texture_unit() const { return reflection_texture_unit_; }

    void set_polygon_mode(PolygonMode mode) { polygon_mode_ = mode; }
    PolygonMode polygon_mode() const { return polygon_mode_; }

    void set_cull_mode(CullMode mode) { cull_mode_ = mode; }
    CullMode cull_mode() const { return cull_mode_; }

    void set_shade_model(ShadeModel model) { shade_model_ = model; }
    ShadeModel shade_model() const { return shade_model_; }

    void set_colour_material(ColourMaterial material) {
        colour_material_ = material;
    }
    ColourMaterial colour_material() const { return colour_material_; }

    void set_prevent_textures(bool value) { allow_textures_ = !value; }

    Property<MaterialPass, Material> material = { this, &MaterialPass::material_ };    
    Property<MaterialPass, UniformManager> uniforms = { this, &MaterialPass::uniforms_ };
    Property<MaterialPass, AttributeManager> attributes = { this, &MaterialPass::attributes_ };

    const GPUProgramID& gpu_program_id() const { return gpu_program_; }
    void set_gpu_program_id(GPUProgramID program_id);

private:
    UniformManager uniforms_;
    AttributeManager attributes_;

    static std::shared_ptr<GPUProgram> default_program;

    Material* material_ = nullptr;

    GPUProgramID gpu_program_;
    GPUProgramPtr gpu_program_ref_;

    std::map<std::string, float> float_uniforms_;
    std::map<std::string, int> int_uniforms_;


    Colour diffuse_ = Colour::WHITE;
    Colour ambient_ = Colour::BLACK;
    Colour specular_ = Colour::BLACK;
    float shininess_ = 0.0;
    bool allow_textures_ = true;

    std::vector<TextureUnit> texture_units_;

    IterationType iteration_ = ITERATE_ONCE;
    uint32_t max_iterations_ = 0;

    BlendType blend_;

    bool depth_writes_enabled_ = true;
    bool depth_test_enabled_ = true;
    bool lighting_enabled_ = false; // Only has an effect on GL 1.x
    bool texturing_enabled_ = true; // Only has an effect on GL 1.x

    float point_size_;

    float albedo_ = 0.0;
    uint8_t reflection_texture_unit_ = 0;

    PolygonMode polygon_mode_ = POLYGON_MODE_FILL;
    CullMode cull_mode_ = CULL_MODE_BACK_FACE;
    ShadeModel shade_model_ = SHADE_MODEL_SMOOTH;
    ColourMaterial colour_material_ = COLOUR_MATERIAL_NONE;

    friend class Material;
    MaterialPass::ptr new_clone(Material *owner) const;
};

typedef sig::signal<void (MaterialID)> MaterialChangedSignal;

class Material:
    public Resource,
    public Loadable,
    public generic::Identifiable<MaterialID>,
    public Managed<Material>,
    public Updateable,
    public Organism {

    DEFINE_SIGNAL(MaterialChangedSignal, signal_material_changed);

public:
    struct BuiltIns {
        static const std::string TEXTURE_ONLY;
        static const std::string DIFFUSE_ONLY;
        static const std::string ALPHA_TEXTURE;
        static const std::string DIFFUSE_WITH_LIGHTING;
        static const std::string MULTITEXTURE2_MODULATE;
        static const std::string MULTITEXTURE2_ADD;
        static const std::string TEXTURE_WITH_LIGHTMAP;
        static const std::string TEXTURE_WITH_LIGHTMAP_AND_LIGHTING;
        static const std::string MULTITEXTURE2_MODULATE_WITH_LIGHTING;
        static const std::string SKYBOX;
        static const std::string TEXTURED_PARTICLE;
        static const std::string DIFFUSE_PARTICLE;
    };

    static const std::map<std::string, std::string> BUILT_IN_NAMES;

    Material(MaterialID mat_id, ResourceManager* resource_manager);
    ~Material();

    void update(float dt) override;
    bool has_reflective_pass() const { return !reflective_passes_.empty(); }

    uint32_t new_pass();
    MaterialPass::ptr pass(uint32_t index);
    uint32_t pass_count() const { return pass_count_; }
    MaterialPass::ptr first_pass() {
        if(pass_count()) {
            return pass(0);
        }
        return MaterialPass::ptr();
    }
    void delete_pass(uint32_t index);

    void set_texture_unit_on_all_passes(uint32_t texture_unit_id, TextureID tex);

    MaterialID new_clone(ResourceManager* target_resource_manager, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) const;

    void each(std::function<void (uint32_t, MaterialPass*)> callback) {
        // Wait until we can lock the atomic flag
        std::vector<MaterialPass::ptr> passes;
        {
            std::lock_guard<std::mutex> lock(pass_lock_);
            passes = passes_; // Copy, to prevent holding the lock for the entire loop
        }

        uint32_t i = 0;
        for(auto pass: passes) {
            callback(i++, pass.get());
        }
    }

public:
    typedef sig::signal<void (MaterialID, MaterialPass*)> MaterialPassCreated;
    typedef sig::signal<void (MaterialID, MaterialPass*)> MaterialPassDestroyed;

    MaterialPassCreated& signal_material_pass_created() { return signal_material_pass_created_; }
    MaterialPassDestroyed& signal_material_pass_destroyed() { return signal_material_pass_destroyed_; }

    void set_int_property(const std::string& name, int value);
    void set_float_property(const std::string& name, float value);

    void create_int_property(const std::string& name);
    void create_float_property(const std::string& name);

    const MaterialProperties& properties() const { return properties_; }

private:
    MaterialPassCreated signal_material_pass_created_;
    MaterialPassDestroyed signal_material_pass_destroyed_;

    void on_pass_created(MaterialPass *pass);
    void on_pass_changed(MaterialPass *pass);
    void on_pass_destroyed(MaterialPass *pass);

private:
    /*
     * Although individual resources are not thread-safe
     * we do call update() automatically, which means that without
     * some kind of locking manipulating materials would be impossible.
     *
     * This flag keeps track of whether we should be updating, and we
     * set it and clear it whenever we manipulate the material. If the flag
     * is set then updating won't happen until it's cleared
     */
    std::atomic_flag updating_disabled_ = ATOMIC_FLAG_INIT;

    /*
     * Are we iterating passes? If so we should block on adding/removing them
     */
    std::mutex pass_lock_;

    std::vector<MaterialPass::ptr> passes_;
    uint32_t pass_count_; // Used to prevent locking on passes_ when requesting the count

    std::set<MaterialPass*> reflective_passes_;

    MaterialProperties properties_;

    friend class MaterialPass;
};

}

#endif // MATERIAL_H
