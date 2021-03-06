#include "static_body.h"
#include "simulation.h"

#include "../../deps/bounce/bounce.h"
#include "../../utils/mesh/triangulate.h"

namespace smlt {
namespace behaviours {

std::unordered_map<MeshID, std::shared_ptr<StaticBody::b3MeshGenerator>> StaticBody::mesh_cache;

StaticBody::StaticBody(RigidBodySimulation* simulation):
    Body(simulation) {

}

StaticBody::~StaticBody() {

}

StaticBody::b3MeshGenerator::b3MeshGenerator():
    mesh_(new b3Mesh()) {

}

void StaticBody::b3MeshGenerator::append_vertex(const Vec3 &v) {
    b3Vec3 bv;
    to_b3vec3(v, bv);
    vertices_.push_back(bv);

    mesh_->vertices = &vertices_[0];
    mesh_->vertexCount = vertices_.size();
}

void StaticBody::b3MeshGenerator::append_triangle(const utils::Triangle& src) {
    b3Triangle tri;
    tri.v1 = src.idx[0];
    tri.v2 = src.idx[1];
    tri.v3 = src.idx[2];

    triangles_.push_back(tri);
    mesh_->triangles = &triangles_[0];
    mesh_->triangleCount = triangles_.size();
    mesh_->BuildTree(); // Rebuild the tree
}

void StaticBody::add_mesh_collider(const MeshID &mesh_id, const PhysicsMaterial &properties, const Vec3 &offset, const Quaternion &rotation) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    // If we haven't already seen this mesh, then create a new b3Mesh for it
    if(mesh_cache.count(mesh_id) == 0) {
        auto bmesh = std::make_shared<b3MeshGenerator>();
        auto mesh = mesh_id.fetch();

        std::vector<Vec3> vertices;
        std::vector<utils::Triangle> triangles;

        // Turn the mesh into a list of vertices + triangle indexes
        triangulate(mesh, vertices, triangles);

        // Add them to our b3Mesh generator
        bmesh->insert_vertices(vertices.begin(), vertices.end());
        bmesh->insert_triangles(triangles.begin(), triangles.end());

        // Store the generator for later
        mesh_cache.insert(std::make_pair(mesh_id, bmesh));
    }

    b3MeshShape shape;
    // Grab the b3Mesh from the generator
    shape.m_mesh = mesh_cache.at(mesh_id)->get_mesh();

    b3ShapeDef sdef;
    sdef.shape = &shape;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    store_collider(sim->bodies_.at(this)->CreateShape(sdef), properties);
}


}
}
