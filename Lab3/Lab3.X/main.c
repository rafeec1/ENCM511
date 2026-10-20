// ENCM 511 Assignment 3, Part 2, FIRST DRAFT
// Group members: Arslan Sheriff Amjath (30181510), Sudeep Mann (30173100), 
//                Rafee Chowdhury (30244011). Date: 2026-10-13
// Target: PIC24FJ256GA702, XC16, 8 MHz FRC (FCY = 4 MHz).
// PB0=RA4, PB1=RB8, PB2=RB9 (active low); LED0=RB5 (active high).
// Configuration pragmas below are retained from the provided starter.
// See README.md for wiring, design decisions and first hardware checks.
//

// FSEC
#pragma config BWRP = OFF    //Boot Segment Write-Protect bit->Boot Segment may be written
#pragma config BSS = DISABLED    //Boot Segment Code-Protect Level bits->No Protection (other than BWRP)
#pragma config BSEN = OFF    //Boot Segment Control bit->No Boot Segment
#pragma config GWRP = OFF    //General Segment Write-Protect bit->General Segment may be written
#pragma config GSS = DISABLED    //General Segment Code-Protect Level bits->No Protection (other than GWRP)
#pragma config CWRP = OFF    //Configuration Segment Write-Protect bit->Configuration Segment may be written
#pragma config CSS = DISABLED    //Configuration Segment Code-Protect Level bits->No Protection (other than CWRP)
#pragma config AIVTDIS = OFF    //Alternate Interrupt Vector Table bit->Disabled AIVT

// FBSLIM
#pragma config BSLIM = 8191    //Boot Segment Flash Page Address Limit bits->8191

// FOSCSEL
#pragma config FNOSC = FRC    //Oscillator Source Selection->Internal Fast RC (FRC)
#pragma config PLLMODE = PLL96DIV2    //PLL Mode Selection->96 MHz PLL. Oscillator input is divided by 2 (8 MHz input)
#pragma config IESO = OFF    //Two-speed Oscillator Start-up Enable bit->Start up with user-selected oscillator source

// FOSC
#pragma config POSCMD = NONE    //Primary Oscillator Mode Select bits->Primary Oscillator disabled
#pragma config OSCIOFCN = ON    //OSC2 Pin Function bit->OSC2 is general purpose digital I/O pin
#pragma config SOSCSEL = OFF    //SOSC Power Selection Configuration bits->Digital (SCLKI) mode
#pragma config PLLSS = PLL_FRC    //PLL Secondary Selection Configuration bit->PLL is fed by the on-chip Fast RC (FRC) oscillator
#pragma config IOL1WAY = ON    //Peripheral pin select configuration bit->Allow only one reconfiguration
#pragma config FCKSM = CSECMD    //Clock Switching Mode bits->Clock switching is enabled,Fail-safe Clock Monitor is disabled

// FWDT
#pragma config WDTPS = PS32768    //Watchdog Timer Postscaler bits->1:32768
#pragma config FWPSA = PR128    //Watchdog Timer Prescaler bit->1:128
#pragma config FWDTEN = ON_SWDTEN    //Watchdog Timer Enable bits->WDT Enabled/Disabled (controlled using SWDTEN bit)
#pragma config WINDIS = OFF    //Watchdog Timer Window Enable bit->Watchdog Timer in Non-Window mode
#pragma config WDTWIN = WIN25    //Watchdog Timer Window Select bits->WDT Window is 25% of WDT period
#pragma config WDTCMX = WDTCLK    //WDT MUX Source Select bits->WDT clock source is determined by the WDTCLK Configuration bits
#pragma config WDTCLK = LPRC    //WDT Clock Source Select bits->WDT uses LPRC

// FPOR
#pragma config BOREN = ON    //Brown Out Enable bit->Brown Out Enable Bit
#pragma config LPCFG = OFF    //Low power regulator control->No Retention Sleep
#pragma config DNVPEN = ENABLE    //Downside Voltage Protection Enable bit->Downside protection enabled using ZPBOR when BOR is inactive

// FICD
#pragma config ICS = PGD1    //ICD Communication Channel Select bits->Communicate on PGEC1 and PGED1
#pragma config JTAGEN = OFF    //JTAG Enable bit->JTAG is disabled

// FDEVOPT1
#pragma config ALTCMPI = DISABLE    //Alternate Comparator Input Enable bit->C1INC, C2INC, and C3INC are on their standard pin locations
#pragma config TMPRPIN = OFF    //Tamper Pin Enable bit->TMPRN pin function is disabled
#pragma config SOSCHP = ON    //SOSC High Power Enable bit (valid only when SOSCSEL = 1->Enable SOSC high power mode (default)
#pragma config ALTI2C1 = ALTI2CEN    //Alternate I2C pin Location->SDA1 and SCL1 on RB9 and RB8


#include <xc.h>
#include <stdint.h>
#include "app.h"
#include "buttons.h"
#include "uart.h"

#define FCY_HZ 4000000UL
#define LED_ON  1u
#define LED_OFF 0u

// ISR-shared 16-bit values are volatile and naturally aligned. The related
// button snapshot fields are copied atomically together in main
static volatile uint16_t milliseconds;
static volatile uint16_t last_button_edge;
static volatile uint16_t raw_buttons;
static volatile uint16_t blink_interval;
static volatile uint16_t blink_remaining;

static void Board_Init(void);
static void ApplyLED(const App *app);

int main(void)
{
    App app;
    Buttons buttons;
    char line[64];
    char received;
    uint8_t event, dirty = 1;
    uint16_t now, raw, edge, n;

    App_Init(&app);
    Buttons_Init(&buttons);
    Board_Init();
    InitUART2();
    ApplyLED(&app);

    while(1) {
        // Drain only already-received RAM bytes, with a fixed upper bound
        // RX is processed before a PB2 save, so pending digits are saved
        // Outside editing, received characters are consumed and ignored
        for (n = 0; n < 31u; ++n) {
            if (!UART2_ReadChar(&received)) break;
            if (App_Character(&app, received)) dirty = 1;
        }

        // Compiler intrinsic emits DISI instructions, not a timed delay
        // All enabled ISRs have priority 4, so this protects the snapshot
        // Only these three short loads execute with interrupts masked
        __builtin_disi(0x3FFF);
        now = milliseconds;
        raw = raw_buttons;
        edge = last_button_edge;
        __builtin_disi(0);

        event = Buttons_Update(&buttons, (uint8_t)raw, edge, now);
        if (event != 0u && App_ButtonEvent(&app, event)) {
            ApplyLED(&app);
            dirty = 1;
        }
        if (dirty) {
            App_Status(&app, line);
            if (UART2_TryLine(line)) dirty = 0;
        }

        // Idle leaves Timer1 and UART running. An interrupt wakes the CPU
        // If an interrupt lands just before this instruction, the next 1 ms
        // tick still wakes us. work can be delayed <=1 tick, never stranded
        Idle();
    }
}

static void Board_Init(void)
{
    uint16_t a, b;
    RCONbits.SWDTEN = 0; // Disable software-controlled watchdog
    CLKDIVbits.DOZEN = 0;
    ANSELA = 0;
    ANSELB = 0;

    LATBbits.LATB5 = LED_OFF;
    TRISBbits.TRISB5 = 0;
    TRISAbits.TRISA4 = 1;
    TRISBbits.TRISB8 = 1;
    TRISBbits.TRISB9 = 1;

    IEC1bits.IOCIE = 0;
    PADCONbits.IOCON = 0;
    IOCPAbits.IOCPA4 = 1; // Rising edge: PB0 release
    IOCNAbits.IOCNA4 = 1; // Falling edge: PB0 press
    IOCPBbits.IOCPB8 = 1; // Both edges for PB1/PB2
    IOCPBbits.IOCPB9 = 1;
    IOCNBbits.IOCNB8 = 1;
    IOCNBbits.IOCNB9 = 1;

    IOCPUAbits.IOCPUA4 = 1; // Pull up config
    IOCPUBbits.IOCPUB8 = 1;
    IOCPUBbits.IOCPUB9 = 1;
    
    IOCFAbits.IOCFA4 = 0;   // reset flags to 0
    IOCFBbits.IOCFB8 = 0;
    IOCFBbits.IOCFB9 = 0;
    IFS1bits.IOCIF = 0;
    IPC4bits.IOCIP = 4;
    PADCONbits.IOCON = 1;

    // One startup read all subsequent input-port reads are in the IOC ISR
    // Buttons connect to ground, so a LOW input means PRESSED
    a = PORTA;
    b = PORTB;
    raw_buttons = ((a & 0x0010u) ? 0u : PB0_MASK)
                | ((b & 0x0100u) ? 0u : PB1_MASK)
                | ((b & 0x0200u) ? 0u : PB2_MASK);
    last_button_edge = 0;
    IEC1bits.IOCIE = 1;

    T1CON = 0;          // Internal FCY clock, 1:1 prescale, run in Idle
    TMR1 = 0;
    PR1 = (uint16_t)(FCY_HZ / 1000UL - 1UL); // 3999 => 1 ms
    IFS0bits.T1IF = 0;
    IPC0bits.T1IP = 4;
    IEC0bits.T1IE = 1;
    T1CONbits.TON = 1;
}

static void ApplyLED(const App *app)
{
    uint16_t interval = App_BlinkInterval(app);
    uint8_t on = (uint8_t)(interval != 0u || App_SteadyOn(app));
    // Update both countdown values and LED atomically with respect to T1
    IEC0bits.T1IE = 0;
    blink_interval = interval;
    blink_remaining = interval;
    LATBbits.LATB5 = on ? LED_ON : LED_OFF;
    IEC0bits.T1IE = 1;
}

void __attribute__((interrupt, no_auto_psv)) _IOCInterrupt(void)
{
    uint16_t a, b, flags_a, flags_b;
    IFS1bits.IOCIF = 0;
    flags_a = IOCFA;
    flags_b = IOCFB;
    // Clear per-pin flags using read-modify-write, DS30010118E section 11.3
    // IOCSTAT port summary bits clear automatically when IOCFx is cleared
    IOCFA &= (uint16_t)~flags_a;
    IOCFB &= (uint16_t)~flags_b;
    a = PORTA;
    b = PORTB;
    raw_buttons = ((a & 0x0010u) ? 0u : PB0_MASK)
                | ((b & 0x0100u) ? 0u : PB1_MASK)
                | ((b & 0x0200u) ? 0u : PB2_MASK);
    last_button_edge = milliseconds;
    // No calls, delays, state transitions, or UART output in this ISR
}

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void)
{
    IFS0bits.T1IF = 0;
    ++milliseconds;
    if (blink_interval != 0u) {
        if (blink_remaining > 0u) --blink_remaining;
        if (blink_remaining == 0u) {
            LATBbits.LATB5 ^= 1;
            blink_remaining = blink_interval;
        }
    }
}
