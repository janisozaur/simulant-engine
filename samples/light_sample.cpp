#include "simulant/simulant.h"
#include "simulant/shortcuts.h"
#include "simulant/extra.h"

using namespace smlt;
using namespace smlt::extra;

class GameScreen : public smlt::Screen<GameScreen> {
public:
    GameScreen(WindowBase& window):
        smlt::Screen<GameScreen>(window, "game_screen") {}

    void do_load() {
        prepare_basic_scene(stage_id_, camera_id_);
        auto stage = window->stage(stage_id_);
        stage->host_camera(camera_id_);

        window->camera(camera_id_)->set_perspective_projection(
            45.0,
            float(window->width()) / float(window->height()),
            0.1,
            1000.0
        );

        stage->set_ambient_light(smlt::Colour(0.2, 0.2, 0.2, 1.0));

        actor_id_ = stage->new_actor_with_mesh(stage->assets->new_mesh_as_cube(2.0));
        stage->actor(actor_id_)->move_to(0.0, 0.0, -10.0);

        smlt::TextureID texture = stage->assets->new_texture_from_file("sample_data/crate.png");
        stage->actor(actor_id_)->mesh()->set_texture_on_material(0, texture);

        // Test Camera::look_at function
        stage->camera(camera_id_)->look_at(stage->actor(actor_id_)->absolute_position());

        {
            auto light = stage->light(stage->new_light());
            light->move_to(5.0, 0.0, -5.0);
            light->set_diffuse(smlt::Colour::GREEN);
            light->set_attenuation_from_range(20.0);

            auto light2 = stage->light(stage->new_light());
            light2->move_to(-5.0, 0.0, -5.0);
            light2->set_diffuse(smlt::Colour::BLUE);
            light2->set_attenuation_from_range(30.0);

            auto light3 = stage->light(stage->new_light());
            light3->move_to(0.0, 15.0, -5.0);
            light3->set_diffuse(smlt::Colour::RED);
            light3->set_attenuation_from_range(50.0);
        }

        float xpos = 0;
        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_A, [&](SDL_Keysym key, double dt) mutable {
                xpos -= 20.0 * dt;
                window->stage(stage_id_)->camera(camera_id_)->move_to_absolute(xpos, 2, 0);
                window->stage(stage_id_)->camera(camera_id_)->look_at(window->stage(stage_id_)->actor(actor_id_)->absolute_position());
        });
        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_D, [&](SDL_Keysym key, double dt) mutable {
                xpos += 20.0 * dt;
                window->stage(stage_id_)->camera(camera_id_)->move_to_absolute(xpos, 2, 0);
                window->stage(stage_id_)->camera(camera_id_)->look_at(window->stage(stage_id_)->actor(actor_id_)->absolute_position());
        });
    }

    void do_step(double dt) {
        window->stage(stage_id_)->actor(actor_id_)->rotate_x_by(smlt::Degrees(dt * 20.0));
        window->stage(stage_id_)->actor(actor_id_)->rotate_y_by(smlt::Degrees(dt * 15.0));
        window->stage(stage_id_)->actor(actor_id_)->rotate_z_by(smlt::Degrees(dt * 25.0));
    }

private:
    CameraID camera_id_;
    StageID stage_id_;
    ActorID actor_id_;
};

class LightingSample: public smlt::Application {
public:
    LightingSample():
        Application("Simulant Light Sample") {
    }

private:
    bool do_init() {
        register_screen("/", screen_factory<GameScreen>());
        return true;
    }
};

int main(int argc, char* argv[]) {
    LightingSample app;
    return app.run();
}

