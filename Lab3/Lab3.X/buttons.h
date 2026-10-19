#ifndef BUTTONS_H
#define BUTTONS_H
#include <stdint.h>

#define DEBOUNCE_MS 20u
#define CHORD_MS    80u

typedef struct {
    uint8_t stable;
    uint8_t single;
    uint8_t chord;
    uint8_t consumed;
    uint16_t added_at;
} Buttons;

void Buttons_Init(Buttons *buttons);
/* raw is an IOC-captured RAM snapshot, NEVER a live PORT read.
 * last_edge is the time of the latest IOC interrupt.
 * Returns 0 for no event, else PB mask 1..7. */
uint8_t Buttons_Update(Buttons *buttons, uint8_t raw,
                       uint16_t last_edge, uint16_t now);
#endif
