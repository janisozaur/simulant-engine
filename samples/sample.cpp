#include "simulant/simulant.h"

using namespace smlt;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(Window* window):
        smlt::Scene<GameScene>(window) {}

    void load() {
        auto pipeline = prepare_basic_scene(stage_, camera_, smlt::PARTITIONER_NULL);
        pipeline->viewport->set_colour(smlt::Colour::SKY_BLUE);

        camera_->set_perspective_projection(
            Degrees(45.0),
            float(window->width()) / float(window->height()),
            1.0,
            1000.0
        );

        stage_->new_light_as_directional(smlt::Vec3(1, -1, 0));
        stage_->new_light_as_directional(smlt::Vec3(-1, 0, 0), smlt::Colour::RED);
        stage_->set_ambient_light(smlt::Colour(0.3, 0.3, 0.3, 1.0));

        // Load an animated MD2 mesh
        smlt::MeshID mesh_id = stage_->assets->new_mesh_from_file("sample_data/ogro.md2");

        auto actor = stage_->new_actor_with_mesh(mesh_id); // Create an instance of it
        actor->move_to(0.0f, 0.0f, -80.0f);
        actor->rotate_global_y_by(smlt::Degrees(180));

        auto actor3 = stage_->new_actor_with_mesh(mesh_id);
        actor3->move_to(-40.0f, 0.0f, -95.0f);
        actor3->rotate_global_y_by(smlt::Degrees(180));
        actor3->animation_state->play_animation("idle_2");

        // Add a fly controller to the camera for user input
        camera_->new_behaviour<behaviours::Fly>(window);
    }

private:
    CameraPtr camera_;
    StagePtr stage_;
};

class Sample: public smlt::Application {
public:
    Sample(const AppConfig& config):
        Application(config) {
    }

private:
    bool init() {
        scenes->register_scene<GameScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "Basic Sample";
    config.fullscreen = false;
    config.width = 1280;
    config.height = 960;

    Sample app(config);
    return app.run();
}
