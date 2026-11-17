/* Research prototype linked with GPL-3.0-or-later eSpeak NG 1.52.0.
 * This wrapper is offered under GPL-3.0-or-later. */
#include <espeak-ng/speak_lib.h>
#include <string.h>
#include <stdlib.h>
static char output[65536];
int init(const char *path) { return espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, path, 0); }
const char *raw(const char *text, const char *accent) {
  if (espeak_SetVoiceByName(strcmp(accent, "en-gb") == 0 ? "gmw/en" : "gmw/en-US") != EE_OK) return NULL;
  const void *cursor = text;
  output[0] = 0;
  size_t used = 0;
  while (cursor) {
    const char *p = espeak_TextToPhonemes(&cursor, 1, 0x02 | (1 << 7) | (0x361 << 8));
    if (!p || !*p) continue;
    size_t n = strlen(p);
    if (used + n + 2 >= sizeof(output)) return NULL;
    if (used) output[used++] = ' ';
    memcpy(output + used, p, n + 1);
    used += n;
  }
  return output;
}
