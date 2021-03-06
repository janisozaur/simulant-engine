#pragma once

#include <kaztest/kaztest.h>
#include "../simulant/simulant.h"
#include "global.h"

namespace {

using namespace smlt;

class WidgetTest : public SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        stage_ = window->new_stage();
    }

    void tear_down() {
        window->delete_stage(stage_->id());
        SimulantTestCase::tear_down();
    }

    void test_button_creation() {
        auto button = stage_->ui->new_widget_as_button("Test", 100, 20);

        assert_equal(_u("Test"), button->text());
        assert_equal(100, button->requested_width());
        assert_equal(20, button->requested_height());
    }

    void test_focus_chain() {
        auto widget1 = stage_->ui->new_widget_as_label("label1");
        auto widget2 = stage_->ui->new_widget_as_label("label2");

        assert_is_null((ui::Widget*) widget1->focused_in_chain());

        widget1->set_focus_next(widget2);
        widget1->focus();

        assert_equal(widget1, widget1->focused_in_chain());
        widget1->focus_next_in_chain();

        assert_equal(widget2, widget2->focused_in_chain());

        widget2->blur();

        assert_is_null((ui::Widget*) widget1->focused_in_chain());
    }

private:
    StagePtr stage_;
};


class ImageTests : public SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        stage_ = window->new_stage();
    }

    void tear_down() {
        window->delete_stage(stage_->id());
        SimulantTestCase::tear_down();
    }

    void test_image_creation() {
        auto texture = stage_->assets->new_texture_from_file("../assets/textures/simulant-icon.png").fetch();
        auto image = stage_->ui->new_widget_as_image(texture->id());

        assert_equal(image->width(), texture->width());
        assert_equal(image->height(), texture->height());
        assert_true(image->has_background_image());
        assert_false(image->has_foreground_image());
        assert_equal(image->resize_mode(), smlt::ui::RESIZE_MODE_FIXED);
    }

    void test_set_source_rect() {
        auto texture = stage_->assets->new_texture_from_file("../assets/textures/simulant-icon.png").fetch();
        auto image = stage_->ui->new_widget_as_image(texture->id());

        image->set_source_rect(smlt::Vec2(0, 0), smlt::Vec2(128, 128));

        assert_equal(image->width(), 128);
        assert_equal(image->height(), 128);
    }

private:
    StagePtr stage_;

};

}
