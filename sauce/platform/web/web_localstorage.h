#ifndef WEB_LOCALSTORAGE_H_
#define WEB_LOCALSTORAGE_H_

/* Thin C wrapper around the browser's localStorage, used to persist score and
   save-state data across page loads without any backend service - the whole
   game still runs from static files only. Binary blobs are base64-encoded by
   the JS side (see web_localstorage.c). */

void web_ls_save(const char *key, const void *data, int length);
/* Copies up to maxLen bytes into dst, returns the actual stored length, or -1
   if the key doesn't exist. */
int web_ls_load(const char *key, void *dst, int maxLen);
void web_ls_remove(const char *key);

#endif
