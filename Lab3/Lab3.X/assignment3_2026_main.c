/*
 * File:   main.c
 * Author: ENTER GROUP MEMBER NAME(S) HERE
 *
 * Created FOR ENCM 511
 * PLEASE ADD DATE CREATED HERE: 2025-XX-XX
 * 
 * FAILURE TO UPDATE THIS HEADER WITH YOUR GROUP MEMBER NAMES
 * MAY RESULT IN PENALTIES
 */

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


#include "xc.h"
#include "uart.h"

// #define TESTSEND

/**
 * You might find it useful to add your own #defines to improve readability here
 */

uint16_t slow = 0;
char received;
uint16_t ascii = 49;

int main(void) {
    
    /** This is usually where you would add run-once code
     * e.g., peripheral initialization. For the first labs
     * you might be fine just having it here. For more complex
     * projects, you might consider having one or more initialize() functions
     */
    
    ANSELA = 0x0000; /* keep this line as it sets I/O pins that can also be analog to be digital */
    ANSELB = 0x0000; /* keep this line as it sets I/O pins that can also be analog to be digital */
    TRISBbits.TRISB5 = 0; // set RB5 as output
    _LATB5 = 1;
    
    TRISBbits.TRISB6 = 0; // rb6 as output
    LATBbits.LATB6 = 0;
    
    TRISAbits.TRISA4 = 1;
    TRISBbits.TRISB8 = 1;
    TRISBbits.TRISB9 = 1;
    
    InitUART2();
    InitTMR2();
    
    while(1) {
    
#ifdef TESTSEND
        // XmitUART2('1',5);
        // XmitUART2('\n',5);
        while(ascii <= 57){
            XmitUART2(ascii, 5);
            ascii++;
        }
        XmitUART2('\n', 5);
        ascii = 49;
        
        
        for (slow = 0; slow < 4000; slow++) {
            
        }
#endif
        
#ifndef TESTSEND
        Disp2String("Enter a character (hit enter to receive): ");
        received = RecvUartChar();
        Disp2String("\r\n you entered ");
        XmitUART2(received,1);
        XmitUART2('\r',1);
        XmitUART2('\n',1);
        
        
#endif
        
    }
    
    return 0;
}

void InitTMR2(void){
    
    T2CON = 0;                  // Reset Timer2 configuration
    TMR2 = 0;                   // Start counting from zero
    PR2 = 4999;                 // Interrupt every 10 ms

    T2CONbits.T32 = 0;          // 16-bit timer mode
    T2CONbits.TCS = 0;          // Internal clock (FCY)
    T2CONbits.TGATE = 0;        // Disable gated mode
    T2CONbits.TCKPS = 0b01;     // Prescaler = 1:8

    IPC1bits.T2IP = 3;          // Timer2 interrupt priority 3
    IFS0bits.T2IF = 0;          // Clear Timer2 interrupt flag
    IEC0bits.T2IE = 1;          // Enable Timer2 interrupt

    T2CONbits.TON = 1;          // Start Timer2

}

volatile uint16_t tickCount = 0;
volatile uint16_t selectedInterval = 0;

void __attribute__((interrupt, no_auto_psv))
_T2Interrupt(void)
{
    uint16_t newInterval = 0;

    // Detect button
    if (PORTAbits.RA4 == 0)
    {
        newInterval = 25;
    }
    else if (PORTBbits.RB8 == 0)
    {
        newInterval = 50;
    }
    else if (PORTBbits.RB9 == 0)
    {
        newInterval = 100;
    }

    // Restart timing whenever the button selection changes
    if (newInterval != selectedInterval)
    {
        selectedInterval = newInterval;
        tickCount = 0;

        // Start ON when a button is selected
        LATBbits.LATB6 = (newInterval != 0);
    }
    else if (selectedInterval != 0)
    {
        tickCount++;

        if (tickCount >= selectedInterval)
        {
            LATBbits.LATB6 ^= 1;
            tickCount = 0;
        }
    }

    IFS0bits.T2IF = 0;
}
