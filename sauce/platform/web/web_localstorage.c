#include "web_localstorage.h"

#include <emscripten.h>

EM_JS(void, web_ls_save, (const char *key, const void *data, int length), {
    var bytes = new Uint8Array(Module.HEAPU8.buffer, data, length);
    var binary = "";
    for (var i = 0; i < bytes.length; i++) {
        binary += String.fromCharCode(bytes[i]);
    }
    try {
        localStorage.setItem(UTF8ToString(key), btoa(binary));
    } catch (e) {
        /* Storage full/unavailable (private browsing etc.) - persistence is
           a nice-to-have, not required for the game to keep running. */
    }
});

EM_JS(int, web_ls_load, (const char *key, void *dst, int maxLen), {
    var raw;
    try {
        raw = localStorage.getItem(UTF8ToString(key));
    } catch (e) {
        return -1;
    }
    if (raw === null) return -1;
    var binary = atob(raw);
    var length = Math.min(binary.length, maxLen);
    var heap = new Uint8Array(Module.HEAPU8.buffer, dst, length);
    for (var i = 0; i < length; i++) {
        heap[i] = binary.charCodeAt(i);
    }
    return binary.length;
});

EM_JS(void, web_ls_remove, (const char *key), {
    try {
        localStorage.removeItem(UTF8ToString(key));
    } catch (e) {
    }
});
