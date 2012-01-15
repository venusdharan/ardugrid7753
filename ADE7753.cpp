/* ADE7753.cpp = Class constructor and function implementation for Olimex Energy Shield using ADE7753)
======================================================================================================
V1.0
MercinatLabs / MERCINAT SARL France
By: Thierry Brunet de Courssou
http://www.mercinat.com
Created:     27 Dec 2011
Last update: 01 Jan 2012

Project hosted at: http://code.google.com/p/ardugrid7753 - Repository type: Subversion
Version Control System: TortoiseSVN 1.7.3, Subversion 1.7.3, for Window7 64-bit - http://tortoisesvn.net/downloads.html

Configuration
-------------
Hardware: Nanode v5 + Olimex Energy Shield + 12V transformer + Current transformer
www.nanode.eu
http://www.olimex.cl/product_info.php?products_id=797&product__name=Arduino_Energy_Shield
http://www.conradpro.fr/webapp/wcs/stores/servlet/CatalogSearchFASResultView?storeId=10051&catalogId=10101&langId=-2&searchSKU=&fh_search=504462-62 
http://www.seeedstudio.com/depot/noninvasive-ac-current-sensor-30a-max-p-519.html?cPath=144_154
http://garden.seeedstudio.com/images/b/bc/SCT013-030V.pdf

schematics:
	http://wiki.london.hackspace.org.uk/w/images/c/cf/Nanode5.pdf
	http://www.olimex.cl/pdf/Main_Sch.pdf

Software: Arduino 1.0 IDE for Windows at http://files.arduino.cc/downloads/arduino-1.0-windows.zip
-- did not test with Arduino IDE 0022 or 0023

Project summary
---------------
Real-time streaming of line grid energy consumption/production and RMS voltage/current 
to Pachube using Nanode cpu board and Olimex Energy Shield fitted with Analog Device ADE7753 

Comments
--------
This code has been abundantly commented with lots of external links to serve as a tutorial for newbies.

Some code is from Olimex http://www.olimex.cl/pdf/MCI-TDD-00797%20Demo.rar that has been
significantly improved and reduced in size to operate on Nanode for streaming to Pachube

	---------------------------------------------------------------------------------
	You will have to examine Analog Device ADE7753 spec for detailed description
			http://www.analog.com/static/imported-files/data_sheets/ADE7753.pdf
	---------------------------------------------------------------------------------

The IRQ Interrupt Request pin of the ADE7753 on the Olimex Energy Shield is not wired 
to the Arduino board, so we need to continuously poll the interrupt status register to 
figure out when an an interrupt flag has be set.

*/  

#if ARDUINO >= 100
#include <Arduino.h> // Arduino 1.0
#else
#include <WProgram.h> // Arduino 0022+
#endif
#include <string.h>
#include <avr/pgmspace.h>
#include "SPI.h"
#include "ADE7753.h"
#include <avr/wdt.h> // Watchdog timer


/** === setSPI ===
* Class constructor, sets chip select pin (CS define) and SPI communication with arduino.
* @param none
* @return void
*/

void ADE7753::setSPI(void) {
	pinMode(CS,OUTPUT);  // Chip select by digital output on pin nbs CS
	digitalWrite(CS, HIGH);//is disabled by default, so need to set
	// SPI Init
	SPI.setDataMode(SPI_MODE2);
	SPI.setClockDivider(SPI_CLOCK_DIV32);
	SPI.setBitOrder(MSBFIRST);
	SPI.begin();
	delay(10);
}

void ADE7753::closeSPI(void) {
	SPI.end();
	delay(10);
}

/*****************************
*
* private functions
*
*****************************/

/** === enableChip ===
* Enable chip, setting low ChipSelect pin (CS)
* @param none
*
*/
void ADE7753::enableChip(void){
	digitalWrite(CS,LOW);
}


/** === disableChip ===
* Disable chip, setting high ChipSelect pin (CS)
* @param none
*
*/
void ADE7753::disableChip(void){
	digitalWrite(CS,HIGH);  
}


/** === read8 ===
* Read 8 bits from the device at specified register
* @param char containing register direction
* @return char with contents of register
*
*/
unsigned char ADE7753::read8(char reg){
	enableChip();
	unsigned char b0;
	delayMicroseconds(50);
	SPI.transfer(reg);
	delayMicroseconds(50);
	b0=SPI.transfer(0x00);
	delayMicroseconds(50);
	disableChip();
	//    return (unsigned long)SPI.transfer(0x00);
	return b0;
}


/** === read16 ===
* Read 16 bits from the device at specified register
* @param char containing register direction
* @return int with contents of register
*
*/
unsigned int ADE7753::read16(char reg){
	enableChip();
	unsigned char b1,b0;
	delayMicroseconds(50);
	SPI.transfer(reg);
	delayMicroseconds(50);
	b1=SPI.transfer(0x00);
	delayMicroseconds(50);
	b0=SPI.transfer(0x00);
	delayMicroseconds(50);
	disableChip();
	return (unsigned int)b1<<8 | (unsigned int)b0;
}


/** === read24 ===
* Read 24 bits from the device at specified register
* @param: char containing register direction
* @return: char with contents of register
*
*/
unsigned long ADE7753::read24(char reg){
	enableChip();
	unsigned char b2,b1,b0;
	delayMicroseconds(50);
	SPI.transfer(reg);
	delayMicroseconds(50);
	b2=SPI.transfer(0x00);
	delayMicroseconds(50);
	b1=SPI.transfer(0x00);
	delayMicroseconds(50);
	b0=SPI.transfer(0x00);
	delayMicroseconds(50);
	disableChip();
	return (unsigned long)b2<<16 | (unsigned long)b1<<8 | (unsigned long)b0;
}



/** === write8 ===
* Write 8 bits to the device at specified register
* @param reg char containing register direction
* @param data char, 8 bits of data to send
*
*/
void ADE7753::write8(char reg, unsigned char data){
	enableChip();
	unsigned char data0 = 0;

	// 8th bit (DB7) of the register address controls the Read/Write mode (Refer to spec page 55 table 13)
	// For Write -> DB7 = 1  / For Read -> DB7 = 0
	reg |= WRITE;
	data0 = (unsigned char)data;
	
	delayMicroseconds(50);
	SPI.transfer((unsigned char)reg);          //register selection
	delayMicroseconds(50);
	SPI.transfer((unsigned char)data0);
	delayMicroseconds(50);
	disableChip();
}


/** === write16 ===
* Write 16 bits to the device at specified register
* @param reg: char containing register direction
* @param data: int, 16 bits of data to send
*
*/
void ADE7753::write16(char reg, unsigned int data){
	enableChip();
	unsigned char data0=0,data1=0;
	// 8th bit (DB7) of the register address controls the Read/Write mode (Refer to spec page 55 table 13)
	// For Write -> DB7 = 1  / For Read -> DB7 = 0
	reg |= WRITE;
	//split data
	data0 = (unsigned char)data;
	data1 = (unsigned char)(data>>8);
	
	//register selection, we have to send a 1 on the 8th bit to perform a write
	delayMicroseconds(50);
	SPI.transfer((unsigned char)reg);    
	delayMicroseconds(50);    
	//data send, MSB first
	SPI.transfer((unsigned char)data1);
	delayMicroseconds(50);
	SPI.transfer((unsigned char)data0);  
	delayMicroseconds(50);
	disableChip();
}



/*****************************
*
*     public functions
*
*****************************/


/**
* In general:
* @params:  void
* @return: register content (measure) of the proper type depending on register width
*/

/** === setMode / getMode ===
* This is a 16-bit register through which most of the ADE7753 functionality is accessed.
* Signal sample rates, filter enabling, and calibration modes are selected by writing to this register.
* The contents can be read at any time.
* 
* 
* The next table describes the functionality of each bit in the register:
* 
* Bit     Location	Bit Mnemonic	Default Value Description			
* 0	   DISHPF       0	        HPF (high-pass filter) in Channel 1 is disabled when this bit is set.		
* 1	   DISLPF2      0	        LPF (low-pass filter) after the multiplier (LPF2) is disabled when this bit is set.		
* 2	   DISCF        1	        Frequency output CF is disabled when this bit is set.		
* 3	   DISSAG       1	        Line voltage sag detection is disabled when this bit is set.		
* 4	   ASUSPEND     0	        By setting this bit to Logic 1, both ADE7753 A/D converters can be turned off. In normal operation, this bit should be left at Logic 0. All digital functionality can be stopped by suspending the clock signal at CLKIN pin.		
* 5	   TEMPSEL      0	        Temperature conversion starts when this bit is set to 1. This bit is automatically reset to 0 when the temperature conversion is finished.		
* 6	   SWRST        0	        Software Chip Reset. A data transfer should not take place to the ADE7753 for at least 18 us after a software reset.		
* 7	   CYCMODE      0	        Setting this bit to Logic 1 places the chip into line cycle energy accumulation mode.		
* 8	   DISCH1       0	        ADC 1 (Channel 1) inputs are internally shorted together.		
* 9	   DISCH2       0	        ADC 2 (Channel 2) inputs are internally shorted together.		
* 10	   SWAP         0	        By setting this bit to Logic 1 the analog inputs V2P and V2N are connected to ADC 1 and the analog inputs V1P and V1N are connected to ADC 2.		
* 12, 11  DTRT1,0      0	        These bits are used to select the waveform register update rate.		
* 				        DTRT 1	DTRT0	Update Rate
* 				            0	0	27.9 kSPS (CLKIN/128)
* 				            0	1	14 kSPS (CLKIN/256)
* 				            1	0	7 kSPS (CLKIN/512)
* 				            1	1	3.5 kSPS (CLKIN/1024)
* 14, 13  WAVSEL1,0	0	        These bits are used to select the source of the sampled data for the waveform register.		
* 			                  WAVSEL1, 0	Length	Source
* 			                  0	        0	24 bits active power signal (output of LPF2)
* 			                  0	        1	Reserved
* 			                  1	        0	24 bits Channel 1
* 			                  1	        1	24 bits Channel 2
* 15	POAM	        0	        Writing Logic 1 to this bit allows only positive active power to be accumulated in the ADE7753.		
* 
* 
* @param none
* @return int with the data (16 bits unsigned).
*/
void ADE7753::setMode(int m){
	write16(MODE, m);
}
int ADE7753::getMode(){
	return read16(MODE);
}

/* The IRQ Interrupt Request pin of the ADE7753 on the Olimex Energy Shield is not wired 
to the Arduino board, so need to poll to service the interrupts.
See board spec and wiring diagram at: 
http://www.olimex.cl/product_info.php?products_id=797&product__name=Arduino_Energy_Shield
http://www.olimex.cl/pdf/Main_Sch.pdf
*/  

/** === getInterrupts / setInterrupts
* ADE7753 interrupts can be deactivated at any time by setting the corresponding
* bit in this 16-bit enable register to Logic 0. The status register continues
* to register an interrupt event even if disabled. However, the IRQ output is not activated.

* The next table summarizes the function of each bit in this register:
* 
* BitLocation / Interrupt Flag / Description			
* 0	AEHF	    Indicates that an interrupt occurred because the active energy register, AENERGY, is more than half full.			
* 1	SAG	        Indicates that an interrupt was caused by a SAG on the line voltage.			
* 2	CYCEND	    Indicates the end of energy accumulation over an integer number of half line cycles as defined by the content of the LINECYC register—see the Line Cycle Energy Accumulation Mode section.			
* 3	WSMP	    Indicates that new data is present in the waveform register.			
* 4	ZX	        This status bit is set to Logic 0 on the rising and falling edge of the the voltage waveform. See the Zero-Crossing Detection section.			
* 5	TEMP	    Indicates that a temperature conversion result is available in the temperature register.			
* 6	RESET	    Indicates the end of a reset (for both software or hardware reset). The corresponding enable bit has no function in the interrupt enable register, i.e., this status bit is set at the end of a reset, but it cannot be enabled to cause an interrupt.			
* 7	AEOF	    Indicates that the active energy register has overflowed.			
* 8	PKV	        Indicates that waveform sample from Channel 2 has exceeded the VPKLVL value.			
* 9	PKI	        Indicates that waveform sample from Channel 1 has exceeded the IPKLVL value.			
* 10	VAEHF	    Indicates that an interrupt occurred because the active energy register, VAENERGY, is more than half full.			
* 11	VAEOF	    Indicates that the apparent energy register has overflowed.			
* 12	ZXTO	    Indicates that an interrupt was caused by a missing zero crossing on the line voltage for the specified number of line cycles—see the Zero-Crossing Timeout section.			
* 13	PPOS	    Indicates that the power has gone from negative to positive.			
* 14	PNEG	    Indicates that the power has gone from positive to negative.			
* 15	RESERVED	Reserved.			
* 
//Register address
	IRQEN     0x0A  // interrupts enable register
	STATUS    0x0B  // interrupts status register
	RSTSTATUS 0x0C  // interrupts status register but read will reset all interrupt flags

* @param none
* @return int with the data (16 bits unsigned).
*/
int ADE7753::getEnabledInterrupts(void){
	return read16(IRQEN);
}


/** === getStatus ===
* This is an 16-bit read-only register. The status register contains information regarding the source of ADE7753 interrupts
* @param none
* @return int with the data (16 bits unsigned).
*/
int ADE7753::getInterruptStatus(void){
	return read16(STATUS);
}

/** === resetStatus ==
* Same as the interrupt status register except that the register contents are reset to 0 (all flags cleared) after a read operation.
* @param none
* @return int with the data (16 bits unsigned).
*/
int ADE7753::getresetInterruptStatus(void){
	return read16(RSTSTATUS);
}


/** (1) === getActiveEnergyLineSync ===
* The instantaneous active power is accumulated in this read-only register over 
* the LINECYC number of half line cycles.
* Used in combination with CYCEND Interrupt Flag and with LINECYC register (nbr of half-cycles)
* >>> This is the prefered method The advantage of summing the active energy over an integer number 
*     of line cycles is that the sinusoidal component in the active energy is reduced to 0. 
*     This eliminates any ripple in the energy calculation. Energy is calculated more 
*     accurately and in a shorter time because the integration period can be shortened.
* @param none
* @return long with the data (24 bits 2-complement signed).
*/
long ADE7753::getActiveEnergyLineSync(void){
	return read24(LAENERGY);
}

/** (2) === getApparentEnergyLineSync ===
* The instantaneous real power is accumulated in this read-only register over 
* the LINECYC number of half line cycles.
* >>> This is the prefered method The advantage of summing the active energy over an integer number 
*     of line cycles is that the sinusoidal component in the active energy is reduced to 0. 
*     This eliminates any ripple in the energy calculation. Energy is calculated more 
*     accurately and in a shorter time because the integration period can be shortened. 
* @param none
* @return long with the data (24 bits unsigned).
*/
long ADE7753::getApparentEnergyLineSync(void){
	return read24(LVAENERGY);
}

/** (3) === getReactiveEnergyLineSync ===
* The instantaneous reactive power is accumulated in this read-only register over 
* the LINECYC number of half line cycles.
* >>> This is the prefered method The advantage of summing the active energy over an integer number 
*     of line cycles is that the sinusoidal component in the active energy is reduced to 0. 
*     This eliminates any ripple in the energy calculation. Energy is calculated more 
*     accurately and in a shorter time because the integration period can be shortened.
* @param none
* @return long with the data (24 bits 2-complement signed).
*/
long ADE7753::getReactiveEnergyLineSync(void){
	return read24(LVARENERGY);
}

/** === getIRMS ===
* Channel 2 RMS Value (Current Channel).
* The update rate of the Channel 2 rms measurement is CLKIN/4.
* To minimize noise, synchronize the reading of the rms register with the zero crossing 
* of the voltage input and take the average of a number of readings.
* @param none
* @return long with the data (24 bits unsigned).
*/
long ADE7753::getIRMS(void){
	long lastupdate = 0;
	ADE7753::getresetInterruptStatus(); // Clear all interrupts
	lastupdate = millis();
	while( !  ( ADE7753::getInterruptStatus() & ZX )  )   // wait Zero-Crossing
	{ // wait for the selected interrupt to occur
		if ( ( millis() - lastupdate ) > 100) 
		{ 
			wdt_reset();
			Serial.println("\n--> getIRMS Timeout - no AC input"); 
			break;  
		}
	}          
	return read24(IRMS);
}

/** === getVRMS ===
* Channel 1 RMS Value (Voltage Channel).
* The update rate of the Channel 1 rms measurement is CLKIN/4.
* To minimize noise, synchronize the reading of the rms register with the zero crossing 
* of the voltage input and take the average of a number of readings.
* @param none
* @return long with the data (24 bits unsigned).
*/
long ADE7753::getVRMS(void){
	long lastupdate = 0;
	ADE7753::getresetInterruptStatus(); // Clear all interrupts
	lastupdate = millis();
	while( !  ( ADE7753::getInterruptStatus() & ZX )  )   // wait Zero-Crossing
	{ // wait for the selected interrupt to occur
		if ( ( millis() - lastupdate ) > 100) 
		{ 
			wdt_reset();
			Serial.println("\n--> getIRMS Timeout - no AC input"); 
			break;  
		}
	}          
	return read24(VRMS);
}

/** === vrms ===
* Returns the mean of last 100 readings of RMS voltage. Also supress first reading to avoid 
* corrupted data.
* rms measurement update rate is CLKIN/4.
* To minimize noise, synchronize the reading of the rms register with the zero crossing 
* of the voltage input and take the average of a number of readings.
* @param none
* @return long with RMS voltage value 
*/
long ADE7753::vrms(){
	char i=0;
	long v=0;
	getVRMS();//Ignore first reading to avoid garbage
	for(i=0;i<100;++i){
		v+=getVRMS();
	}
	return v/100;
}

/** === irms ===
* Returns the mean of last 100 readings of RMS current. Also supress first reading to avoid 
* corrupted data.
* rms measurement update rate is CLKIN/4.
* To minimize noise, synchronize the reading of the rms register with the zero crossing 
* of the voltage input and take the average of a number of readings.
* @param none
* @return long with RMS current value in hundreds of [mA], ie. 6709=67[mA]
*/
long ADE7753::irms(){
	char n=0;
	long i=0;
	getIRMS();//Ignore first reading to avoid garbage
	for(n=0;n<100;++n){
		i+=getIRMS();
	}
	return i/100;
}

/** === getWaveform ===
* This read-only register contains the sampled waveform data from either Channel 1,
* Channel 2, or the active power signal. The data source and the length of the waveform 
* registers are selected by data Bits 14 and 13 in the mode register.
* - Max sampling CLKIN/128 = 3.579545 MHz / 128 = 27.9 kSPS
* - Bandwidth 14 kHz
* - one of four output sample rates can be chosen by using Bits 11 and 12 
*   of the mode register (WAVSEL1,0). The output sample rate 
*   can be 27.9 kSPS, 14 kSPS, 7 kSPS, or 3.5 kSPS
* - arrivals of new waveform samples after each read is indicated by interrupt
*   request IRQ, but unfortunatly IRQ is not wired in the Olimex Energy Shield
* - The interrupt request output, IRQ, signals a new sample 
*   availability by going active low.
* - In waveform sampling mode, the WSMP bit (Bit 3) in the 
*   interrupt enable register must also be set to Logic 1.
* - The interrupt request output IRQ stays low until the interrupt 
*   routine reads the reset status register.
* - Interrupt Flag WSMP (bit location 3) in the Interrupt Status Register
*   indicates that new data is present in the waveform register.
*   Therefore arrival of new waveform samples may be indicated by polling 
*   this flag and then reading the 24-bit waveform register
*   --- we use this polling method for Arduino/Nanode ---
* - When acquiring waveform data, disable low pass filter in order to 
*   obtain and view all the high harmoniques
* @param none
* @return long with the data (24 bits 2-complement signed).
*/
long ADE7753::getWaveform(void){  // this function will have to be rewritten for allowing rapid polling 
	// of WSMP flag in ISR and getting rapid Waveform data over 1 full cycle
	// and storing to Arduino tiny RAM
	return read24(WAVEFORM);
}

/** === getIpeakReset ===
* Same as Channel 1 Peak Register except that the register contents are reset to 0 after read.
* @param none
* @return long with the data (24 bits 24 bits unsigned).
*/
long ADE7753::getIpeakReset(void){
	return read24(RSTIPEAK);
}

/** === getVpeakReset ===
* Same as Channel 2 Peak Register except that the register contents are reset to 0 after a read.
* @param none
* @return long with the data (24 bits  unsigned).
*/
long ADE7753::getVpeakReset(void){
	return read24(RSTVPEAK);
}

/** === getPeriod ===
* Period of the Channel 2 (Voltage Channel) Input Estimated by Zero-Crossing Processing. 
* The ADE7753 provides the period measurement of the grid line. 
* The period register is an unsigned 16-bit register and is updated every period. 
* The MSB of this register is always zero.
* The resolution of this register is 2.2 μs/LSB when CLKIN = 3.579545 MHz, 
* which represents 0.013% when the line fre-quency is 60 Hz. 
* When the line frequency is 60 Hz, the value of the period register is 
* approximately CLKIN/4/32/60 Hz × 16 = 7457d.
* The length of the register enables the measurement of line frequencies as low as 13.9 Hz.
* @param none
* @return int with the data (16 bits unsigned).
*/
int ADE7753::getPeriod(void){
	return read16(PERIOD);
}

/** === getTemp ===
* Force a temperature measure and then returns it. This is done by setting bit 5 HIGH in MODE register.
* Temperature measuring can't be calibrated, the values used in this function are according to the datasheet
* (register TEMP is 0x00 at -25 celsius degrees).
*  The contents of the temperature register are signed (twos complement) with a resolution of 
* approximately 1.5 LSB/°C. The temperature register produces a code of 0x00 when the ambient 
* temperature is approximately −25°C. The temperature measurement is uncalibrated in the ADE7753 
* and has an offset tolerance as high as ±25°C.
* @param none
* @return char with the temperature in celsius degrees.
*/
char ADE7753::getTemp(){
	unsigned char r=0; 
	long lastMode = 0;
	long lastupdate = 0;
	lastMode = getMode();
	//Temp measure
	setMode(TEMPSEL);
	ADE7753::getresetInterruptStatus(); // Clear all interrupts
	lastupdate = millis();
	while( ! ( ADE7753::getInterruptStatus() & TEMPREADY ) ) // wait for Temperature measurement to be ready
	{ // wait for the selected interrupt to occur
		if ( ( millis() - lastupdate ) > 100) 
		{ 
			wdt_reset();
			Serial.println("\n--> Temperature Timeout no AC input"); 
			ADE7753::getresetInterruptStatus(); // Clear all interrupts
			break;  
		}
	}  
	//Read register
	r= read8(TEMP);
	
	//    // Do it twice to make sure     
	//    setMode(TEMPSEL);
	//    ADE7753::getresetInterruptStatus(); // Clear all interrupts
	//    lastupdate = millis();
	//    while( ! ( ADE7753::getInterruptStatus() & TEMPREADY ) ) // wait for Temperature measurement to be ready
	//        { // wait for the selected interrupt to occur
	//             if ( ( millis() - lastupdate ) > 2500) 
	//               { 
	//                 Serial.println("--> Temperature Timeout no AC input"); 
	//                 ADE7753::getresetInterruptStatus(); // Clear all interrupts
	//                 break;  
	//               }
	//         }
	//Read register
	r= read8(TEMP);
	
	// Set to the previous mode
	setMode (lastMode);
	return r;
}

// Functions for manual setting of calibrations

/** === energySetup ===
* @param 
* @param 
*/
void ADE7753::energySetup(int wgain, char wdiv, int apos, int vagain, char vadiv, char phcal){
	write16(WGAIN,wgain);
	write8(WDIV,wdiv);
	write16(APOS,apos);  
	write16(VAGAIN,vagain);
	write8(VADIV,vadiv);
	write8(PHCAL,phcal);
}


/** === frequencySetup ===
* The output frequency on the CF pin is adjusted by writing to this 12-bit
* read/write register—see the Energy-to-Frequency Conversion section.
* @param cfnum: integer containing number (12 bits available unsigned. ie range=[0,4095])
* @param cfden: the same as cfnum
*/
void ADE7753::frequencySetup(int cfnum, int cfden){
	write16(CFNUM,cfnum);
	write16(CFDEN,cfden);
}

/** === analogSetup ===
* This 8-bit register is used to adjust the gain selection for the PGA in Channels 1 and 2
* @param gain_ch1 char set the PGA channel 1 gain, use constants GAIN_1, GAIN_2, GAIN_4, GAIN_8 and gain_16
* @param gain_ch2 char set the PGA channel 2 gain, use constants GAIN_1, GAIN_2, GAIN_4, GAIN_8 and gain_16
* @param os_ch1 char set channel 1 analog offset, range : [-32,32]
* @param os_ch2 char set channel 1 analog offset, range : [-32,32]
* @param scale_ch1 char
* @param integrator_ch1 char
* @return char with the data (8 bits unsigned).
*/
void ADE7753::analogSetup(char gain_ch1, char gain_ch2,char os_ch1,char os_ch2,char scale_ch1,char integrator_ch1){
	unsigned char pga = (gain_ch2<<5) | (scale_ch1<<3) | (gain_ch1); //format is |3 bits PGA2 gain|2 bits full scale|3 bits PGA1 gain
	char sign = 0;
	char ch1os = 0, ch2os = 0;
	
	
	write8(GAIN,pga);//write GAIN register, format is |3 bits PGA2 gain|2 bits full scale|3 bits PGA1 gain
	
	// CH1OS: ch1 offset 6-bit, sign magnitude on Bit-5 then integrator on Bit-7
	// Refer to spec Page 58 Table 16
	if(os_ch1<0){
		sign=1;
		os_ch1=-os_ch1;
	} else{ sign=0; }
	ch1os = (integrator_ch1<<7) | (sign<<5) | os_ch1;
	write8(CH1OS,ch1os);

	// CH2OS: ch2 offset, sign magnitude (ch2 doesn't have integrator) and the offset applied is inverted (ie offset of -1 sums 1)
	if(os_ch2<0){
		sign=1;
		os_ch2=-os_ch2;
	} else{ sign=0; }
	ch2os = (sign<<5) | os_ch2;
	write8(CH2OS,ch2os);
} 

/** === rmsSetup ===
**/
void ADE7753::rmsSetup(int irmsos, int vrmsos){
	write16(VRMSOS,vrmsos);
	write16(IRMSOS,irmsos);
}   

void ADE7753::miscSetup(int zxtout, char sagsyc, char saglvl, char ipklvl, char vpklvl, char tmode){
	// ZXTOUT 12-bit (U) - Zero-Crossing Timeout
	// SAGCYC  8-bit (U) - Sag Line Cycle Register.
	// SAGLVL  8-bit (U) - Sag Voltage Level.
	// IPKLVL  8-bit (U) - Channel 1 Peak Level Threshold
	// VPKLVL  8-bit (U) - Channel 2 Peak Level Threshold
	// TMODE   8-bit (U) - Test Mode Register
	write16(ZXTOUT,zxtout);
	write8(SAGCYC,sagsyc);
	write8(SAGLVL,saglvl);    
	write8(IPKLVL,ipklvl); 
	write8(VPKLVL,vpklvl);
	write8(TMODE,tmode);
}

void ADE7753::setInterruptsMask(int Mask16){
	write16(IRQEN, Mask16);
}

void ADE7753::setLineCyc(int d){
	write16(LINECYC,d);
}

// May have to remove this code because of code/RAM size restriction with Nanode

#define NanodeReduceCodeSize 1
#ifndef NanodeReduceCodeSize

/** === printGetResetInterruptStatus ===
* used for code verification et debugging
*/
void ADE7753::printGetResetInterruptStatus(void){

	int InterruptMask;
	
	Serial.print("Interrupt Status (binary): ");
	InterruptMask = ADE7753::getresetInterruptStatus();
	Serial.println(InterruptMask,BIN);
	
	if ( InterruptMask & AEHF )      Serial.println("AEHF");
	if ( InterruptMask & SAG  )      Serial.println("SAG");
	if ( InterruptMask & CYCEND )    Serial.println("CYCEND");
	if ( InterruptMask & WSMP )      Serial.println("WSMP");
	if ( InterruptMask & ZX )        Serial.println("ZX");
	if ( InterruptMask & TEMPREADY ) Serial.println("TEMPREADY");
	if ( InterruptMask & RESET )     Serial.println("RESET");
	if ( InterruptMask & AEOF )      Serial.println("AEOF");
	if ( InterruptMask & PKV )       Serial.println("PKV");
	if ( InterruptMask & PKI )       Serial.println("PKI");
	if ( InterruptMask & VAEHF )     Serial.println("VAEHF");
	if ( InterruptMask & VAEOF )     Serial.println("VAEOF");
	if ( InterruptMask & ZXTO )      Serial.println("ZXTO");
	if ( InterruptMask & PPOS )      Serial.println("PPOS");
	if ( InterruptMask & PNEG )      Serial.println("PNEG");
	if ( InterruptMask & RESERVED )  Serial.println("RESERVED");
}    

/** === printGetMode ===
* used for code verification et debugging
*/     
void ADE7753::printGetMode(void){

	int ModeMask;
	
	Serial.print("Mode (binary): ");
	ModeMask = ADE7753::getMode();
	Serial.println(ModeMask,BIN);
	
	if ( ModeMask & DISHPF )   Serial.println("DISHPF");
	if ( ModeMask & DISLPF2  ) Serial.println("DISLPF2");
	if ( ModeMask & DISCF )    Serial.println("DISCF");
	if ( ModeMask & DISSAG )   Serial.println("DISSAG");
	if ( ModeMask & ASUSPEND ) Serial.println("ASUSPEND");
	if ( ModeMask & TEMPSEL )  Serial.println("TEMPSEL");
	if ( ModeMask & SWRST )    Serial.println("SWRST");
	if ( ModeMask & CYCMODE )  Serial.println("CYCMODE");
	if ( ModeMask & DISCH1 )   Serial.println("DISCH1");
	if ( ModeMask & DISCH2 )   Serial.println("DISCH2");
	if ( ModeMask & SWAP )     Serial.println("SWAP");
	if ( ModeMask & DTRT0 )    Serial.println("DTRT0");
	if ( ModeMask & DTRT1 )    Serial.println("DTRT1");
	if ( ModeMask & WAVSEL0 )  Serial.println("WAVSEL0");
	if ( ModeMask & WAVSEL1 )  Serial.println("WAVSEL1");
	if ( ModeMask & POAM )     Serial.println("POAM");   
}


/** === printAllRegisters ===
* used for code verification et debugging
*/    
void ADE7753::printAllRegisters(void){

	unsigned char b = 0;
	int i = 0;
	unsigned char sign = 0;
	int integrator = 0;
	Serial.print("WAVEFORM   "); Serial.println(read24(WAVEFORM),DEC);
	Serial.print("AENERGY    "); Serial.println(read24(AENERGY),DEC);
	Serial.print("RAENERGY   "); Serial.println(read24(RAENERGY),DEC);
	Serial.print("LAENERGY   "); Serial.println(read24(LAENERGY),DEC);
	Serial.print("VAENERGY   "); Serial.println(read24(VAENERGY),DEC);
	Serial.print("RVAENERGY  "); Serial.println(read24(RVAENERGY),DEC);
	Serial.print("LVAENERGY  "); Serial.println(read24(LVAENERGY),DEC);
	Serial.print("LVARENERGY "); Serial.println(read24(LVARENERGY),DEC);
	Serial.print("MODE       "); Serial.println(read16(MODE),BIN);            
	Serial.print("IRQEN      "); Serial.println(read16(IRQEN),BIN);
	Serial.print("STATUS     "); Serial.println(read16(STATUS),BIN);
	Serial.print("RSTSTATUS  "); Serial.println(read16(RSTSTATUS),BIN);

	// CH1OS register - see spec Page 58 Table 16
	b = read8(CH1OS);
	sign = b & 0x20;  // test sign on bit-6
	if ( sign > 1 ) {i = - int (b & 0x1F) ;} else {i = int (b & 0x1F);}
	Serial.print("CH1OS  "); Serial.print(i, DEC);  // convert signed 6-bit into signed 16-bit
	integrator =  b & 0x80; // test integrator flag on bit-8
	if ( integrator > 1 ) { Serial.println(" Integrator ON");} else { Serial.println(" Integrator OFF");} 

	b = read8(CH2OS);
	sign = b & 0x20;  // test sign on bit-6
	if ( sign > 1 ) {i = - int (b & 0x1F) ;} else {i = int (b & 0x1F);}
	Serial.print("CH2OS  "); Serial.println(i, DEC);  // convert signed 6-bit into signed 16-bit
	
	Serial.print("GAIN   "); Serial.println(read8 (GAIN),BIN);
	Serial.print("PHCAL  "); Serial.println(read8 (PHCAL),HEX);
	Serial.print("APOS   "); Serial.println(int(read16(APOS)),DEC);
	Serial.print("WGAIN  "); Serial.println(int (read16(WGAIN)),DEC);
	Serial.print("WDIV   "); Serial.println(read8 (WDIV),DEC);
	Serial.print("CFNUM  "); Serial.println(read16(CFNUM),DEC);
	Serial.print("CFDEN  "); Serial.println(read16(CFDEN),DEC);
	Serial.print("->IRMS   "); Serial.println(read24(IRMS),DEC);
	Serial.print("->VRMS   "); Serial.println(read24(VRMS),DEC);
	Serial.print("--IRMSOS "); Serial.println(int(read16(IRMSOS)),DEC);
	Serial.print("--VRMSOS "); Serial.println(int(read16(VRMSOS)),DEC);
	Serial.print("VAGAIN "); Serial.println(int(read16(VAGAIN)),DEC);
	Serial.print("VADIV  "); Serial.println(read8 (VADIV),DEC);
	Serial.print("LINECYC  "); Serial.println(read16(LINECYC),DEC);
	Serial.print("ZXTOUT   "); Serial.println(read16(ZXTOUT),DEC);
	Serial.print("SAGCYC   "); Serial.println(read8 (SAGCYC),DEC);
	Serial.print("SAGLVL   "); Serial.println(read8 (SAGLVL),DEC);
	Serial.print("IPKLVL   "); Serial.println(read8 (IPKLVL),DEC);
	Serial.print("VPKLVL   "); Serial.println(read8 (VPKLVL),DEC);
	Serial.print("IPEAK    "); Serial.println(read24(IPEAK),DEC);
	Serial.print("RSTIPEAK "); Serial.println(read24(RSTIPEAK),DEC);
	Serial.print("VPEAK    "); Serial.println(read24(VPEAK),DEC);
	Serial.print("RSTVPEAK "); Serial.println(read24(RSTVPEAK),DEC);
	Serial.print("TEMP     "); Serial.println(read8 (TEMP),DEC);
	Serial.print("PERIOD   "); Serial.println(read16(PERIOD),DEC);
	Serial.print("TMODE    "); Serial.println(read8 (TMODE),DEC);
	Serial.print("CHKSUM   "); Serial.println(read8 (CHKSUM),DEC);
	Serial.print("DIEREV   "); Serial.println(read8 (DIEREV),DEC);      
} 

/** === chkSum ===
* Checksum Register. This 6-bit read-only register is equal to the sum of all the ones in the previous read.
* see the ADE7753 Serial Read Operation section.
* @param none
* @return char with the data (6 bits unsigned).
*/
char ADE7753::chkSum(){
	return read8(CHKSUM);
}

/** === getActiveEnergy ===
* Active power is accumulated (integrated) over time in this 24-bit, read-only register
* @param none
* @return long with the data (24 bits 2-complement signed).
*/
long ADE7753::getActiveEnergy(void){
	return read24(AENERGY);
}

/** === getActiveEnergyReset ===
* Same as the active energy register except that the register is reset to 0 following a read operation.
* @param none
* @return long with the data (24 bits 2-complement signed).
*/
long ADE7753::getActiveEnergyReset(void){
	return read24(RAENERGY);
}

/** === getApparentEnergy ===
* Apparent Energy Register. Apparent power is accumulated over time in this read-only register.
* @param none
* @return long with the data (24 bits unsigned).
*/
long ADE7753::getApparentEnergy(void){
	return read24(VAENERGY);
}

/** === getApparentEnergyReset ===
* Same as the VAENERGY register except that the register is reset to 0 following a read operation.
* @param none
* @return long with the data (24 bits unsigned).
*/
long ADE7753::getApparentEnergyReset(void){
	return read24(RVAENERGY);
}

/** === getCurrentOffset ===
* Channel 2 RMS Offset Correction Register.
* @param none
* @return int with the data (12 bits 2-complement signed).
*/
int ADE7753::getCurrentOffset(){
	return read16(IRMSOS);
}

/** === getVoltageOffset ===
* Channel 2 RMS Offset Correction Register.
* 
* @param none
* @return int with the data (12 bits 2-complement's signed).
*/
int ADE7753::getVoltageOffset(){
	return read16(VRMSOS);
}

/** === setZeroCrossingTimeout / getZeroCrossingTimeout ===
* Zero-Crossing Timeout. If no zero crossings are detected
* on Channel 2 within a time period specified by this 12-bit register,
* the interrupt request line (IRQ) is activated
* @param none
* @return int with the data (12 bits unsigned).
*/
void ADE7753::setZeroCrossingTimeout(int d){
	write16(ZXTOUT,d);
}
int ADE7753::getZeroCrossingTimeout(){
	return read16(ZXTOUT);
}

/** === getSagCycles / setSagCycles ===
* Sag Line Cycle Register. This 8-bit register specifies the number of
* consecutive line cycles the signal on Channel 2 must be below SAGLVL
* before the SAG output is activated.
* @param none
* @return char with the data (8 bits unsigned).
*/
char ADE7753::getSagCycles(){
	return read8(SAGCYC);
}
void ADE7753::setSagCycles(char d){
	write8(SAGCYC,d);
}

/** === getLineCyc / setLineCyc ===
* Line Cycle Energy Accumulation Mode Line-Cycle Register. 
* This 16-bit register is used during line cycle energy accumulation mode 
* to set the number of half line cycles for energy accumulation
* @param none
* @return int with the data (16 bits unsigned).
*/
int ADE7753::getLineCyc(){
	return read16(LINECYC);
}


/** === getSagVoltageLevel / setSagVoltageLevel ===
* Sag Voltage Level. An 8-bit write to this register determines at what peak
* signal level on Channel 2 the SAG pin becomes active. The signal must remain
* low for the number of cycles specified in the SAGCYC register before the SAG pin is activated
* @param none
* @return char with the data (8 bits unsigned).
*/
char ADE7753::getSagVoltageLevel(){
	return read8(SAGLVL);
}
void ADE7753::setSagVoltageLevel(char d){
	write8(SAGLVL,d);
}

/** === getIPeakLevel / setIPeakLevel ===
* Channel 1 Peak Level Threshold (Current Channel). This register sets the level of the current
* peak detection. If the Channel 1 input exceeds this level, the PKI flag in the status register is set.
* @param none
* @return char with the data (8 bits unsigned).
*/
char ADE7753::getIPeakLevel(){
	return read8(IPKLVL);
}
void ADE7753::setIPeakLevel(char d){
	write8(IPKLVL,d);
}

/** === getVPeakLevel / setVPeakLevel ===
* Channel 2 Peak Level Threshold (Voltage Channel). This register sets the level of the
* voltage peak detection. If the Channel 2 input exceeds this level, 
* the PKV flag in the status register is set.
* @param none
* @return char with the data (8bits unsigned).
*/
char ADE7753::getVPeakLevel(){
	return read8(VPKLVL);
}
void ADE7753::setVPeakLevel(char d){
	write8(VPKLVL,d);
}

/** === getVpeak ===
* Channel 2 Peak Register. The maximum input value of the voltage channel since the last read of the register is stored in this register.
* @param none
* @return long with the data (24 bits unsigned).
*/
long ADE7753::getVpeak(void){
	return read24(VPEAK);
}

/** === getIpeak ===
* Channel 1 Peak Register. The maximum input value of the current channel since the last read
* of the register is stored in this register.
* @param none
* @return long with the data (24 bits unsigned) .
*/
long ADE7753::getIpeak(void){
	return read24(IPEAK);
}

/** === energyGain ===
* @param 
* @param 
*/
void ADE7753::energyGain(int wgain, int vagain){
	write16(WGAIN,wgain);
	write16(VAGAIN,vagain);
}

#endif

