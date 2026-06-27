#include <emscripten.h>
#include <stdlib.h>
#include <time.h>

#include "../../constants.h"
#include "../../deviceinfo.h"
#include "../../game/colors.h"
#include "../../game/game.h"
#include "../../game/gamesession.h"
#include "../i_device.h"

#include "web_framebuffer.h"
#include "web_input.h"
#include "web_text.h"

/* Expands the RGB565 display buffer to RGBA8888 and blits it onto the page's
   pixel <canvas> via putImageData. Runs once per frame from web_frame(). */
EM_JS(void, web_present, (const unsigned short *pixels, int width, int height), {
    var ctx = Module.pixelCtx;
    if (!ctx) return;
    var imageData = Module.pixelImageData;
    if (!imageData || imageData.width !== width || imageData.height !== height) {
        imageData = ctx.createImageData(width, height);
        Module.pixelImageData = imageData;
    }
    var src = new Uint16Array(Module.HEAPU8.buffer, pixels, width * height);
    var dst = imageData.data;
    for (var i = 0; i < src.length; i++) {
        var p = src[i];
        var r = (p >> 11) & 0x1F;
        var g = (p >> 5) & 0x3F;
        var b = p & 0x1F;
        var o = i * 4;
        dst[o] = (r * 255 / 31) | 0;
        dst[o + 1] = (g * 255 / 63) | 0;
        dst[o + 2] = (b * 255 / 31) | 0;
        dst[o + 3] = 255;
    }
    ctx.putImageData(imageData, 0, 0);
});

/* Clears the transparent text overlay canvas before this frame's draws. */
EM_JS(void, web_beginFrame, (int width, int height), {
    var ctx = Module.overlayCtx;
    if (ctx) ctx.clearRect(0, 0, width, height);
});

static void web_frame(void) {
    WebRawEvent rawEvent;
    Coordinate screenSize = idevice_screenSize();

    web_beginFrame(screenSize.x, screenSize.y);
    web_textClearAll();

    if (!web_input_pop(&rawEvent)) {
        rawEvent.type = WEB_RAW_NIL;
        rawEvent.x = 0;
        rawEvent.y = 0;
        rawEvent.id = 0;
    }
    game_mainLoop(&rawEvent, NULL);

    if (web_displayBuffer != NULL) {
        web_present(web_displayBuffer->pixels, web_displayBuffer->width, web_displayBuffer->height);
        web_textFlush(web_displayBuffer);
    }
}

EMSCRIPTEN_KEEPALIVE
void web_start(void) {
    srand((unsigned)time(NULL));
    colors_setupReferenceColors(deviceinfo_colorSupported(), deviceinfo_currentDepth());
    game_setup();
    emscripten_set_main_loop(web_frame, 0, 1);
}

int main(void) {
    /* Real startup is driven from JS (web/index.html), once it has measured
       the canvas and called web_setScreenSize + web_start - the hex grid's
       zoom needs the actual device screen size before game_setup() runs. */
    return 0;
}
