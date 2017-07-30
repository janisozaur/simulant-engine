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


#include "managers.h"
#include "background.h"
#include "window_base.h"
#include "stage.h"
#include "nodes/camera.h"
#include "render_sequence.h"
#include "loader.h"

#include "renderers/batching/render_queue.h"

namespace smlt {

//============== START BACKGROUNDS ==========
BackgroundManager::BackgroundManager(WindowBase* window):
    window_(window) {

}

BackgroundManager::~BackgroundManager() {
    auto objects = BackgroundManager::__objects();
    for(auto background_pair: objects) {
        delete_background(background_pair.first);
    }
}

void BackgroundManager::update(float dt) {
    //Update the backgrounds
    for(auto background_pair: BackgroundManager::__objects()) {
        auto* bg = background_pair.second.get();
        bg->update(dt);
    }
}

BackgroundID BackgroundManager::new_background() {
    BackgroundID bid = BackgroundManager::make(this);
    return bid;
}

BackgroundID BackgroundManager::new_background_from_file(const unicode& filename, float scroll_x, float scroll_y) {
    BackgroundID result = new_background();
    try {
        background(result)->set_texture(window_->shared_assets->new_texture_from_file(filename));
        background(result)->set_horizontal_scroll_rate(scroll_x);
        background(result)->set_vertical_scroll_rate(scroll_y);
    } catch(...) {
        delete_background(result);
        throw;
    }

    return result;
}

BackgroundPtr BackgroundManager::background(BackgroundID bid) {
    return BackgroundManager::get(bid).lock().get();
}

bool BackgroundManager::has_background(BackgroundID bid) const {
    return BackgroundManager::contains(bid);
}

void BackgroundManager::delete_background(BackgroundID bid) {
    BackgroundManager::destroy(bid);
}

uint32_t BackgroundManager::background_count() const {
    return BackgroundManager::count();
}

//============== END BACKGROUNDS ============

//=============== START CAMERAS ============

CameraManager::CameraManager(Stage *stage):
    stage_(stage) {

}

CameraID CameraManager::new_camera() {
    CameraID new_camera = cameras_.make(this->stage_);
    new_camera.fetch()->set_parent(stage_);

    return new_camera;
}

CameraID CameraManager::new_camera_with_orthographic_projection(double left, double right, double bottom, double top, double near, double far) {
    /*
     *  Instantiates a camera with an orthographic projection. If both left and right are zero then they default to 0 and window.width()
     *  respectively. If top and bottom are zero, then they default to window.height() and 0 respectively. So top left is 0,0
     */
    CameraID new_camera_id = new_camera();

    if(!left && !right) {
        right = stage_->window->width();
    }

    if(!bottom && !top) {
        top = stage_->window->height();
    }

    camera(new_camera_id)->set_orthographic_projection(left, right, bottom, top, near, far);

    return new_camera_id;
}

CameraID CameraManager::new_camera_for_viewport(const Viewport& vp) {
    float x, y, width, height;
    calculate_ratios_from_viewport(vp.type(), x, y, width, height);

    CameraID cid = new_camera();
    camera(cid)->set_perspective_projection(Degrees(45.0), width / height);

    return cid;
}

CameraID CameraManager::new_camera_for_ui() {
    return new_camera_with_orthographic_projection(0, stage_->window->width(), 0, stage_->window->height(), -1, 1);
}

CameraPtr CameraManager::camera(CameraID c) {
    return cameras_.get(c);
}

void CameraManager::delete_camera(CameraID cid) {
    cameras_.destroy(cid);
}

uint32_t CameraManager::camera_count() const {
    return cameras_.count();
}

const bool CameraManager::has_camera(CameraID id) const {
    return cameras_.contains(id);
}

void CameraManager::delete_all_cameras() {
    cameras_.clear();
}

//============== END CAMERAS ================


//=========== START STAGES ==================

StageManager::StageManager(WindowBase* window):
    window_(window) {

}

StageID StageManager::new_stage(AvailablePartitioner partitioner) {
    auto ret = StageManager::make(this->window_, partitioner);
    signal_stage_added_(ret);
    return ret;
}

uint32_t StageManager::stage_count() const {
    return StageManager::count();
}

/**
 * @brief StageManager::stage
 * @return A shared_ptr to the stage
 *
 * We don't return a ProtectedPtr because it makes usage a nightmare. Stages don't suffer the same potential
 * threading issues as other objects as they are the highest level object. Returning a weak_ptr means that
 * we retain ownership, and calling code won't die if the stage goes missing.
 */

StagePtr StageManager::stage(StageID s) {
    return StageManager::get(s).lock().get();
}

void StageManager::delete_stage(StageID s) {
    StageManager::destroy(s);
    signal_stage_removed_(s);
}

void StageManager::fixed_update(float dt) {
    for(auto stage_pair: StageManager::__objects()) {
        TreeNode* root = stage_pair.second.get();

        root->each_descendent_and_self([=](uint32_t, TreeNode* node) {
            StageNode* stage_node = static_cast<StageNode*>(node);
            stage_node->fixed_update(dt);
        });
    }
}

void StageManager::late_update(float dt) {
    for(auto stage_pair: StageManager::__objects()) {
        TreeNode* root = stage_pair.second.get();

        root->each_descendent_and_self([=](uint32_t, TreeNode* node) {
            StageNode* stage_node = static_cast<StageNode*>(node);
            stage_node->late_update(dt);
        });
    }
}


void StageManager::update(float dt) {
    //Update the stages
    for(auto stage_pair: StageManager::__objects()) {
        TreeNode* root = stage_pair.second.get();

        root->each_descendent_and_self([=](uint32_t, TreeNode* node) {
            StageNode* stage_node = static_cast<StageNode*>(node);
            stage_node->update(dt);
        });
    }
}


void StageManager::print_tree() {
    for(auto stage: StageManager::__objects()) {
        uint32_t counter = 0;
        print_tree(stage.second.get(), counter);
    }
}

void StageManager::print_tree(StageNode *node, uint32_t& level) {
    for(uint32_t i = 0; i < level; ++i) {
        std::cout << "    ";
    }

    std::cout << *dynamic_cast<Printable*>(node) << std::endl;

    level += 1;
    node->each_child([&](uint32_t, TreeNode* child) {
        print_tree(static_cast<StageNode*>(child), level);
    });
    level -= 1;
}

bool StageManager::has_stage(StageID stage_id) const {
    return contains(stage_id);
}

void StageManager::delete_all_stages() {
    destroy_all();
}

// ============= END STAGES ===========

}
