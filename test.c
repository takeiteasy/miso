#include "miso.h"

static void init(void) {
    
}

static void frame(void) {
    
}

static void event(const sapp_event *event) {
    
}

static void cleanup(void) {
    
}

int main(int argc, const char *argv[]) {
    sapp_desc desc = {
        .width = 640,
        .height = 480,
        .window_title = "miso!",
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = cleanup
    };
    return RunMiso(&desc);
}
