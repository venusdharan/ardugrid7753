/* ADE7753.h = Class constructor and function implementation for Olimex Energy Shield using ADE7753)
====================================================================================================
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

#ifndef ADE7753_H
#define ADE7753_H

/***
 * Defines
 *
 */
 
//Register address
#define WAVEFORM   0x01
#define AENERGY    0x02
#define RAENERGY   0x03
#define LAENERGY   0x04
#define VAENERGY   0x05
#define RVAENERGY  0x06
#define LVAENERGY  0x07
#define LVARENERGY 0x08
#define MODE       0x09
#define IRQEN      0x0A   // interrupts enable register
#define STATUS     0x0B   // interrupts status register
#define RSTSTATUS  0x0C   // interrupts status register but read will reset all interrupt flags
#define CH1OS      0x0D
#define CH2OS      0x0E
#define GAIN       0x0F
#define PHCAL      0x10
#define APOS       0x11
#define WGAIN      0x12
#define WDIV       0x13
#define CFNUM      0x14
#define CFDEN      0x15
#define IRMS       0x16
#define VRMS       0x17
#define IRMSOS     0x18
#define VRMSOS     0x19
#define VAGAIN     0x1A
#define VADIV      0x1B
#define LINECYC    0x1C
#define ZXTOUT     0x1D
#define SAGCYC     0x1E
#define SAGLVL     0x1F
#define IPKLVL     0x20
#define VPKLVL     0x21
#define IPEAK      0x22
#define RSTIPEAK   0x23
#define VPEAK      0x24
#define RSTVPEAK   0x25
#define TEMP       0x26
#define PERIOD     0x27
#define TMODE      0x3D
#define CHKSUM     0x3E
#define DIEREV     0X3F


//bits

// MODE register - Bit Location
#define DISHPF   0x0001 // bit 0 - HPF (high-pass filter) in Channel 1 is disabled when this bit is set.
#define DISLPF2  0x0002 // bit 1 - LPF (low-pass filter) after the multiplier (LPF2) is disabled when this bit is set.
#define DISCF    0x0004 // bit 2 - Frequency output CF is disabled when this bit is set.
#define DISSAG   0x0008 // bit 3 - Line voltage sag detection is disabled when this bit is set.
#define ASUSPEND 0x0010 // bit 4 - By setting this bit to Logic 1, both ADE7753 A/D converters can be turned off. In normal operation, this bit should be left at Logic 0. All digital functionality can be stopped by suspending the clock signal at CLKIN pin.
#define TEMPSEL  0x0020 // bit 5 - Temperature conversion starts when this bit is set to 1. This bit is automatically reset to 0 when the temperature conversion is finished.
#define SWRST    0x0040 // bit 6 - Software Chip Reset. A data transfer should not take place to the ADE7753 for at least 18 us after a software reset.
#define CYCMODE  0x0080 // bit 7 - Setting this bit to Logic 1 places the chip into line cycle energy accumulation mode.
#define DISCH1   0x0100 // bit 8 - ADC 1 (Channel 1) inputs are internally shorted together.
#define DISCH2   0x0200 // bit 9 - ADC 2 (Channel 2) inputs are internally shorted together.
#define SWAP     0x0400 // bit 10 - By setting this bit to Logic 1 the analog inputs V2P and V2N are connected to ADC 1 and the analog inputs V1P and V1N are connected to ADC 2.
#define DTRT1    0x0800 // bit 11 - These bits are used to select the waveform register update rate.
#define DTRT0    0x1000 // bit 12 - These bits are used to select the waveform register update rate.
#define WAVSEL1  0x2000 // bit 13 - These bits are used to select the source of the sampled data for the waveform register.
#define WAVSEL0  0x4000 // bit 14 - These bits are used to select the source of the sampled data for the waveform register.
#define POAM     0x8000 // bit 15 - Writing Logic 1 to this bit allows only positive active power to be accumulated in the ADE7753.

						 // bit 12, 11  DTRT1,0      0	        These bits are used to select the waveform register update rate.		
						 // * 				        DTRT 1	DTRT0	Update Rate
						 // * 				            0	0	27.9 kSPS (CLKIN/128)
						 // * 				            0	1	14 kSPS (CLKIN/256)
						 // * 				            1	0	7 kSPS (CLKIN/512)
						 // * 				            1	1	3.5 kSPS (CLKIN/1024)

						 // bit  14, 13  WAVSEL1,0	0	        These bits are used to select the source of the sampled data for the waveform register.		
						 // * 			                  WAVSEL1, 0	Length	Source
						 // * 			                  0	        0	24 bits active power signal (output of LPF2)
						 // * 			                  0	        1	Reserved
						 // * 			                  1	        0	24 bits Channel 1
						 // * 			                  1	        1	24 bits Channel 2

/** === INTERRUPT STATUS REGISTER (0x0B), RESET INTERRUPT STATUS REGISTER (0x0C), INTERRUPT ENABLE REGISTER (0x0A) ===
The status register is used by the MCU to determine the source of an interrupt request (IRQ). 
When an interrupt event occurs in the ADE7753, the corresponding flag in the interrupt status 
register is set to logic high. If the enable bit for this flag is Logic 1 in the interrupt 
enable register, the IRQ logic output goes active low. When the MCU services the interrupt, 
it must first carry out a read from the interrupt status register to 
determine the source of the interrupt.
**/

// The next table summarizes the function of each bit for
// the Interrupt Status Register, the Reset Interrupt Status Register, and the Interrupt Enable Register

//             Bit Mask // Bit Location / Description			
#define AEHF      0x0001 // bit 0 - Indicates that an interrupt occurred because the active energy register, AENERGY, is more than half full.
#define SAG       0x0002 // bit 1 - Indicates that an interrupt was caused by a SAG on the line voltage.
#define CYCEND    0x0004 // bit 2 - Indicates the end of energy accumulation over an integer number of half line cycles as defined by the content of the LINECYC register�see the Line Cycle Energy Accumulation Mode section.
#define WSMP      0x0008 // bit 3 - Indicates that new data is present in the waveform register.
#define ZX        0x0010 // bit 4 - This status bit is set to Logic 0 on the rising and falling edge of the the voltage waveform. See the Zero-Crossing Detection section.
#define TEMPREADY 0x0020 // bit 5 - Indicates that a temperature conversion result is available in the temperature register.
#define RESET     0x0040 // bit 6 - Indicates the end of a reset (for both software or hardware reset). The corresponding enable bit has no function in the interrupt enable register, i.e., this status bit is set at the end of a reset, but it cannot be enabled to cause an interrupt.
#define AEOF      0x0080 // bit 7 - Indicates that the active energy register has overflowed.
#define PKV       0x0100 // bit 8 - Indicates that waveform sample from Channel 2 has exceeded the VPKLVL value.
#define PKI       0x0200 // bit 9 - Indicates that waveform sample from Channel 1 has exceeded the IPKLVL value.
#define VAEHF     0x0400 // bit 10 - Indicates that an interrupt occurred because the active energy register, VAENERGY, is more than half full.
#define VAEOF     0x0800 // bit 11 - Indicates that the apparent energy register has overflowed.
#define ZXTO      0x1000 // bit 12 - Indicates that an interrupt was caused by a missing zero crossing on the line voltage for the specified number of line cycles�see the Zero-Crossing Timeout section.
#define PPOS      0x2000 // bit 13 - Indicates that the power has gone from negative to positive.
#define PNEG      0x4000 // bit 14 - Indicates that the power has gone from positive to negative.
#define RESERVED  0x8000 // bit 15 - Reserved.

						 
//constants
#define GAIN_1    0x0
#define GAIN_2    0x1
#define GAIN_4    0x2
#define GAIN_8    0x3
#define GAIN_16   0x4
#define INTEGRATOR_ON  1
#define INTEGRATOR_OFF 0
#define FULLSCALESELECT_0_5V    0x00
#define FULLSCALESELECT_0_25V   0x01
#define FULLSCALESELECT_0_125V  0x02

// Class Atributes
#define CS 10                // Chip Select ADE7753 - Digital output pin nbr on Olimex Energy Shield  
#define WRITE 0x80           // WRITE bit BT7 to write to registers
#define CLKIN 4000000        // ADE7753 frec, 4.000000MHz
//The cristal associated to the for the ADE7753 the Olimex energy shield is 4.000000 MHz . Therefore 
//the CLKIN Frequency 3.579545 MHz mentionned in the specification is not applicable, and all the 
//calculations linked to the clock need to be reviewed.


class ADE7753 {
   //public methods
   public:
      void setSPI(void);
      void closeSPI(void);
      
      void setMode(int m);
      int  getMode(void);
      void setInterruptsMask(int i);
      int  getEnabledInterrupts(void);
      int  getInterruptStatus(void);
      int  getresetInterruptStatus(void);
      
      void printGetResetInterruptStatus(void);
      void printGetMode(void);
      void printAllRegisters(void);

      long getActiveEnergyLineSync(void); 
      long getApparentEnergyLineSync(void);
      long getReactiveEnergyLineSync(void);
	  
      long getVRMS(void);  
      long getIRMS(void);  
      long vrms();
      long irms();
      long getIpeak(void);
      long getIpeakReset(void);
      long getVpeak(void);
      long getVpeakReset(void);

     long getWaveform(void);
     char getTemp(void);
     int getPeriod(void);
      
      void setPowerOffset(int d);
      void setPhaseCalibration(char d);
      void setEnergyGain(char d);
      void setActivePowerGain(int d);
      void setActiveEnergyDivider(char d);
      void setApparentPowerGain(int d);
      void setApparentEnergyDivider(char d);
      void setZeroCrossingTimeout(int d);
      void setSagVoltageLevel(char d);
      void setSagCycles(char d);
      void setIPeakLevel(char d);
      void setVPeakLevel(char d);
      void setLineCyc(int d);
       
      void analogSetup(char gain_ch1, char gain_ch2,char os_ch1,char os_ch2,char scale_ch1,char integrator_ch1);
      void frequencySetup(int cfnum, int cfden);
      void energySetup(int wgain, char wdiv, int apos, int vagain, char vadiv, char phcal);
      void rmsSetup(int vrmsos, int irmsos);
      void energyGain(int wgain, int vagain);
      void miscSetup(int zxtout, char sagsyc, char saglvl, char ipklvl, char vpklvl, char tmode);
    // ZXTOUT 12-bit (U) - Zero-Crossing Timeout
    // SAGCYC  8-bit (U) - Sag Line Cycle Register.
    // SAGLVL  8-bit (U) - Sag Voltage Level.
    // IPKLVL  8-bit (U) - Channel 1 Peak Level Threshold
    // VPKLVL  8-bit (U) - Channel 2 Peak Level Threshold
    // TMODE   8-bit (U) - Test Mode Register

 
//   //private methods
//   private:
      unsigned char read8(char reg);
      unsigned int read16(char reg);
      unsigned long read24(char reg);
      void write16(char reg, unsigned int data);
      void write8(char reg, unsigned char data);
      void enableChip(void);  
      void disableChip(void);
      long waitInterrupt(unsigned int interrupt);
};

#define NanodeReduceCodeSize 1
#ifndef NanodeReduceCodeSize
	  
      long getActiveEnergy(void);
      long getActiveEnergyReset(void);
      long getApparentEnergy(void);
      long getApparentEnergyReset(void);
      char chkSum(void);
      char getCH1Offset(void);
      char getCH2Offset(void);
      char getPhaseCalibration(void);
      char getEnergyGain(void);
      char getActiveEnergyDivider(void);
      char getApparentEnergyDivider(void);
      char getSagCycles(void);
      char getSagVoltageLevel(void);
      char getIPeakLevel(void);
      char getVPeakLevel(void);
      
      int getWattGain(int load);
      int getVoltageOffset(void);
      int getCurrentOffset(void);     
      int getActivePowerOffset(void);
      int getPowerOffset(void);
      int getActivePowerGain(void);
      int getFrequencyDividerNumerator(void);
      int getFrequencyDividerDenominator(void);
      int getZeroCrossingTimeout(void);
      int getApparentPowerGain(void);
      int getWattGain();
      int getLineCyc();
      
      // void calibrateEnergyAccurateSource(int Imax, int I, int V, int MeterConstant, long power, int linecyc);
      // void calibrateEnergy(int CFnominal, int CFexpected);
      // void calibrateRMSOS(int v1,int v2,int i1,int i2);

//};
#endif

#endif
