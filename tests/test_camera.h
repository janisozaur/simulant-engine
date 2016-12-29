#ifndef TEST_CAMERA_H
#define TEST_CAMERA_H

#include "kaztest/kaztest.h"

#include "simulant/simulant.h"
#include "global.h"

namespace {

using namespace smlt;

class CameraTest : public SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        camera_id_ = window->new_camera();
        stage_id_ = window->new_stage();
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        window->delete_camera(camera_id_);
        window->delete_stage(stage_id_);
    }

    void test_project_point() {
        window->camera(camera_id_)->set_perspective_projection(45.0, float(window->width()) / float(window->height()));

        kmVec3 p1 = window->camera(camera_id_)->project_point(*window, Viewport(), Vec3(0, 0, -10));

        assert_equal(window->width() / 2, p1.x);
        assert_equal(window->height() / 2, p1.y);

        p1 = window->camera(camera_id_)->project_point(*window, Viewport(), Vec3(1, 0, -10));

        assert_true(p1.x > (window->width() / 2));
        assert_equal(window->height() / 2, p1.y);
    }

    void test_look_at() {       
        Vec3 pos(0, 0, -1);


        auto stage = window->stage(stage_id_);

        stage->host_camera(camera_id_);
        stage->camera(camera_id_)->look_at(pos);

        Quaternion q = stage->camera(camera_id_)->absolute_rotation();
        assert_true(kmQuaternionIsIdentity(&q));

        //Just double check that kazmath actually works
        Mat3 rot;
        kmMat3FromRotationQuaternion(&rot, &q);

        Quaternion other;
        kmQuaternionRotationMatrix(&other, &rot);

        assert_true(kmQuaternionAreEqual(&q, &other));

        pos = Vec3(0, -1, 0);
        stage->camera(camera_id_)->look_at(pos);

        assert_equal(Vec3(0, 0, -1), stage->camera(camera_id_)->up());

    }

    void test_following() {
        auto stage = window->stage(stage_id_);
        stage->host_camera(camera_id_);

        ActorID a = stage->new_actor();
        stage->actor(a)->move_to(Vec3());

        stage->camera(camera_id_)->follow(a, CAMERA_FOLLOW_MODE_DIRECT, Vec3(0, 0, 10));

        assert_equal(Vec3(0, 0, 10), stage->camera(camera_id_)->absolute_position());

        stage->actor(a)->rotate_to(Degrees(90), 0, -1, 0);
        stage->camera(camera_id_)->_update_following(1.0);

        auto cam_pos = stage->camera(camera_id_)->absolute_position();
        assert_close(-10, cam_pos.x, 0.0001);
        assert_close(0, cam_pos.y, 0.0001);
        assert_close(0, cam_pos.z, 0.0001);

        assert_equal(stage->actor(a)->absolute_rotation(), stage->camera(camera_id_)->absolute_rotation());
    }

private:
    CameraID camera_id_;
    StageID stage_id_;
};

}

#endif
