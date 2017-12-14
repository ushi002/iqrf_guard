// *********************************************************************
// *                           E01 - TX                                *
// *                        RF transmitter                             *
// *********************************************************************
// Example of simple RF transmission.
//
// Intended for:
//    HW: IQRF TR modules TR-52B, TR-53B and compatibles
//        DK-EVAL-03, DK-EVAL-04 development kit
//    OS: v3.00 or higher
//
// Description:
//    This example shows how to make a simple transmitter. It can be
//    tested with the E02-RX example.
//    - RF message is sent (and red LED flashes once) whenever the
//      pushbutton on the kit is pressed.
//    - Content of the messages = Application data.
//      (It is the 32 B array in EEPROM dedicated to user information
//      about the application - see the IQRF OS manual.
//      The "01234567890123456789012345678901" data array is used here.
//    - To lower power consumption the module utilizes the Sleep mode
//      and Wake-up on pin change for response to the button.
//      Wake-up on pin change is under user's control
//      and has to be enabled by the user when needed.
//    - The watchdog is repeatedly cleared.
//      If omitted:
//      - The module is regularly reset from the watchdog overflow
//        which leads to additional transmission whenever the
//        watchdog overflows while the button is just pressed.
//      - Transmission aborted if watchdog overflows while transmitting.
//
//
// File:    E01-TX.c
// Version: v1.00                                   Revision: 08/12/2010
//
// Revision history:
//     v1.00: 08/12/2010  First release
//
// *********************************************************************
#include "includes/template-basic.h"

// *********************************************************************

#define NUMOFUNIT '0'

//#define SPIDEBUG //it will generate alarms!
//#define SENDSTATUS

void APPLICATION()             	// Obligatory assigning - see E00-START
{    
	
	uns8 motion_detected = 0;
	uns8 ring_alarm_force_cnt = 0;
	uns16 wupnum = 0; //number of wakeups (wakeup every 4s by wdt)
	
	//set sck as output:
	#ifndef SPIDEBUG
	disableSPI();
	#else
	enableSPI();                // Enable SPI
	#endif

	setRFmode(_TX_XLP);         //TX XLP mode - prolonged TX preamble
	toutRF = 3;
		
    while (1)                  	// Main cycle (perpetually repeated)
    {
        clrwdt();              	// Clear watchdog
		//keep wdt running to wake up every cca 4s
		SWDTEN = 1;
		
		GIE = 0;               	//   disable all interrupts
		RBIE = 1;              	//   enable wake-up on pin change
		iqrfSleep();           	// Put the module into Sleep mode
		RBIF = 0;              	// Requested: clear flag		
		
		if (_SS)     	//   PIR sensor activated (PORTA.5, SPI SS (C5))
		{			
			motion_detected = 1;			
			pulseLEDR();			
			pulseLEDG();
			waitDelay(25);
		}else
		{
			pulseLEDG();
			waitDelay(25);
#ifdef SENDSTATUS
			wupnum++;
#endif //SENDSTATUS
		}
		
		if (motion_detected)
		{
			motion_detected	= 0;
			wupnum = 0; //do not send status packet
			PIN = 0; //peer-to-peer topology, update after every RFRXpacket before every RFTXpacket
			bufferRF[0] = 'A';  //ALARM
			bufferRF[1] = NUMOFUNIT;   //Number of unit causing alarm
			bufferRF[2] = '_';
			bufferRF[3] = getSupplyVoltage()+48;  //2,25V+getSupplyVoltage()*0,1
			bufferRF[4] = 'V';
            DLEN = 5;         	//      Setup RF packet length								
            RFTXpacket();      	//      Transmit the message			
			pulseLEDR();
		}

#ifdef SENDSTATUS
		//15 wakeups per minute, 15*10 => 10min
		//if (wupnum > 15*10)
		if (wupnum > 3)
		{
			wupnum = 0;
			bufferRF[0] = 'S';  //STATUS
			bufferRF[1] = NUMOFUNIT;   //Number of unit causing alarm
			bufferRF[2] = '_';
			bufferRF[3] = getSupplyVoltage()+48;  //2,25V+getSupplyVoltage()*0,1
			bufferRF[4] = 'V';
            DLEN = 5;         	//      Setup RF packet length
								//        (only 10 B of Application
								//        info to be sent)
            RFTXpacket();      	//      Transmit the message            
			pulseLEDR();
			waitDelay(25);     	//      and wait 250ms (25*10ms)
		}
#endif //SENDSTATUS
    }                          	// End of main cycle
}

// *********************************************************************

#pragma packedCdataStrings 0    // Store the string unpacked
                                //   (one byte in one location)

//                             00000000001111111111222222222233
#pragma cdata[__EEAPPINFO] =  "01234567890123456789012345678901"
                                // 32 B to be stored to the
                                // Application Info array in EEPROM
// *********************************************************************

