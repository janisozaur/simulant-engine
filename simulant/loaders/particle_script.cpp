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

#include "../deps/jsonic/jsonic.h"

#include "particle_script.h"
#include "../material.h"
#include "../stage.h"
#include "../nodes/particle_system.h"
#include "../nodes/particles/manipulators/size_manipulator.h"

namespace smlt {
namespace loaders {

void KGLPLoader::into(Loadable &resource, const LoaderOptions &options) {
    ParticleSystem* ps = loadable_to<ParticleSystem>(resource);
    jsonic::Node js;

    jsonic::loads(
        std::string((std::istreambuf_iterator<char>(*this->data_)), std::istreambuf_iterator<char>()),
        js
    );

    ps->set_name((js.has_key("name")) ? _u(js["name"]): "");

    L_DEBUG(_F("Loading particle system: {0}").format(ps->name()));

    if(js.has_key("quota")) ps->set_quota(uint32_t(js["quota"]));
    L_DEBUG(_F("    Quota: {0}").format(ps->quota()));

    if(js.has_key("particle_width")) ps->set_particle_width((uint32_t)js["particle_width"]);
    L_DEBUG(_F("    Particle Width: {0}").format(ps->particle_width()));

    if(js.has_key("particle_height")) ps->set_particle_height((uint32_t)js["particle_height"]);
    L_DEBUG(_F("    Particle Height: {0}").format(ps->particle_height()));

    if(js.has_key("cull_each")) ps->set_cull_each((bool) js["cull_each"]);
    L_DEBUG(_F("    Cull Each: {0}").format(ps->cull_each()));

    if(js.has_key("material")) {
        std::string material = js["material"];

        if(Material::BUILT_IN_NAMES.count(material)) {
            material = Material::BUILT_IN_NAMES.at(material);
        }

        ps->set_material_id(ps->stage->assets->new_material_from_file(material));
    }


    if(js.has_key("emitters")) {
        L_DEBUG("Loading emitters");

        jsonic::Node& emitters = js["emitters"];
        for(uint32_t i = 0; i < emitters.length(); ++i) {
            jsonic::Node& emitter = emitters[i];

            auto new_emitter = ps->push_emitter();

            if(emitter.has_key("type")) {
                auto emitter_type = std::string(emitter["type"]);
                L_DEBUG(_F("Emitter {0} has type {1}").format(i, emitter_type));
                new_emitter->set_type((emitter_type == "point") ? particles::PARTICLE_EMITTER_POINT : particles::PARTICLE_EMITTER_BOX);
            }

            if(emitter.has_key("direction")) {
                auto parts = unicode(emitter["direction"]).split(" ");
                //FIXME: check length
                new_emitter->set_direction(smlt::Vec3(
                    parts.at(0).to_float(),
                    parts.at(1).to_float(),
                    parts.at(2).to_float()
                ));
            }

            if(emitter.has_key("velocity")) {
                new_emitter->set_velocity(emitter["velocity"]);
            }

            if(emitter.has_key("width")) {
                new_emitter->set_width(emitter["width"]);
            }

            if(emitter.has_key("height")) {
                new_emitter->set_height(emitter["height"]);
            }

            if(emitter.has_key("depth")) {
                new_emitter->set_depth(emitter["depth"]);
            }

            if(emitter.has_key("ttl")) {
                new_emitter->set_ttl(emitter["ttl"]);
            } else {
                if(emitter.has_key("ttl_min") && emitter.has_key("ttl_max")) {
                    new_emitter->set_ttl_range(emitter["ttl_min"], emitter["ttl_max"]);
                } else if(emitter.has_key("ttl_min")) {
                    new_emitter->set_ttl_range(emitter["ttl_min"], new_emitter->ttl_range().second);
                } else if(emitter.has_key("ttl_max")) {
                    new_emitter->set_ttl_range(new_emitter->ttl_range().first, emitter["ttl_max"]);
                }
            }

            if(emitter.has_key("duration")) {
                new_emitter->set_duration(emitter["duration"]);
            }

            if(emitter.has_key("repeat_delay")) {
                new_emitter->set_repeat_delay(emitter["repeat_delay"]);
            }

            if(emitter.has_key("angle")) {
                new_emitter->set_angle(Degrees(emitter["angle"]));
            }

            if(emitter.has_key("colour")) {
                auto parts = unicode(emitter["colour"]).split(" ");
                new_emitter->set_colour(smlt::Colour(
                    parts.at(0).to_float(), parts.at(1).to_float(), parts.at(2).to_float(), parts.at(3).to_float()
                ));
            }

            if(emitter.has_key("emission_rate")) {
                new_emitter->set_emission_rate(emitter["emission_rate"]);
            }
        }

        if(js.has_key("manipulators")) {
            jsonic::Node& manipulators = js["manipulators"];
            for(uint32_t i = 0; i < manipulators.length(); ++i) {
                auto& manipulator = manipulators[i];

                if(std::string(manipulator["type"]) == "size") {
                    auto m = ps->new_manipulator<particles::SizeManipulator>();

                    if(manipulator.has_key("rate")) {
                        m->set_property("rate", (float) manipulator["rate"]);
                    }
                }
            }
        }
    }
}

}
}

