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

#ifndef PARTITIONER_H
#define PARTITIONER_H

#include <stack>
#include <memory>
#include <set>
#include <vector>

#include "generic/property.h"
#include "generic/managed.h"
#include "renderers/renderer.h"
#include "types.h"
#include "interfaces.h"

namespace smlt {

class SubActor;

enum WriteOperation {
    WRITE_OPERATION_ADD,
    WRITE_OPERATION_UPDATE,
    WRITE_OPERATION_REMOVE
};

enum StageNodeType {
    STAGE_NODE_TYPE_ACTOR,
    STAGE_NODE_TYPE_LIGHT,
    STAGE_NODE_TYPE_GEOM,
    STAGE_NODE_TYPE_PARTICLE_SYSTEM
};

struct StagedWrite {
    WriteOperation operation;
    StageNodeType stage_node_type;

    GeomID geom_id;
    ActorID actor_id;
    LightID light_id;
    ParticleSystemID particle_system_id;

    AABB new_bounds;
};


class Partitioner:
    public Managed<Partitioner> {

public:
    Partitioner(Stage* ss):
        stage_(ss) {}

    void add_particle_system(ParticleSystemID particle_system_id);
    void update_particle_system(ParticleSystemID particle_system_id, const AABB& bounds);
    void remove_particle_system(ParticleSystemID particle_system_id);

    void add_geom(GeomID geom_id);
    void remove_geom(GeomID geom_id);

    void add_actor(ActorID actor_id);
    void update_actor(ActorID actor_id, const AABB& bounds);
    void remove_actor(ActorID actor_id);

    void add_light(LightID light_id);
    void update_light(LightID light_id, const AABB& bounds);
    void remove_light(LightID light_id);

    void _apply_writes();

    virtual void lights_and_geometry_visible_from(
        CameraID camera_id,
        std::vector<LightID>& lights_out,
        std::vector<StageNode*>& geom_out
    ) = 0;

    virtual MeshID debug_mesh_id() { return MeshID(); }
protected:
    Property<Partitioner, Stage> stage = { this, &Partitioner::stage_ };

    Stage* get_stage() const { return stage_; }

    void stage_write(const StagedWrite& op);

    virtual void apply_staged_write(const StagedWrite& write) = 0;

private:
    Stage* stage_;

    std::mutex staging_lock_;
    std::stack<StagedWrite> staged_writes_;
};

}

#endif // PARTITIONER_H
