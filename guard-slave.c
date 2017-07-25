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
void ring_alarm(void);

void APPLICATION()             	// Obligatory assigning - see E00-START
{    
	
	uns8 motion_detected = 0;
	uns8 ring_alarm_force_cnt = 0;
	uns16 wupnum = 0; //number of wakeups (wakeup every 4s by wdt)
	
	//set sck as output:
	#ifndef SPIDEBUG
	disableSPI();
	TRISC.3 = 0;	//port direction of portc.3 (sam as _SCK) for beeper
	_SCK = 0;
	
	TRISA.5 = 1;	//port direction of porta.5 (_SS) for PIR
	#else
	enableSPI();                // Enable SPI
	#endif

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
			wupnum++;
		}
		
		if (motion_detected)
		{	
			wupnum = 0; //do not send status packet
			//ring_alarm_force_cnt - keep asking master for answer every wdt-timeout-wakeup			
			ring_alarm_force_cnt++;
			PIN = 0; //after every RFRXpacket before every RFTXpacket
			bufferRF[0] = 'A';  //ALARM
			bufferRF[1] = NUMOFUNIT;   //Number of unit causing alarm
			bufferRF[2] = '_';
			bufferRF[3] = getSupplyVoltage()+48;  //2,25V+getSupplyVoltage()*0,1
			bufferRF[4] = 'V';
            DLEN = 5;         	//      Setup RF packet length								
            RFTXpacket();      	//      Transmit the message			
			pulseLEDR();
		}

		//wait for alarm ack or timeout -- TODO: CHANGE THIS: ring alarm after the object is captured on camera - defined by constant time
		if (RFRXpacket())           // If anything was received
        {			
			#ifdef SPIDEBUG
            copyBufferRF2COM();     //   Copy received RF data  from bufferRF to bufferCOM
            startSPI(DLEN);         //     and send it via SPI
			#endif
			pulseLEDR();			
			pulseLEDG();
			waitDelay(25);
			
			if (bufferRF[0] == 'A' && bufferRF[1] == 'C' && bufferRF[2] == 'K' && bufferRF[3] == NUMOFUNIT)
			{				   
				ring_alarm_force_cnt = 0; 		//master communicates, wait for his command to ring alarm
				pulsingLEDG();                      // 4x LED flash (ACK indication)
				waitDelay(95);
				stopLEDG();				
				if (bufferRF[4] == 'A')        //alarm acknowledged, ring alarm
				{
					motion_detected = 0;
					ring_alarm();					
				}
			}
        }
		
		if (ring_alarm_force_cnt > 5)
		{
			//it seems master is off, thus ring alarm to scare thiefs
			ring_alarm_force_cnt = 0;
			motion_detected = 0;
			ring_alarm();
		}

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
    }                          	// End of main cycle
}

void ring_alarm(void)
{
	uns8 i,j;
	
	for(i=0; i<1; i++)
	{
		_SCK = 1;           // BEEP ON
		waitDelay(25);     	//      and wait 250ms (25*10ms)
		_SCK = 0;           // BEEP OFF
		waitDelay(75);
	}
	pulsingLEDR();                      // 4x LED flash (ALARM indication)
	waitDelay(95);
	stopLEDR();
}
// *********************************************************************

#pragma packedCdataStrings 0    // Store the string unpacked
                                //   (one byte in one location)

//                             00000000001111111111222222222233
#pragma cdata[__EEAPPINFO] =  "01234567890123456789012345678901"
                                // 32 B to be stored to the
                                // Application Info array in EEPROM
// *********************************************************************

