#include <memory>
#include <GL/gl.h>
#include <GL/glut.h>

#include "kos_window.h"
#include "sound_drivers/kos_sound_driver.h"
#include "renderers/renderer_config.h"
#include "utils/memory.h"

namespace smlt {

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_DEPTH 32

KOSWindow::KOSWindow(uint32_t width, uint32_t height, uint32_t bpp, bool fullscreen):
    WindowBase() {

    set_width(SCREEN_WIDTH);
    set_height(SCREEN_HEIGHT);
    set_bpp(SCREEN_DEPTH);
    set_fullscreen(true);
}


void KOSWindow::swap_buffers() {
    glutSwapBuffers();
}

bool KOSWindow::create_window(int width, int height, int bpp, bool fullscreen) {
    set_width(SCREEN_WIDTH);
    set_height(SCREEN_HEIGHT);

    L_DEBUG("Initializing OpenGL");
#ifdef _arch_dreamcast
        print_available_ram();
#endif

    glKosInit();
    L_DEBUG("OpenGL initialized");
#ifdef _arch_dreamcast
        print_available_ram();
#endif

    renderer_ = std::make_shared<GL1XRenderer>(this);

    set_has_context(true); //Mark that we have a valid GL context

    L_DEBUG("Renderer initialized");
#ifdef _arch_dreamcast
        print_available_ram();
#endif

    return true;
}

void KOSWindow::destroy_window() {

}

void KOSWindow::check_events() {

}

std::shared_ptr<SoundDriver> KOSWindow::create_sound_driver() {
    return std::make_shared<KOSSoundDriver>(this);
}

}
