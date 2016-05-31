#pragma once

#include "../renderer.h"

namespace kglt {

class GL1XRenderer : public Renderer {
public:
    GL1XRenderer(WindowBase* window):
        Renderer(window) {}

    new_batcher::RenderGroup new_render_group(Renderable *renderable, MaterialPass *material_pass);

    void render(CameraPtr camera,
        StagePtr stage, bool render_group_changed,
        const new_batcher::RenderGroup *,
        Renderable* renderable,
        MaterialPass* material_pass,
        Light* light,
        new_batcher::Iteration iteration
    ) override;
};

}
