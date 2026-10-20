#ifndef APP_H
#define APP_H
#include <stdint.h>

#define PB0_MASK 1u
#define PB1_MASK 2u
#define PB2_MASK 4u
#define ALL_PBS  7u

typedef enum { MODE_FAST, MODE_PROG } Mode;
typedef enum { STATE_IDLE, STATE_PB0, STATE_PB1, STATE_PB2,
               STATE_TWO_BUTTONS } State;
typedef struct {
    Mode mode;
    State state;
    uint8_t saved_setting;
    uint8_t edit_setting;
    uint8_t pair_mask;
} App;

void App_Init(App *app);
// One completed single click OR one recognized multi-button gesture
// Returns 1 only if state/output has changed
uint8_t App_ButtonEvent(App *app, uint8_t mask);
uint8_t App_Character(App *app, char c);
uint16_t App_BlinkInterval(const App *app); // milliseconds, 0 = no blink
uint8_t App_SteadyOn(const App *app);
void App_Status(const App *app, char text[64]);
#endif
