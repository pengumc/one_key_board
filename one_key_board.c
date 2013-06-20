//MCU = atmega88
//F_CPU = 20000000 defined in makefile
//hfuse: DF 
//lfuse: E6
//efuse: 0


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "usbdrv.h"
#include "oddebug.h"

#define SET(x,y) (x|=(1<<y))
#define CLR(x,y) (x&=(~(1<<y)))
#define CHK(x,y) (x&(1<<y)) 
#define TOG(x,y) (x^=(1<<y))

static uchar    reportBuffer[1];    /* buffer for HID reports */
static uchar    idleRate;           /* in 4 ms units */


const PROGMEM char usbHidReportDescriptor[35] = {   /* USB report descriptor */
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0xe0,                    //   USAGE (Volume)
    0x15, 0xff,                    //   LOGICAL_MINIMUM (-1)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x02,                    //   REPORT_SIZE (2)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x06,                    //   INPUT (Data,Var,Rel)
    0x09, 0xe2,                    //   USAGE (Mute)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x06,                    //   INPUT (Data,Var,Rel)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x81, 0x07,                    //   INPUT (Cnst,Var,Rel)
    0xc0                           // END_COLLECTION
};

 
 uchar	usbFunctionSetup(uchar data[8]){
    usbRequest_t    *rq = (void *)data;
    TOG(PORTB, PB0);

    usbMsgPtr = reportBuffer;
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
            return sizeof(reportBuffer);
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];
        }
    }else{
        /* no vendor specific requests implemented */
    }
	return 0;
}


uchar keydown = 0;
uchar prev = 0;
uchar changed = 0;
int main(){
    usbInit();
    sei();
    reportBuffer[0] = 0x00;//(1<<1);
    CLR(DDRB, PB0);
    SET(DDRD, PD7);
    TCCR0B = 5; //prescale TC0 1024, 20 MHz -> 50 ns * 1024 * 255 = 13.056 ms
    while(1){
        usbPoll();
        if(CHK(TIFR0, TOV0)){
            TIFR0 = 1<<TOV0;
            if(CHK(PINB, PB0) && changed == 0){
                keydown = 1;
                if (prev == keydown) changed = 0;
                else changed = 1;
                prev = 1;
            }else{
                keydown = 0;
                if (prev == keydown) changed =0;
                else changed = 1;
                prev = 0;
            }
        }
        if(changed){
            changed = 0;
            TIFR0 = 1<<TOV0;
            TCNT0 = 0;
            if(keydown){
                reportBuffer[0] = 0x04;
            }else{
                reportBuffer[0] = 0x00;
            }
            TOG(PORTD, PD7);
            usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
        }
    }
}