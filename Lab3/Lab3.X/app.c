/* ENCM 511 Assignment 3, first draft, 2026-10-13.
 * Group members: (insert names and UCIDs)
 * Hardware-independent state machine. All functions run in main, never an ISR.
 * Unspecified cases: different single buttons are ignored while active;
 * two buttons override any fast state, but do nothing in prog;
 * three buttons change mode only from idle. See README.md.
 */
#include "app.h"

void App_Init(App *app)
{
    app->mode = MODE_FAST;
    app->state = STATE_IDLE;
    app->saved_setting = 0;
    app->edit_setting = 0;
    app->pair_mask = 0;
}

uint8_t App_ButtonEvent(App *app, uint8_t mask)
{
    State selected;
    if (mask == ALL_PBS) {
        if (app->state != STATE_IDLE) return 0;
        app->mode = (app->mode == MODE_FAST) ? MODE_PROG : MODE_FAST;
        return 1;
    }
    if (mask == 3u || mask == 5u || mask == 6u) {
        if (app->mode != MODE_FAST) return 0;
        if (app->state == STATE_TWO_BUTTONS && app->pair_mask == mask) return 0;
        app->pair_mask = mask;
        app->state = STATE_TWO_BUTTONS;
        return 1;
    }
    if (mask == PB0_MASK) selected = STATE_PB0;
    else if (mask == PB1_MASK) selected = STATE_PB1;
    else if (mask == PB2_MASK) selected = STATE_PB2;
    else return 0;

    if (app->state == STATE_TWO_BUTTONS) {
        app->state = STATE_IDLE;
        return 1;
    }
    if (app->state == selected) {
        if (app->mode == MODE_PROG && selected == STATE_PB2)
            app->saved_setting = app->edit_setting;
        app->state = STATE_IDLE;
        return 1;
    }
    if (app->state != STATE_IDLE) return 0;
    app->state = selected;
    if (app->mode == MODE_PROG && selected == STATE_PB2)
        app->edit_setting = app->saved_setting;
    return 1;
}

uint8_t App_Character(App *app, char c)
{
    uint8_t value;
    if (app->mode != MODE_PROG || app->state != STATE_PB2) return 0;
    if (c < '0' || c > '2') return 0;
    value = (uint8_t)(c - '0');
    if (value == app->edit_setting) return 0;
    app->edit_setting = value;
    return 1;
}

uint16_t App_BlinkInterval(const App *app)
{
    static const uint16_t fast_ms[3] = {250u, 500u, 1000u};
    if (app->state == STATE_IDLE || app->state == STATE_TWO_BUTTONS) return 0;
    if (app->mode == MODE_FAST) {
        if (app->state == STATE_PB0) return fast_ms[0];
        if (app->state == STATE_PB1) return fast_ms[1];
        return fast_ms[2];
    }
    if (app->state == STATE_PB0) return 3000u;
    if (app->state == STATE_PB1) return fast_ms[app->saved_setting];
    return 125u;
}

uint8_t App_SteadyOn(const App *app)
{
    return app->mode == MODE_FAST && app->state == STATE_TWO_BUTTONS;
}

/* Tiny string builder: no printf, strlen, or string library. Known messages
 * are shorter than 64 characters including the terminator. */
static char *Append(char *out, const char *in)
{
    while (*in != '\0') *out++ = *in++;
    return out;
}

void App_Status(const App *app, char text[64])
{
    char *p = text;
    p = Append(p, app->mode == MODE_FAST ? "Fast Mode: " : "Prog Mode: ");
    if (app->state == STATE_IDLE) p = Append(p, "IDLE");
    else if (app->state == STATE_TWO_BUTTONS) {
        p = Append(p, "PB");
        *p++ = (app->pair_mask == 6u) ? '1' : '0';
        p = Append(p, " and PB");
        *p++ = (app->pair_mask == 3u) ? '1' : '2';
        p = Append(p, " are pressed");
    } else if (app->mode == MODE_PROG && app->state == STATE_PB2) {
        p = Append(p, "Blink setting = ");
        *p++ = (char)('0' + app->edit_setting);
    } else {
        p = Append(p, "PB");
        *p++ = app->state == STATE_PB0 ? '0' :
               app->state == STATE_PB1 ? '1' : '2';
        p = Append(p, " was pressed");
        if (app->mode == MODE_PROG && app->state == STATE_PB1) {
            p = Append(p, ", Setting = ");
            *p++ = (char)('0' + app->saved_setting);
        }
    }
    *p = '\0';
}
