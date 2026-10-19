/* ENCM 511 Assignment 3, replacement for the supplied blocking uart.c.
 * Group members: (insert names and UCIDs). Date: 2026-10-13.
 * Same UART2 pins and baud setting, new interrupt-driven RX/TX queues.
 * No function calls, delay loops, or unbounded loops inside ISRs.
 */
#include <xc.h>
#include "uart.h"

#define TX_SIZE 128u
#define RX_SIZE 32u
static volatile uint8_t tx[TX_SIZE];
static volatile uint8_t rx[RX_SIZE];
static volatile uint16_t tx_head, tx_tail;
static volatile uint16_t rx_head, rx_tail;

void InitUART2(void)
{
    IEC1bits.U2RXIE = 0;
    IEC1bits.U2TXIE = 0;
    U2MODE = 0;
    U2STA = 0;
    tx_head = tx_tail = 0;
    rx_head = rx_tail = 0;

    LATBbits.LATB10 = 1; /* UART TX is idle high before PPS takes control. */
    TRISBbits.TRISB10 = 0;
    TRISBbits.TRISB11 = 1;
    /* Explicit PPS unlock/lock. This is the only PPS configuration, which
     * respects the starter's IOL1WAY = ON configuration. */
    __builtin_write_OSCCONL(OSCCON & 0xBFu);
    RPINR19bits.U2RXR = 11; /* RP11/RB11, physical pin 22, adapter TX. */
    RPOR5bits.RP10R = 5;    /* RP10/RB10, physical pin 21, adapter RX. */
    __builtin_write_OSCCONL(OSCCON | 0x40u);

    U2MODEbits.BRGH = 1;
    U2MODEbits.USIDL = 0;   /* Continue UART operation during CPU Idle. */
    U2BRG = 103;           /* 4 MHz / (4*(103+1)) = 9615.38 baud. */
    /* Zero PDSEL/STSEL: 8 data bits, no parity, 1 stop bit. */
    U2STAbits.URXISEL = 0; /* RX interrupt after every received character. */
    U2STAbits.UTXISEL0 = 0;
    U2STAbits.UTXISEL1 = 0; /* TX interrupt as a byte moves into shifter. */
    IPC7bits.U2RXIP = 4;
    IPC7bits.U2TXIP = 4;
    IFS1bits.U2RXIF = 0;
    IFS1bits.U2TXIF = 0;
    U2MODEbits.UARTEN = 1;
    U2STAbits.URXEN = 1;
    U2STAbits.UTXEN = 1;
    IEC1bits.U2RXIE = 1;
    /* TX interrupt is enabled only when software has something to send. */
}

uint8_t UART2_ReadChar(char *c)
{
    if (rx_tail == rx_head) return 0;
    *c = (char)rx[rx_tail];
    rx_tail = (rx_tail + 1u) & (RX_SIZE - 1u);
    return 1;
}

uint8_t UART2_TryLine(const char *text)
{
    uint16_t len, used, head, i;
    /* Bounded RAM scan, not hardware polling or a timing delay. */
    for (len = 0; len < 64u && text[len] != '\0'; ++len) { }
    if (len == 64u) return 0;
    head = tx_head;
    used = (head - tx_tail) & (TX_SIZE - 1u);
    if (TX_SIZE - 1u - used < len + 5u) return 0;

    /* Do not publish head until this entire line has been copied. The ISR
     * can drain older data meanwhile, but cannot read unpublished bytes. */
    tx[head] = '\r'; head = (head + 1u) & (TX_SIZE - 1u);
    tx[head] = 0x1B; head = (head + 1u) & (TX_SIZE - 1u);
    tx[head] = '[';  head = (head + 1u) & (TX_SIZE - 1u);
    tx[head] = '2';  head = (head + 1u) & (TX_SIZE - 1u);
    tx[head] = 'K';  head = (head + 1u) & (TX_SIZE - 1u);
    for (i = 0; i < len; ++i) {
        tx[head] = (uint8_t)text[i];
        head = (head + 1u) & (TX_SIZE - 1u);
    }
    tx_head = head;
    /* Disabling briefly avoids racing the ISR's empty-queue disable. */
    IEC1bits.U2TXIE = 0;
    IFS1bits.U2TXIF = 1;  /* Kick-start without a blocking first-byte wait. */
    IEC1bits.U2TXIE = 1;
    return 1;
}

void __attribute__((interrupt, no_auto_psv)) _U2RXInterrupt(void)
{
    uint16_t i, value, next;
    uint8_t bad;
    IFS1bits.U2RXIF = 0;
    /* Four-entry hardware RX FIFO. At most four immediate reads per ISR.
     * This never waits for a new byte to arrive. */
    for (i = 0; i < 4u; ++i) {
        if (!U2STAbits.URXDA) break;
        bad = (uint8_t)(U2STAbits.FERR | U2STAbits.PERR);
        value = U2RXREG;
        next = (rx_head + 1u) & (RX_SIZE - 1u);
        if (!bad && next != rx_tail) {
            rx[rx_head] = (uint8_t)value;
            rx_head = next;
        }
        /* On software queue overflow, discard newest byte, preserve order. */
    }
    if (U2STAbits.OERR) U2STAbits.OERR = 0;
    if (U2STAbits.URXDA) IFS1bits.U2RXIF = 1;
}

void __attribute__((interrupt, no_auto_psv)) _U2TXInterrupt(void)
{
    IFS1bits.U2TXIF = 0;
    if (tx_tail == tx_head) {
        IEC1bits.U2TXIE = 0;
    } else if (!U2STAbits.UTXBF) {
        U2TXREG = tx[tx_tail];
        tx_tail = (tx_tail + 1u) & (TX_SIZE - 1u);
    }
    /* TX stays enabled; no wait for TRMT and no LED manipulation here. */
}
