/* ArduGrid7753 = Nanode + Olimex Energy Shield power (using ADE7753) + datastreaming to Pachube
================================================================================================
V1.1
MercinatLabs / MERCINAT SARL France
By: Thierry Brunet de Courssou
http://www.mercinat.com
Created 28 May 2011
Last update: 14 Jan 2012
Project hosted at: http://code.google.com/p/arduwind/ - Repository type: Subversion
Version Control System: TortoiseSVN 1.7.3, Subversion 1.7.3, for Window7 64-bit - http://tortoisesvn.net/downloads.html

Changes
-------
V1.0 - Original release
V1.1 - Reduced SRAM usage with showString, added watchdog timer, added SRAM memory check, randomize network start
	over 2 seconds to avoid all Nanodes to collide on network access
	record number of reboots in EEPROM and feed to Pachube, record number of Watchdog timeouts and feed to Pachube 
V1.2 (soon)- get NTP atomic time, use ATmega328 1024 bytes EEPROM, use Microchip 11AA02E48 2Kbit serial EEPROM (MAC chip),
V1.3 (soon)- Averaging, 1mn/1h/24h/30days


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

*/

/*
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

--------------------------------------

The ATMEL CPU ATMEGA328P mounted on the Nanode board is a 
high-performance Atmel picoPower 8-bit AVR RISC-based microcontroller that combines 32KB ISP flash memory 
with read-while-write capabilities, 1024B EEPROM, 2KB SRAM, 23 general purpose I/O lines, 32 general purpose 
working registers, three flexible timer/counters with compare modes, internal and external interrupts, 
serial programmable USART, a byte-oriented 2-wire serial interface, SPI serial port, a 6-channel 10-bit A/D 
converter (8-channels in TQFP and QFN/MLF packages), programmable watchdog timer with internal oscillator, 
and five software selectable power saving modes. The device operates between 1.8-5.5 volts.

	http://www.atmel.com/dyn/resources/prod_documents/8271S.pdf
	http://www.atmel.com/dyn/resources/prod_documents/doc8271.pdf

The Analog Device ADE7753 circuit and Microchip ENC28J60 circuit both use the SPI interface.

	http://www.analog.com/static/imported-files/data_sheets/ADE7753.pdf
	http://ww1.microchip.com/downloads/en/DeviceDoc/39662c.pdf

Refer to SPI library at: http://www.arduino.cc/en/Reference/SPI
also at WIKI: http://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus
	
	SPI for ADE7753 on the Olimex Energy Shield is connected to digital pin D11, D12 and D13 
of the Arduino. Chip select for ADE7753 is connected to D10.
	http://www.olimex.cl/pdf/Main_Sch.pdf

SPI for ENC28J60 on the Nanode board is connected to digital pin D3, D4, and D5 of the Arduino.
Chip select for ENC28J60 is connected to D8.
	http://wiki.london.hackspace.org.uk/w/images/c/cf/Nanode5.pdf

The SPI channels need to be orderly OPENED and CLOSED in turn using SPI.begin() followed by 
all SPI configuration settings, then SPI.close() when accessing the circuits.

For every SPI device there are four signals that control SPI data transfer: The MOSI, MSIO and SCK signals 
and another signal called the "Chip Select."  Each SPI device has its own "Chip Select" signal.  A 
"Chip Select" signal can use any Arduino pin except for ones that have already been committed to other 
functions.  So, in particular the "Chip Select" signal for a SPI device can not be pin 11, 12, or 13.

"SS" refers to "Slave Select" function of the SPI interface inside the ATMega328p. 

The Ethernet library reads and writes to the SPI registers to cause data transfer into or out of the SPI 
port in the following sequence:

	1. It calls a function named setSS() to put the Ethernet Shield "Chip Select" pin in proper state (LOW, as 
	it happens) to perform SPI operations on the Ethernet Shield.

	2. It does some other SPI register stuff to set up things to read or write and to fire off the serial 
	transfer.

	3. It calls a function named resetSS() to put the "Chip Select" pin in proper state (HIGH) to tell the 
	Ethernet Shield to stay off of the SPI bus.

Typically there are three lines common to all the devices,
	Master In Slave Out (MISO) - The Slave line for sending data to the master,
	Master Out Slave In (MOSI) - The Master line for sending data to the peripherals,
	Serial Clock (SCK) - The clock pulses which synchronize data transmission generated by the master, and
	Slave Select pin - the pin on each device that the master can use to enable and disable specific devices. 

	When a device's Slave Select pin is low, it communicates with the master. When it's high, it ignores the 
master. This allows you to have multiple SPI devices sharing the same MISO, MOSI, and CLK lines.

--------------------------------------	

Sag Voltage Level (SAGLVL), Channel 1 Current Peak Level Threshold (IPKLVL) and Channel 2 Peak Voltage 
Level Threshold (VPKLVL) are not used because the IRQ signal is not wired to wakeup the CPU.

Pin CF frequency output is not used for energy measurement or for calibration, therefore complete 
calibration via the ADE7753 internal registers is not necessary. For sake of simplicity, we only use the
internal registers for offset compensation. Calibration are done directly in the code.

The cristal associated to the for the ADE7753 the Olimex energy shield is 4.000000 MHz . Therefore 
the CLKIN Frequency 3.579545 MHz mentionned in the specification is not applicable, and all the 
calculations linked to the clock need to be reviewed.

----------------------------------------------------------------------------------------------

http://www.analog.com/static/imported-files/recommended_reading/815002980PowerTheoryNew.pdf

ACTIVE POWER CALCULATION
Power is defined as the rate of energy flow from source to load. It is defined as the product 
of the voltage and current wave-forms. The resulting waveform is called the instantaneous power 
signal and is equal to the rate of energy flow at every instant of time. 
The unit of power is the watt or joules/sec.

REACTIVE POWER CALCULATION
Reactive power is defined as the product of the voltage and current waveforms when one of these signals 
is phase-shifted by 90°. The resulting waveform is called the instantaneous reactive power signal.
the reactive power measurement is negative when the load is capacitive, and positive when the load is inductive. 
The sign of the reactive power can therefore be used to reflect the sign of the power factor.

APPARENT POWER CALCULATION
The apparent power is defined as the maximum power that can be delivered to a load. 
Vrms and Irms are the effective voltage and current delivered to the load; the apparent power (AP) 
is defined as Vrms × Irms. The angle θ between the active power and the apparent power 
generally represents the phase shift due to non-resistive loads.

POWER FACTOR
Power factor in an ac circuit is
defined as the ratio of the total active power flowing to the load to the apparent power. 
The absolute power factor measurement is defined in terms of leading or lagging referring 
to whether the current is leading or lagging the voltage waveform. When the
current is leading the voltage, the load is capacitive and this is
defined as a negative power factor. When the current is lagging the voltage, the load is 
inductive and this defined as a positive power factor.

Current Transformer used is: SCT-013-030 (0-30A)
http://www.seeedstudio.com/depot/noninvasive-ac-current-sensor-30a-max-p-519.html

This is a standard current transformer thus "digital integration" is not activated.
A di/dt current sensor (Rogowski coil) is not used (which would require digital integrator to be activated)

Note that the SCT-013-030 has a burden resistor for providing 1V for 30A, which is too high for the AD7753 input.
Therefore the Energy Shield includes a second burden resistor R5 20 ohm to lower the coil voltage.

Today’s current sensing solutions
=================================
The three most common sensor technologies today are the low resistance current
shunt, the current transformer (CT), and the Hall effect sensor. 
However, small size precison Rogowski coil, combined with digital integrator, offers a cost competitive
current sensing technology that could become the technology of choice for the next
generation electric energy meters. See whitepaper at:
http://www.analog.com/static/imported-files/tech_articles/16174506155607IIC_Paper.pdf
http://www.analog.com/static/imported-files/recommended_reading/688920132Current_sensing_for_meteringNew.pdf

*/

/*  
Configuration & Calibrations
----------------------------

6-bit  ->         64  or  +/-        32
8-bit  ->        256  or  +/-       128
12-bit ->      4 096  or  +/-     2 048
16-bit ->     65 536  or  +/-    32 768
24-bit -> 16 777 216  or  +/- 8 388 608
	
meter.analogSetup(GAIN_1, GAIN_1, 0, 0, FULLSCALESELECT_0_5V, INTEGRATOR_OFF);  // GAIN1, GAIN2, CH1OS, CH2OS, Range_ch1, integrator_ch1
	Gain_ch1 set the PGA channel 1 gain, use constants GAIN_1, GAIN_2, GAIN_4, GAIN_8 and GAIN_16
	Gain_ch2 set the PGA channel 2 gain, use constants GAIN_1, GAIN_2, GAIN_4, GAIN_8 and GAIN_16
	CH1OS set channel 1 analog offset, 6-bit (S) range : [-32,32] - (Refer to spec page 17 Table 6, page 58 Table 16))
	CH2OS set channel 2 analog offset, 6-bit (S) range : [-32,32] -- CH2OS register is inverted. To apply a positive offset, 
			a negative number is written to this register.
	Range_ch1 - Channel 1 only has a range selection 0.5V 0.25V 0.025V using bit 3 and 4 (Refer to Spec page 16 - Table 5)
			use constants FULLSCALESELECT_0_5V, FULLSCALESELECT_0_25V, FULLSCALESELECT_0_125V
	integrator_ch1 - Channel 1 only has a selectable integrator (Refer to Spec page 16 - Table 5)
			use constants INTEGRATOR_OFF, INTEGRATOR_ON (bit-7 of CH1OS is used to set the integrator flag)
	--- Channel 1 is for Current - Channel 2 is for Voltage

meter.rmsSetup( 0, 0 );                 // IRMSOS,VRMSOS  12-bit (S) [-2048 +2048] -- Refer to spec page 25, 26

meter.energySetup(0, 0, 0, 0, 0, 0x0D); // WGAIN,WDIV,APOS,VAGAIN,VADIV,PHCAL  -- Refer to spec page 39, 31, 46, 44, 52, 53
	WGAIN  - Power Gain Adjust                - 12-bit (S) [-2048 +2048] 
	WDIV   - Active Energy Divider            -  8-bit (U) [0 +256]
	APOS   - Active Power Offset Correction   - 16-bit (S) [-32768 +32768]
	VAGAIN - Apparent Gain Register           - 12-bit (S) [-2048 +2048]
	VADIV  - Apparent Energy Divider Register -  8-bit (U) [0 +256]
	PHCAL  - Phase Calibration Register       -  6-bit (S) [0x1D  0x21] only valid content of this twos compliment register 
																		0x0D gives zero phase delay

meter.frequencySetup(0, 0);             // CFNUM,CFDEN  12-bit (U) -- for CF pulse output  -- Refer to spec page 31

meter.miscSetup(0, 0, 0, 0, 0, 0);
	ZXTOUT  - Zero-Crossing Timeout          - 12-bit (U) [0 +4096]
	SAGCYC  - Sag Line Cycle Register        -  8-bit (U) [0 +256]
	SAGLVL  - Sag Voltage Level              -  8-bit (U) [0 +256]
	IPKLVL  - Channel 1 Peak Level Threshold -  8-bit (U) [0 +256]
	VPKLVL  - Channel 2 Peak Level Threshold -  8-bit (U) [0 +256]
	TMODE   - Test Mode Register             -  8-bit (U) [0 +256]
*/

/** === setMode / getMode ===
* The MODE register is a 16-bit register through which most of the ADE7753 functionality is accessed.
* Signal sample rates, filter enabling, and calibration modes are selected by writing to this register.
* The contents can be read at any time.
*  
* The next table describes the functionality of each bit in the register:
* 
* Bit Location	/ Bit Mnemonic / Default Value Description			
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

/*
// RAM space is very limited so use PROGMEM to conserve memory with strings

// As Pachube feeds may hang at times, we reboot regularly. 
// We will monitor stability then remove reboot when OK.

// The last datastream ID6 is a health indicator. This is simply an incremeting number so any
// interruption can be easily addentified as a discontinuity on the ramp graph shown on Pachube

// Mercinat Etel test site Nanodes -- As we use one Nanode per sensor, making use of the MAC allows to assign IP by DHCP and identify each sensor
// -------------------------------
// Nanode: 1	Serial: 266	 Mac: 00:04:A3:2C:2B:D6 --> Aurora
// Nanode: 2	Serial: 267	 Mac: 00:04:A3:2C:30:C2 --> FemtoGrid
// Nanode: 3	Serial: 738	 Mac: 00:04:A3:2C:1D:EA --> Skystream
// Nanode: 4	Serial: 739	 Mac: 00:04:A3:2C:1C:AC --> Grid RMS #1
// Nanode: 5	Serial: 740	 Mac: 00:04:A3:2C:10:8E --> Grid RMS #2
// Nanode: 6	Serial: 835	 Mac: 00:04:A3:2C:28:FA -->  6 m anemometer/vane
// Nanode: 7	Serial: 836	 Mac: 00:04:A3:2C:26:AF --> 18 m anemometer/vane
// Nanode: 8	Serial: 837	 Mac: 00:04:A3:2C:13:F4 --> 12 m anemometer/vane
// Nanode: 9	Serial: 838	 Mac: 00:04:A3:2C:2F:C4 -->  9 m anemometer/vane

// Pachube feeds assignement
// -------------------------
// The is one Pachube feed per sensor. This appears to be the most flexible scheme.
// Derived/computed feeds/datastreams may be uploaded later via the Pachube API
// Will post such applications on project repository 
//  38277  - Anemometer/Vane Etel  6 m -- https://pachube.com/feeds/38277
//  38278  - Anemometer/Vane Etel  9 m -- https://pachube.com/feeds/38278
//  38279  - Anemometer/Vane Etel 12 m -- https://pachube.com/feeds/38279
//  38281  - Anemometer/Vane Etel 18 m -- https://pachube.com/feeds/38281
//  35020  - Skystream    (ADE7755 via CF & REVP - modified Hoch Energy Meter board) -- https://pachube.com/feeds/35020
//  37668  - WT FemtoGrid (ADE7755 via CF & REVP - modified Hoch Energy Meter board) -- Private feed -- https://pachube.com/feeds/37668
//  37667  - WT Aurora on (ADE7755 via CF & REVP - modified Hoch Energy Meter board) -- Private feed -- https://pachube.com/feeds/37667
//  40385  - Grid RMS #1  (ADE7753 via SPI - Olimex Energy Shield) -- https://pachube.com/feeds/40385
//  40386  - Grid RMS #2  (ADE7753 via SPI - Olimex Energy Shield) -- https://pachube.com/feeds/40386
//  40388 -- Turbine A -- Private feed -- https://pachube.com/feeds/40388
//  40389 -- Turbine B -- Private feed -- https://pachube.com/feeds/40389
//  37267 -- newly built Nanode test feed (Mercinat) -- https://pachube.com/feeds/37267
//  40447 -- Free room for anyone to test the ArduWind code)   -- https://pachube.com/feeds/40447
//  40448 -- Free room for anyone to test the ArduGrid code)   -- https://pachube.com/feeds/40448
//  40449 -- Free room for anyone to test the ArduSky code)    -- https://pachube.com/feeds/40449
//  40450 -- Free room for anyone to test the SkyChube code)   -- https://pachube.com/feeds/40450
//  40451 -- Free room for anyone to test the NanodeKit code)  -- https://pachube.com/feeds/40451
*/
/* Future updates planned
----------------------
-	Reduce RAM usage (RAM space is very limited on Arduino)

========================================================================================================*/

/* PROGMEM & pgmspace.h library
Store data in flash (program) memory instead of SRAM. There's a description of the various types of 
memory available on an Arduino board. The PROGMEM keyword is a variable modifier, it should be used 
only with the datatypes defined in pgmspace.h. It tells the compiler "put this information into flash 
memory", instead of into SRAM, where it would normally go. PROGMEM is part of the pgmspace.h library.
So you first need to include the library at the top your sketch.
*/
#include <avr/pgmspace.h>

/* EEPROM
Read and write bytes from/to EEPROM. EEPROM size: 1024 bytes on the ATmega328
An EEPROM write takes 3.3 ms to complete. The EEPROM memory has a specified life of 100,000 write/erase 
cycles, so you may need to be careful about how often you write to it.
*/
#include <inttypes.h>
#include <avr/eeprom.h>
class EEPROMClass
{
public:
	uint8_t read(int);
	void write(int, uint8_t);
};

uint8_t EEPROMClass::read(int address)
{
	return eeprom_read_byte((unsigned char *) address);
}

void EEPROMClass::write(int address, uint8_t value)
{
	eeprom_write_byte((unsigned char *) address, value);
}
EEPROMClass EEPROM;

#include <avr/wdt.h> // Watchdog timer

// ==================================
// -- Ethernet/Pachube section
// ==================================

#include <EtherCard.h>  // get latest version from https://github.com/jcw/ethercard
// EtherShield uses the enc28j60 IC (not the WIZnet W5100 which requires a different library)

#include <NanodeUNIO.h>   // get latest version from https://github.com/sde1000/NanodeUNIO 
// All Nanodes have a Microchip 11AA02E48 serial EEPROM chip
// soldered to the underneath of the board (it's the three-pin
// surface-mount device).  This chip contains a unique ethernet address
// ("MAC address") for the Nanode.
// To read the MAC address the library NanodeUNIO is needed

// If the above library has not yet been updated for Arduino1 (Rev01, not the beta 0022), 
// in the 2 files NanodeUNIO.h and NanoUNIO.cpp
// you will have to make the following modification:
//#if ARDUINO >= 100
//  #include <Arduino.h> // Arduino 1.0
//#else
//  #include <WProgram.h> // Arduino 0022+
//#endif

byte macaddr[6];  // Buffer used by NanodeUNIO library
NanodeUNIO unio(NANODE_MAC_DEVICE);
boolean bMac; // Success or Failure upon function return

// #define APIKEY  "xxx"  // Mercinat Pachube key
/* //#define APIKEY  "fqJn9Y0oPQu3rJb46l_Le5GYxJQ1SSLo1ByeEG-eccE"  // MercinatLabs FreeRoom Pachube key for anyone to test this code */

#define REQUEST_RATE 10000 // in milliseconds - Pachube update rate
unsigned long lastupdate = 0;  // timer value when last Pachube update was done
uint32_t timer = 0;            // a local timer
unsigned long PachubeResponseTime = 0; // Time between send to and response from Pachube
unsigned long pingtimer;   // ping timer

byte Ethernet::buffer[700];
Stash stash;     // For filling/controlling EtherCard send buffer using satndard "print" instructions

int MyNanode = 0;

// -------------------------------
// END -- Ethernet/Pachube section
// -------------------------------


// ===========================
// -- Energy Shield section
// ===========================

#include <Arduino.h>
#include "SPI.h"
#include "ADE7753.h"

// ----------------------------
// END -- Energy Shield Section
// ----------------------------

unsigned long TimeStampSinceLastReboot;

// Watchdog timer variables
// ------------------------
unsigned long previousWdtMillis = 0;
unsigned long wdtInterval = 0;

// **********************
// -- SETUP
// **********************

void setup(){

	ENC28J60 etherchip; // Instantiate class ENC28J60 to "chip"
	ADE7753 meter;      // Instantiate class ADE7753 to "meter"

	/* We always need to make sure the WDT is disabled immediately after a 
	* reset, otherwise it will continue to operate with default values.
	*/
	wdt_disable();

	Serial.begin(115200);
	delay( random(0,2000) ); // delay startup to avoid all Nanodes to collide on network access after a general power-up
	// and during Pachube updates								
	// WatchdogSetup();// setup Watch Dog Timer to 8 sec
	TimeStampSinceLastReboot = millis();
	pinMode(6, OUTPUT);
	for (int i=0; i < 10; i++) { digitalWrite(6,!digitalRead(6)); delay (50);} // blink LED 6 a bit to greet us after reboot

	showString(PSTR("\n\nArduGrid7753 V1.1 - MercinatLabs (14 Jan 2012)\n"));
	showString(PSTR("[SRAM available   ] = ")); Serial.println(freeRam());
	showString(PSTR("[Number of Reboots] = ")); Serial.println(EEPROM.read(0));
	showString(PSTR("[Watchdog Timeouts] = ")); Serial.println(EEPROM.read(1));
	EEPROM.write(0, EEPROM.read(0)+1 ); // Increment EEPROM for each reboot

	// ===========================
	// -- Energy Shield section
	// ===========================

	meter.setSPI();  // Initialise SPI communication ADE7753 IC
	//
	//// Use this function for identifying optimum CH1OS and CH2OS settings - both inputs 1 & 2 are shorted to ground
	//// Note: think of disabling High Pass Filter when doing DC offset input calibration
	//  meter.setSPI();  // Initialise SPI communication ADE7753 IC
	//  meter.setMode( CYCMODE + DISHPF ); // set mode for Line Cycle Accumulation
	//  TestInputOffset();

	//
	//// Use this function for setting the optimum IRMSOS and VRMSOS offests - to be done after setting CH1OS and CH2OS offsets using TestInputOffset();
	//// Note: think of disabling High Pass Filter when doing DC offset input calibration
	//  meter.setSPI();  // Initialise SPI communication ADE7753 IC
	//  meter.setMode( CYCMODE + DISHPF ); // set mode for Line Cycle Accumulation
	//  TestRMSoffset();
	//
	//// Use if you want to test writing to the registers and to display the content of the registers
	//  TestRegisters ();
	//

	// Settings for Olimex Energy Shield #1 - Etel
	// ------------------------------------
	meter.analogSetup(GAIN_4, GAIN_2, -3, -5, FULLSCALESELECT_0_5V, INTEGRATOR_OFF);  // GAIN1, GAIN2, CH1OS, CH2OS, Range_ch1, integrator_ch1
	meter.rmsSetup( -2000, +2000 );                 // IRMSOS,VRMSOS  12-bit (S) [-2048 +2048] -- Refer to spec page 25, 26 
	meter.energySetup(0, 0, 0, 0, 0, 0x0D); // WGAIN,WDIV,APOS,VAGAIN,VADIV,PHCAL  -- Refer to spec page 39, 31, 46, 44, 52, 53
	meter.frequencySetup(0, 0);             // CFNUM,CFDEN  12-bit (U) -- for CF pulse output  -- Refer to spec page 31
	meter.miscSetup(0, 0, 0, 0, 0, 0);

	//// Settings for Olimex Energy Shield #2
	//// ------------------------------------
	//  meter.analogSetup(GAIN_4, GAIN_2, -6, -1, FULLSCALESELECT_0_5V, INTEGRATOR_OFF);  // GAIN1, GAIN2, CH1OS, CH2OS, Range_ch1, integrator_ch1
	//  meter.rmsSetup( -2000, -2048 );                 // IRMSOS,VRMSOS  12-bit (S) [-2048 +2048] -- Refer to spec page 25, 26 
	//  meter.energySetup(0, 0, 0, 0, 0, 0x0D); // WGAIN,WDIV,APOS,VAGAIN,VADIV,PHCAL  -- Refer to spec page 39, 31, 46, 44, 52, 53
	//  meter.frequencySetup(0, 0);             // CFNUM,CFDEN  12-bit (U) -- for CF pulse output  -- Refer to spec page 31
	//  meter.miscSetup(0, 0, 0, 0, 0, 0);
	//          

	meter.closeSPI();  // Close SPI communication with ADE7753 IC

	// ----------------------------
	// END -- Energy Shield Section
	// ----------------------------


	// ==================================
	// -- Ethernet/Pachube section
	// ==================================

	GetMac(); // get MAC adress from the Microchip 11AA02E48 located at the back of the Nanode board

	// Identify which sensor is assigned to this board
	// If you have boards with identical MAC last 2 values, you will have to adjust your code accordingly
	switch ( macaddr[5] )
	{
	case 0xFA: MyNanode = 6;  showString(PSTR("n6 ")); showString(PSTR("f38277 - ")); showString(PSTR("Etel 6 m\n")) ; break; 
	case 0xC4: MyNanode = 9;  showString(PSTR("n9 ")); showString(PSTR("f38278 - ")); showString(PSTR("Etel 9 m\n")) ; break;
	case 0xF4: MyNanode = 8;  showString(PSTR("n8 ")); showString(PSTR("f38279 - ")); showString(PSTR("Etel 12 m\n")); break;
	case 0xAF: MyNanode = 7;  showString(PSTR("n7 ")); showString(PSTR("f38281 - ")); showString(PSTR("Etel 18 m\n")); break;
	case 0xD6: MyNanode = 1;  showString(PSTR("n1 ")); showString(PSTR("f37667 - ")); showString(PSTR("Aurora\n"))   ; break;
	case 0xC2: MyNanode = 2;  showString(PSTR("n2 ")); showString(PSTR("f37668 - ")); showString(PSTR("FemtoGrid\n")); break;
	case 0xEA: MyNanode = 3;  showString(PSTR("n3 ")); showString(PSTR("f35020 - ")); showString(PSTR("Skystream\n")); break;
	case 0xAC: MyNanode = 4;  showString(PSTR("n4 ")); showString(PSTR("f40385 - ")); showString(PSTR("Grid RMS #1\n")); break;
	case 0x8E: MyNanode = 5;  showString(PSTR("n5 ")); showString(PSTR("f40386 - ")); showString(PSTR("Grid RMS #2\n")); break;
	default:  
		showString(PSTR("unknown Nanode\n\r"));
		// assigned to NanodeKit free room - https://pachube.com/feeds/40451
		showString(PSTR("nx ")); showString(PSTR("f40451 - ")); showString(PSTR("NanoKit Free Room\r\n")); break; 
	}

	// Ethernet/Internet setup
	while (ether.begin(sizeof Ethernet::buffer, macaddr) == 0) { showString(PSTR( "Failed to access Ethernet controller\n")); }
	while (!ether.dhcpSetup()) { showString(PSTR("DHCP failed\n")); }
	ether.printIp("IP:  ", ether.myip);
	ether.printIp("GW:  ", ether.gwip);  
	ether.printIp("DNS: ", ether.dnsip); 
	while (!ether.dnsLookup(PSTR("api.pachube.com"))) { showString(PSTR("DNS failed\n")); }
	ether.printIp("SRV: ", ether.hisip);  // IP for Pachupe API found by DNS service

	meter.closeSPI();  // Close SPI communication with ADE7753 IC

	// -------------------------------
	// END -- Ethernet/Pachube section
	// -------------------------------

	WatchdogSetup();// setup Watch Dog Timer to 8 sec
	wdt_reset();

}


/*****************************
		Main Loop     
******************************/
void loop() {
	// Reset watch dog timer to prevent timeout
	wdt_reset();

	// Use this endless loop hereunder when you want to verify the effect of the Watchdog timeout
	// while(1) { digitalWrite(6,!digitalRead(6)); delay (100); showString(PSTR("+")); }


	ENC28J60 etherchip; // Instantiate class ENC28J60 to "chip"
	ADE7753 meter;      // Instantiate class ADE7753 to "meter"
	int j = 0;  

	float Vrms 	= 0;
	float Irms   = 0;
	float Vpeak  = 0;
	float Ipeak  = 0;
	int Temp 	  	= 0;
	float Frequency = 0;
	float ActiveEnergy 		= 0;
	float ApparentEnergy 	= 0;
	float ReactiveEnergy 	= 0;

	// Energy shield #1 - ETEL
	float calVrms 	  = 12498.65;
	float calIrms 	  = 167623.8;
	float calVpeak 	  = 141.0;
	float calIpeak 	  = 234565.0;
	float calTemp 	  = 1.0;
	float calActiveEnergy 	= 34.8;
	float calApparentEnergy = 30.4;
	float calReactiveEnergy = 0.60;

	//	// Energy shield #2
	//	float calVrms 	  = 12225.0;
	//	float calIrms 	  = 169192.0;
	//	float calVpeak 	  = 138.39;
	//	float calIpeak 	  = 233518.20;
	//	float calTemp 	  = 1;
	//	float calActiveEnergy 	= 67.28;
	//	float calApparentEnergy = 58.57;
	//	float calReactiveEnergy = 1.40;

	unsigned long lastupdate2 = 0;  // timer value for Energy Shield timeout

	Serial.println("-> main loop"); 
	etherchip.initSPI();

	while ( j < 180 )  // As Pachube feeds may hang at times, reboot regularly. We will monitor stability then remove reboot when OK
	// a value of 180 with an update to Pachube every 10 seconds provoque a reboot every 30 mn. Reboot is very fast.
	{

		//	Serial.println("-> receiving"); 
		wdt_reset();
		ether.packetLoop(ether.packetReceive());  // check response from Pachube
		delay(100);
		showString(PSTR("."));
		
		if ( ( millis()-lastupdate ) > REQUEST_RATE )
		{
			lastupdate = millis();
			timer = lastupdate;
			j++;

			showString(PSTR("\n************************************************************************************************\n"));    
			showString(PSTR("\nStarting Pachube update loop --- "));
			showString(PSTR("[memCheck bytes] ")); Serial.print(freeRam());
			showString(PSTR(" -- [Reboot Time Stamp] "));
			Serial.println( millis() - TimeStampSinceLastReboot );
			showString(PSTR("-> Check response from Pachube\n"));
			
			// DHCP expiration is a bit brutal, because all other ethernet activity and
			// incoming packets will be ignored until a new lease has been acquired
			//    showString(PSTR("-> DHCP? ")); 
			//    if (ether.dhcpExpired() && !ether.dhcpSetup())
			//      showString(PSTR("DHCP failed\n"));
			//   showString(PSTR("is fine\n")); 


			// ping server 
			ether.printIp("-> Pinging: ", ether.hisip);
			pingtimer = micros();
			ether.clientIcmpRequest(ether.hisip);
			if ( ( ether.packetReceive() > 0 ) && ether.packetLoopIcmpCheckReply(ether.hisip) ) 
			{
				showString(PSTR("-> ping OK = "));
				Serial.print((micros() - pingtimer) * 0.001, 3);
				showString(PSTR(" ms\n"));
			} 
			else 
			{
				showString(PSTR("-> ping KO = "));
				Serial.print((micros() - pingtimer) * 0.001, 3);
				showString(PSTR(" ms\n"));
			}
			
			// DHCP expiration is a bit brutal, because all other ethernet activity and
			// incoming packets will be ignored until a new lease has been acquired
			wdt_reset();
			if ( ether.dhcpExpired() && !ether.dhcpSetup() )
			{ 
				showString(PSTR("DHCP failed\n"));
				delay (200); // delay to let the serial port buffer some time to send the message before rebooting
				software_Reset() ;  // Reboot so can a new lease can be obtained
			}



			meter.closeSPI();  // Close SPI communication with ADE7753 IC
			
			// ==================================
			// -- Energy Shield section
			// ==================================
			Serial.println("\n-> measurement cycle");
			meter.setSPI();  // Initialise SPI communication ADE7753 IC
			meter.setMode( CYCMODE ); // set mode for Line Cycle Accumulation
			meter.setLineCyc(200);    // set Line Cycle to 200 * 10 ms = 2 sec (at 50Hz, half line cycle = 10 ms)
			meter.setInterruptsMask(0xFF); // enable all interrupts (useless as only affects IRQ signal, has no effect in status register when using poll mode)
			// >>> Warning <<< The flag bits in the status register are set irrespective of the state of the enable bits.
			// Therefore as IRQ signal is not wired, we have to poll the status register for a selected interrupt with its bit mask
			meter.getresetInterruptStatus(); // Clear all interrupts


			lastupdate2 = millis();
			while( ! ( 
			( meter.getInterruptStatus() & CYCEND ) // wait for end of accumulation cycle
			|| 
			( meter.getInterruptStatus() & ZXTO )  // fall through if missing zero crossing (i.e. no grid input)
			)
			)
			{   // wait for the selected interrupt to occur or timeout
				if ( ( millis() - lastupdate2 ) > 2500)
				{ 
					Serial.println("--> Timeout"); 
					meter.getresetInterruptStatus(); // Clear all interrupts
					break;  
				} 
			}  

			////  // Do it again to discard first set of data because the first line cycle accumulation results 
			////  // may not have used the accumulation time set by the LINECYC register and should be discarded.
			////  // Refer to spec page 40-41
			////  meter.getresetInterruptStatus(); // Clear all interrupts 
			////  lastupdate = millis(); 
			////  while( ! ( 
			////               ( meter.getInterruptStatus() & CYCEND ) // wait for end of accumulation cycle
			////                  || 
			////               ( meter.getInterruptStatus() & ZXTO )  // fall through if missing zero crossing (i.e. no grid input)
			////            )
			////       )
			////          {   // wait for the selected interrupt to occur or timeout
			////              if ( ( millis() - lastupdate ) > 5500) 
			////               { 
			////                 Serial.println("\n>>> Timeout no AC input"); 
			////                 meter.getresetInterruptStatus(); // Clear all interrupts
			////                 break;  
			////               } 
			////          } 

			Vrms 	  = meter.vrms() ;
			Irms 	  = meter.irms() ;
			Vpeak 	  = meter.getVpeakReset() ;
			Ipeak 	  = meter.getIpeakReset() ;
			Temp 	  = meter.getTemp();
			Frequency = float(CLKIN/4) / float(meter.getPeriod());
			ActiveEnergy 	= meter.getActiveEnergyLineSync()  ;
			ApparentEnergy 	= meter.getApparentEnergyLineSync()  ;
			ReactiveEnergy 	= meter.getReactiveEnergyLineSync()  ;


			Serial.println("--> before calibration"); 
			Serial.print(" VRMS_100: ");  Serial.println( Vrms,DEC );
			Serial.print(" IRMS_100: ");  Serial.println( Irms,DEC );
			Serial.print(" Vpeak   : ");  Serial.println( Vpeak,DEC );
			Serial.print(" Ipeak   : ");  Serial.println( Ipeak,DEC );
			Serial.print(" Freq (Hz): "); Serial.println( Frequency, DEC );
			Serial.print(" Temp: ");      Serial.println( meter.getTemp(), DEC );
			Serial.print(" ActiveEnergy  : ");      Serial.println( ActiveEnergy, DEC );
			Serial.print(" ApparentEnergy: ");      Serial.println( ApparentEnergy, DEC );
			Serial.print(" ReactiveEnergy: ");      Serial.println( ReactiveEnergy, DEC );

			Vrms 	  = Vrms / calVrms ;
			Irms 	  = Irms / calIrms ;
			Vpeak 	  = Vpeak / calVpeak ;
			Ipeak 	  = Ipeak / calIpeak ;
			Temp 	  = Temp / calTemp ;
			Frequency = float(CLKIN/4) / float(meter.getPeriod());
			ActiveEnergy 	= ActiveEnergy / calActiveEnergy ;
			ApparentEnergy 	= ApparentEnergy / calApparentEnergy ;
			ReactiveEnergy 	= ReactiveEnergy / calReactiveEnergy ;
			//
			Serial.println("--> after calibration"); 
			Serial.print(" VRMS_100: ");  Serial.println( Vrms,DEC );
			Serial.print(" IRMS_100: ");  Serial.println( Irms,DEC );
			Serial.print(" Vpeak   : ");  Serial.println( Vpeak,DEC );
			Serial.print(" Ipeak   : ");  Serial.println( Ipeak,DEC );
			Serial.print(" Freq (Hz): "); Serial.println( Frequency, DEC );
			Serial.print(" Temp: ");      Serial.println( meter.getTemp(), DEC );
			Serial.print(" ActiveEnergy  : ");      Serial.println( ActiveEnergy, DEC );
			Serial.print(" ApparentEnergy: ");      Serial.println( ApparentEnergy, DEC );
			Serial.print(" ReactiveEnergy: ");      Serial.println( ReactiveEnergy, DEC );
			
			meter.closeSPI();  // Close SPI communication with ADE7753 IC
			
			// ----------------------------
			// END -- Energy Shield Section
			// ----------------------------	


			// ==================================
			// -- Ethernet/Pachube section
			// ==================================

			etherchip.initSPI();

			
			// DHCP expiration is a bit brutal, because all other ethernet activity and
			// incoming packets will be ignored until a new lease has been acquired
			showString(PSTR("-> DHCP? ")); 
			wdt_reset();
			if ( ether.dhcpExpired() && !ether.dhcpSetup() )
			{ 
				showString(PSTR("DHCP failed\n"));
				delay (200); // delay to let the serial port buffer some time to send the message before rebooting
				software_Reset() ;  // Reboot so can a new lease can be obtained
			}
			showString(PSTR("is fine\n")); 



			// Prepare string to send to Pachube
			// *********************************

			byte sd = stash.create();  // Initialise send data buffer
			
			stash.print("0,"); // Datastream 0
			stash.println( Vrms );

			stash.print("1,"); // Datastream 1
			stash.println( Irms );

			stash.print("2,"); // Datastream 2
			stash.println( Vpeak );

			stash.print("3,"); // Datastream 3
			stash.println( Ipeak );

			stash.print("4,");
			stash.println( ActiveEnergy );

			stash.print("5,");
			stash.println( ApparentEnergy );

			stash.print("6,");
			stash.println( ReactiveEnergy );

			stash.print("7,");
			stash.println( Temp );

			stash.print("8,");
			stash.println( Frequency );

			//   stash.print("9,");
			//   stash.println(  );

			stash.print("10,"); // Datastream 10 - Nanode Health
			stash.println( j );
			
			stash.print("11,");  // Datastream 11 - Nbr of REBOOTs
			stash.println( EEPROM.read(0) );
			
			stash.print("12,");  // Datastream 12 - Nbr of WATCHDOG TIMEOUTs
			stash.println( EEPROM.read(1)  );
			
			stash.save(); // Close streaming send data buffer

			// Select the destination feed according to what the Nanode board is assigned to    
			switch ( MyNanode )
			{
			case 4: // Grid RMS #1
				Stash::prepare(PSTR("PUT http://$F/v2/feeds/$F.csv HTTP/1.0" "\r\n"
				"Host: $F" "\r\n"
				"X-PachubeApiKey: $F" "\r\n"
				"Content-Length: $D" "\r\n"
				"\r\n"
				"$H"),
				PSTR("api.pachube.com"), PSTR("40385"), PSTR("api.pachube.com"), PSTR(APIKEY), stash.size(), sd);
				break;
				
			case 5: // Grid RMS #2
				Stash::prepare(PSTR("PUT http://$F/v2/feeds/$F.csv HTTP/1.0" "\r\n"
				"Host: $F" "\r\n"
				"X-PachubeApiKey: $F" "\r\n"
				"Content-Length: $D" "\r\n"
				"\r\n"
				"$H"),
				PSTR("api.pachube.com"), PSTR("40386"), PSTR("api.pachube.com"), PSTR(APIKEY), stash.size(), sd);
				break;
				
			default: // ArduGrid Free Room on Pachube -- https://pachube.com/feeds/40447
				Stash::prepare(PSTR("PUT http://$F/v2/feeds/$F.csv HTTP/1.0" "\r\n"
				"Host: $F" "\r\n"
				"X-PachubeApiKey: $F" "\r\n"
				"Content-Length: $D" "\r\n"
				"\r\n"
				"$H"),
				PSTR("api.pachube.com"), PSTR("40447"), PSTR("api.pachube.com"), PSTR(APIKEY), stash.size(), sd);
				break;
			}
			
			// send the packet - this also releases all stash buffers once done
			showString(PSTR("-> sending\n"));
			ether.tcpSend();
			showString(PSTR("-> done sending\n"));
			delay (1000);
			//    meter.closeSPI();  // Close SPI communication with ADE7753 IC

			// blink LED 6 a bit to show some activity on the board when sending to Pachube       
			for (int i=0; i < 4; i++) { digitalWrite(6,!digitalRead(6)); delay (50);} 
		}

		// -------------------------------
		// END -- Ethernet/Pachube section
		// -------------------------------

	} // This is the end of the WHILE ( j < 180 ) loop

	// ==================================
	// -- This is the end of the main loop
	// ====================================
	// reboot now to clean all dirty buffers to avoid Pachube feed hanging.
	showString(PSTR("-- rebooting --\n")); delay (250); 
	software_Reset() ;

} // -- END of main loop


/*
---------------------------------
	Additional functions     
---------------------------------
*/

#define NanodeReduceCodeSize 1
#ifndef NanodeReduceCodeSize

// Register R/W testing
// --------------------
void TestRegisters (void) {

	ADE7753 meter; // Instantiate class ADE7753 to "meter"
	
	// Hereunder is a testing set to easily verify all the registers Write then Read, and read Modes
	// ---------------------------------------------------------------------------------------------
	showString(PSTR("-- after Reboot\n"));
	meter.printGetResetInterruptStatus(); // RESET bit 6 is set to indicates the end of a reset (for both software or hardware reset)
	showString(PSTR("-- after Status Read-Reset \n"));
	meter.printGetResetInterruptStatus(); // should be all zeros now

	Serial.println("===");
	meter.printGetMode();

	meter.setMode( CYCMODE + TEMPSEL ); // set mode for Line Cycle Accumulation + Temperature reading
	meter.setLineCyc(500);    // set Line Cycle to 500 * 10 ms = 5 sec (at 50Hz, half line cycle = 10 ms)
	meter.setInterruptsMask(0xFF); // enable all interrupts (useless as only affects IRQ signal, has no effect in status register when using poll mode)
	meter.getresetInterruptStatus(); // Clear all interrupts
	meter.analogSetup(GAIN_16, GAIN_16, -22, -23, FULLSCALESELECT_0_125V, INTEGRATOR_ON);
	meter.rmsSetup(-2003,-2004); // IRMSOS,VRMSOS  12-bit (S) [-2048 +2048] -- Refer to spec page 25, 26 
	meter.frequencySetup(2005,2006);
	meter.miscSetup(2000, 101, 102, 103, 104, 105);
	meter.energySetup(-2000, 200, -30000, -2001, 201, 0x21);
	Serial.println("---");  
	meter.printAllRegisters();
}

// Test CH1OS and CH2OS offsets
// ----------------------------
// To help for selecting optimum CH1OS and CH2OS offsets. Inputs 1 and 2 must be shorted to ground.

void TestInputOffset (void) {

	// Because the inputs are shorted to ground, readings cannot be synchronized with the zero crossing of the AC grid
	// voltage in order to minimize noise. Therefore a large number of samples are averaged in this offset calculation
	// procedure. The IRMS and VRMS registers are read directly.

	ADE7753 meter; // Instantiate class ADE7753 to "meter"
	long Current = 0;
	long Voltage = 0;
	
	for ( int i=-32; i < 33; i++ )
	{      
		meter.analogSetup(GAIN_1, GAIN_1, char (i), char (i), FULLSCALESELECT_0_5V, INTEGRATOR_OFF);  // GAIN1, GAIN2, CH1OS, CH2OS, Range_ch1, integrator_ch1
		
		Serial.println(i);
		
		showString(PSTR("-> TestInputOffset\n"));
		Current = meter.read24(IRMS);
		Voltage = meter.read24(VRMS);
		
		for ( int n = 0; n < 1000; n++ )
		{      
			Current = Current + meter.read24(IRMS);
			Voltage = Voltage + meter.read24(VRMS);
		}

		showString(PSTR("Averaged getIRMS: "));
		Serial.print(Current/1000,DEC);
		Serial.println("");
		
		showString(PSTR("Averaged getVRMS: "));
		Serial.print(Voltage/1000,DEC);
		Serial.println("");
	} 
}


// Test RMS offsets - to be done after setting CH1OS and CH2OS offsets using TestInputOffset();
// -----------------
// To help for selecting optium IRMSOS and VRMSOS offsets. Inputs 1 and 2 must be shorted to ground.

void TestRMSoffset (void) {

	// Because the inputs are shorted to ground, readings cannot be synchronized with the zero crossing of the AC grid
	// voltage in order to minimize noise. Therefore a large number of samples are averaged in this offset calculation
	// procedure. The IRMS and VRMS registers are read directly.

	ADE7753 meter; // Instantiate class ADE7753 to "meter"
	long Current = 0;
	long Voltage = 0;
	
	// use CH1OS, CH2OS selected when running TestInputOffset();
	meter.analogSetup(GAIN_4, GAIN_2, -3, -5, FULLSCALESELECT_0_5V, INTEGRATOR_OFF);  // GAIN1, GAIN2, CH1OS, CH2OS, Range_ch1, integrator_ch1
	
	// Re-run with various IRMSOS,VRMSOS setting until offset is cancelled
	meter.rmsSetup( -2000, +2008 );   // IRMSOS,VRMSOS  12-bit (S) [-2048 +2048] -- Refer to spec page 25, 26 

	showString(PSTR("-> TestRMSoffset\n"));
	Current = meter.read24(IRMS);
	Voltage = meter.read24(VRMS);
	
	for ( long int n = 0; n < 10000; n++ )
	{      
		Current = Current + meter.read24(IRMS);
		Voltage = Voltage + meter.read24(VRMS);
	}

	showString(PSTR("Averaged getIRMS: "));
	Serial.print(Current/10000,DEC);
	Serial.println("");
	
	showString(PSTR("Averaged getVRMS: "));
	Serial.print(Voltage/10000,DEC);
	Serial.println("");
	
	showString(PSTR("getIRMS: "));
	Serial.print(meter.getIRMS(),DEC); // rms measurement update rate is CLKIN/4.
	Serial.println("");
	
	showString(PSTR("getVRMS: "));
	Serial.print(meter.getVRMS(),DEC);  // rms measurement update rate is CLKIN/4.
	Serial.println("");
	
	showString(PSTR("IRMS_100: "));
	Serial.print(meter.irms(),DEC);
	Serial.println("");
	
	showString(PSTR("VRMS_100: "));
	Serial.print(meter.vrms(),DEC);
	Serial.println("");
	
	showString(PSTR(("---\n")); 
	meter.printAllRegisters();
}

#endif

// ++++++++++++++++
//    FUNCTIONS 2
// ++++++++++++++++

// Determines how much RAM is currently unused
int freeRam () {
	extern int __heap_start, *__brkval; 
	int v; 
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

// Display string stored in PROGMEM
void showString (PGM_P s)
{
	char c;
	while ((c = pgm_read_byte(s++)) != 0)
	Serial.print(c);
}

void GetMac()
{
	showString(PSTR("Reading MAC address... "));
	bMac=unio.read(macaddr,NANODE_MAC_ADDRESS,6);
	if (bMac) showString(PSTR("success\n\r"));
	else showString(PSTR("failure\n\r"));
	
	showString(PSTR("MAC     : "));
	for (int i=0; i<6; i++) 
	{
		if (macaddr[i]<16) 
		{
			showString(PSTR("0"));
		}
		Serial.print(macaddr[i], HEX);
		if (i<5) 
		{
			showString(PSTR(":"));
		} 
		else 
		{
			showString(PSTR(""));
		}
	}
	showString(PSTR("\n--\n"));
}

void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
	asm volatile ("  jmp 0");  
} 

//initialize watchdog
void WatchdogSetup(void)
{
#define WDPS_16MS   (0<<WDP3 )|(0<<WDP2 )|(0<<WDP1)|(0<<WDP0) 
#define WDPS_32MS   (0<<WDP3 )|(0<<WDP2 )|(0<<WDP1)|(1<<WDP0) 
#define WDPS_64MS   (0<<WDP3 )|(0<<WDP2 )|(1<<WDP1)|(0<<WDP0) 
#define WDPS_125MS  (0<<WDP3 )|(0<<WDP2 )|(1<<WDP1)|(1<<WDP0) 
#define WDPS_250MS  (0<<WDP3 )|(1<<WDP2 )|(0<<WDP1)|(0<<WDP0) 
#define WDPS_500MS  (0<<WDP3 )|(1<<WDP2 )|(0<<WDP1)|(1<<WDP0) 
#define WDPS_1S     (0<<WDP3 )|(1<<WDP2 )|(1<<WDP1)|(0<<WDP0) 
#define WDPS_2S     (0<<WDP3 )|(1<<WDP2 )|(1<<WDP1)|(1<<WDP0) 
#define WDPS_4S     (1<<WDP3 )|(0<<WDP2 )|(0<<WDP1)|(0<<WDP0) 
#define WDPS_8S     (1<<WDP3 )|(0<<WDP2 )|(0<<WDP1)|(1<<WDP0) 

	//disable interrupts
	cli();
	//reset watchdog
	wdt_reset();
	
	/* Setup Watchdog by writing to WDTCSR = WatchDog Timer Control Register */ 
	WDTCSR = (1<<WDCE)|(1<<WDE); // Set Change Enable bit and Enable Watchdog System Reset Mode. 
	
	// Set Watchdog prescalar as desired
	WDTCSR = (1<<WDIE)|(1<<WDIF)|(1<<WDE )| WDPS_8S; 

	
	//Enable global interrupts
	sei();
	
	showString(PSTR(">>> Watchdog has been initialized\n"));
}

void WatchdogClear(void)
{
	//disable interrupts
	cli();
	//reset watchdog
	wdt_reset();
	
	/* Setup Watchdog by writing to WDTCSR = WatchDog Timer Control Register */ 
	WDTCSR = (1<<WDCE)|(1<<WDE); // Set Change Enable bit and Enable Watchdog System Reset Mode. 	
}

//Watchdog timeout ISR
ISR(WDT_vect)
{
	WatchdogSetup(); // If not there, cannot print the message before rebooting
	EEPROM.write(1, EEPROM.read(1)+1 );  // Increment EEPROM for each WatchDog Timeout
	showString(PSTR("\nREBOOTING....\n\n"));
	
	// Time out counter in CPU EEPROM
	// to be implemened soonest
	
	delay(1000); // leave some time for printing on serial port to complete
	software_Reset();	
}



