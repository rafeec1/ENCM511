// Debounce and gesture recognition
#include "buttons.h"
#include "app.h"

void Buttons_Init(Buttons *buttons)
{
    buttons->stable = 0;
    buttons->single = 0;
    buttons->chord = 0;
    buttons->consumed = 0;
    buttons->added_at = 0;
}

uint8_t Buttons_Update(Buttons *b, uint8_t raw,
                       uint16_t last_edge, uint16_t now)
{
    uint8_t old;
    uint8_t result;
    // Any edge restarts the 20 ms quiet period for the whole group
    if ((uint16_t)(now - last_edge) < DEBOUNCE_MS) return 0;
    if (raw != b->stable) {
        old = b->stable;
        b->stable = raw;
        if (b->consumed) {
            if (raw == 0u) {
                b->consumed = 0;
                b->single = 0;
                b->chord = 0;
            }
            return 0;
        }
        if (old == 0u && raw != 0u) {
            b->single = raw;
            b->chord = 0;
            b->added_at = now;
        }
        if ((raw & (uint8_t)~old) != 0u) b->added_at = now;

        // Remember an ACTUALLY simultaneous mask, not a union of all
        // buttons ever touched. A third button gets priority over a pair
        if (raw == ALL_PBS) b->chord = ALL_PBS;
        else if ((raw == 3u || raw == 5u || raw == 6u) && b->chord != ALL_PBS)
            b->chord = raw;

        if (raw == 0u) {
            result = b->chord ? b->chord : b->single;
            b->single = 0;
            b->chord = 0;
            return result;
        }
    }
    // Single buttons wait for release. Multi-button holds produce ONE
    // event after the chord window, then require ALL buttons released
    if (!b->consumed && b->chord != 0u &&
        (uint16_t)(now - b->added_at) >= CHORD_MS) {
        b->consumed = 1;
        return b->chord;
    }
    return 0;
}
