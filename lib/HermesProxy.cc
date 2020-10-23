/* -*- c++ -*- */
/* 
 * Copyright 2013-2017 Tom McDermott, N5EG
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

//
// Hermes Proxy
//
// Encapsulates the Hermes module for access/control by GNU Radio.
// Used by the HermesNB (Narrow Band) and HermesWB (Wide Band) modules
// that provide Hermes sink and source to GNU Radio.
//
// Data+Control --> 2 x USB-like 512-byte frames.
// 2 x USB-like frames --> UDP IP-packet.
// UDP IP packet --> Ethernet frame --> Send to Hermes.
// Reverse happens for data from Hermes. 
// See the HPSDR documentation for USB and Ethernet frame formats.
//
// Uses the Metis Ethernet interface module to send/receive Ethernet
// frames to/from Hermes.
//
// HermesNB uses this proxy to convert raw data and control flags
// and send/receive them to Hermes.
//
// Version:  December 15, 2012
// Updates:  * Make Clock Source and AlexControl programmable from GUI
//           * July 10, 2013 - update for GRC 3.7
//	     * December 4, 2013 - additional parameters in constructor	
//           * March 13, 2014 - flip transmit I and Q symbols, due to FPGA
//           reversing them. Set TxDrive default to 0 (rather than 255).
//	     * July 2017 - increase number of receivers to 8.  Hermes
//	     supports 4, Red Pitaya supports 6.  The protocol spec lists 8,
//	     but do not know of any hardware that yet supports 8. The
//	     constructor is getting unwieldy, but XML contrains what can
//	     be passed from GRC to the constructor to simple types.
//

#include <gnuradio/io_signature.h>
#include "HermesProxy.h"
#include "metis.h"
#include <stdio.h>
#include <cstring>

#include <algorithm>
#include <list>

// 		Build the scheduler vectors for larger numbers of receivers
//
// These involve non-integer ratios, so the queue events are spread relatively
//   evenly while fitting in the exact number of events.

std::vector<int> * schedulevector[20];

//  Three receivers - 25 Tx queue events per set of 63, 126, 252, 504  received frames
std::vector<int> L3_48 = {  0, 3, 5, 8, 10, 13, 15, 18, 20, 23, 25, 28, 30,
 33, 35, 38, 40, 43, 45, 48, 50, 53, 55, 58, 60 }; // 25 frames per 63
std::vector<int> L3_96 = {  1, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 59,
 64, 69, 74, 79, 84, 89, 94, 98, 103, 108, 113, 118, 122 }; // 25 frames per 126
std::vector<int> L3_192 = {  2, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110,
 118, 128, 138, 148, 158, 168, 178, 188, 196, 206, 216, 226, 236, 244 };  // 25 frames per 252
std::vector<int> L3_384 = {  4, 20, 40, 60, 80, 100, 120, 140, 160, 180, 200, 220,
 236, 256, 276, 296, 316, 336, 356, 376, 392, 412, 432, 452, 472, 488 };  // 25 frames per 504

//  Four receivers - 19 Tx queue events per set of 63, 126, 252, 504  received frames
std::vector<int> L4_48 = { 3, 6, 10, 13, 16, 20, 23, 26, 30, 33, 36, 40, 43,
 46, 50, 53, 56, 59, 62  }; // 19 frames per 63
std::vector<int> L4_96 = {  6, 12, 20, 26, 32, 40, 46, 52, 60, 66, 72, 80, 86,
 92, 100, 106, 112, 118, 124 }; // 19 frames per 126
std::vector<int> L4_192 = { 12, 24, 40, 52, 64, 80, 92, 104, 120, 132, 144,
 160, 172, 184, 200, 212, 224, 236, 248 };  // 19 frames per 252
std::vector<int> L4_384 = { 24, 48, 80, 104, 128, 160, 184, 208, 240, 264, 288,
 320, 344, 368, 400, 424, 448, 472, 496 };  // 19 frames per 504

//  Five receivers - 15 Tx queue events per set of 63, 126, 252, 504  received frames
std::vector<int> L5_48 = { 4, 8, 12, 16, 21, 25, 29, 33, 37, 42, 46, 50, 54, 58, 62  }; // 15 frames per 63
std::vector<int> L5_96 = { 8, 16, 24, 32, 42, 50, 58, 66, 74, 84, 92, 100, 108, 116, 124  }; // 15 frames per 126
std::vector<int> L5_192 = { 16, 32, 48, 64, 84, 100, 116, 132, 148, 168, 184, 200, 216, 232, 248  };  // 15 frames per 252
std::vector<int> L5_384 = { 32, 64, 96, 128, 168, 200, 232, 264, 296, 336, 368, 400, 432, 464, 496  };  // 15 frames per 504

//  Six receivers - 13 Tx queue events per set of 63, 126, 252, 504  received frames
std::vector<int> L6_48 = { 5, 10, 15, 20, 24, 29, 34, 39, 44, 48, 53, 58, 62  }; // 13 frames per 63
std::vector<int> L6_96 = { 10, 20, 30, 40, 48, 58, 68, 78, 88, 96, 106, 116, 124  }; // 13 frames per 126
std::vector<int> L6_192 = { 20, 40, 60, 80, 96, 116, 136, 156, 176, 192, 212, 232, 248  };  // 13 frames per 252
std::vector<int> L6_384 = { 40, 80, 120, 160, 192, 232, 272, 312, 352, 384, 424, 464, 496  };  // 13 frames per 504

//  Seven receivers - 11 Tx queue events per set of 63, 126, 252, 504  received frames
std::vector<int> L7_48 = { 6, 12, 17, 23, 29, 34, 40, 45, 51, 57, 62  }; // 11 frames per 63
std::vector<int> L7_96 = { 12, 24, 34, 46, 58, 68, 80, 90, 102, 114, 124  }; // 11 frames per 126
std::vector<int> L7_192 = { 24, 48, 68, 92, 116, 136, 160, 180, 204, 228, 248  };  // 11 frames per 252
std::vector<int> L7_384 = { 48, 96, 136, 184, 232, 272, 320, 360, 408, 456, 496  };  // 11 frames per 504




HermesProxy::HermesProxy(int RxFreq0, int RxFreq1, int RxFreq2, int RxFreq3,
			 int RxFreq4, int RxFreq5, int RxFreq6, int RxFreq7,
			 int TxFreq, int RxPre,
			 int PTTModeSel, int PTTTxMute, int PTTRxMute,
			 unsigned char TxDr, int RxSmp, const char* Intfc, 
			 const char * ClkS, int AlexRA, int AlexTA,
			 int AlexHPF, int AlexLPF, int Verb, int NumRx,
			 const char* MACAddr)	// constructor
{

	schedulevector[0] = &L3_48;		// Build the array of schedule vector pointers
	schedulevector[1] = &L3_96;
	schedulevector[2] = &L3_192;
	schedulevector[3] = &L3_384;
	schedulevector[4] = &L4_48;
	schedulevector[5] = &L4_96;
	schedulevector[6] = &L4_192;
	schedulevector[7] = &L4_384;
	schedulevector[8] = &L5_48;
	schedulevector[9] = &L5_96;
	schedulevector[10] = &L5_192;
	schedulevector[11] = &L5_384;
	schedulevector[12] = &L6_48;
	schedulevector[13] = &L6_96;
	schedulevector[14] = &L6_192;
	schedulevector[15] = &L6_384;
	schedulevector[16] = &L7_48;
	schedulevector[17] = &L7_96;
	schedulevector[18] = &L7_192;
	schedulevector[19] = &L7_384;


	//pthread_mutex_init (&mutexRPG, NULL);
	//pthread_mutex_init (&mutexGPT, NULL);
	//
	// Notes in case needed...
	//
	//  pthread_mutex_lock(&mutex) - acquire a lock on the specified mutex variable. If the
	// mutex is already locked by another thread, this call will block the calling thread
	// until the mutex is unlocked.

	//  pthread_mutex_unlock(&mutex) - unlock a mutex variable. An error is returned if mutex
	// is already unlocked or owned by another thread.

	//  pthread_mutex_trylock(&mutex) - attempt to lock a mutex or will return error code if
	// busy. Useful for preventing deadlock conditions. RETURN VALUES
	// On success, pthread_mutex_trylock() returns 0. On error, one of the following
	// values is returned:
	//	EBUSY    The mutex is already locked.
	//	EINVAL   mutex is not an initialized mutex.
	//	EFAULT   mutex is an invalid pointer.


	RxSampleRate = RxSmp;
	strcpy(interface, Intfc);	// Ethernet interface to use (defaults to eth0)
	NumReceivers = NumRx;

	unsigned int cs;		// Convert ClockSource strings to unsigned, then intitalize
	sscanf(ClkS, "%x", &cs);
	ClockSource = (cs & 0xFC);

//	Initialize the Alex control registers.

	AlexRxAnt = AlexRA;		// Select Alex Receive Antenna or from T/R relay
	AlexTxAnt = AlexTA;		// Select Alex Tx Antenna
	AlexRxHPF = AlexHPF;		// Select Alex Receive High Pass Filter
	AlexTxLPF = AlexLPF;		// Select Alex Transmit Low Pass Filter

	Verbose = Verb;			// Turn Verbose mode on/off

        for (int i=0; i<18; i++)
	  mactarget[i] = toupper(MACAddr[i]);	// Copy the requested MAC target address

	Receive0Frequency = (unsigned)RxFreq0;
	Receive1Frequency = (unsigned)RxFreq1; 
	Receive2Frequency = (unsigned)RxFreq2; 	
	Receive3Frequency = (unsigned)RxFreq3;  
	Receive4Frequency = (unsigned)RxFreq4; 
	Receive5Frequency = (unsigned)RxFreq5; 
	Receive6Frequency = (unsigned)RxFreq6; 
	Receive7Frequency = (unsigned)RxFreq7; 

	TransmitFrequency = (unsigned)TxFreq;		// initialize frequencies
	TxDrive = TxDr;		// default to (almost) off
	PTTMode = PTTModeSel;
	RxPreamp = (bool)RxPre;
	PTTOffMutesTx = (bool)PTTTxMute;   // PTT Off mutes the transmitter
	PTTOnMutesRx = (bool)PTTRxMute;	// PTT On mutes receiver

	ADCdither = false;
	ADCrandom = false;
	RxAtten = 0;		// Hermes V2.0
	Duplex = true;		// Allows TxF to program separately from RxF

	TxStop = false;

	RxWriteCounter = 0;	//
	RxReadCounter = 0;	// These control the Rx buffers to Gnuradio
	RxWriteFill = 0;	//

	TxWriteCounter = 0;	//
 	TxReadCounter = 0;	// These control the Tx buffers to Hermes
	TxControlCycler = 0;	//
	TxFrameIdleCount = 0;	//

	LostRxBufCount = 0;	//
	TotalRxBufCount = 0;	//
	LostTxBufCount = 0;	//
	TotalTxBufCount = 0;	// diagnostics
	CorruptRxCount = 0;	//
	LostEthernetRx = 0;	//
	CurrentEthSeqNum = 0;	//

	
	TxHoldOff = 0;		// initialize transmit hold off counter


	try
	{
	    // allocate the receiver buffers
	    for(int i=0; i<NUMRXIQBUFS; i++)
		RxIQBuf[i] = new float[RXBUFSIZE];

	    // allocate the transmit buffers
	    for(int i=0; i<NUMTXBUFS; i++)
		TxBuf[i] = new unsigned char[TXBUFSIZE];
	}
	catch(std::bad_alloc& ba)
	{
	   fprintf(stderr, "\nFATAL: unable to allocate memory for buffers.\n %s\n", ba.what());
	   throw;
	}


	metis_discover((const char *)(interface));


	USBRowCount[0] = 63;  // Number of Rows of samples per Rx Input 
	USBRowCount[1] = 36;  // USB frame based on number of receivers 1..8
	USBRowCount[2] = 25;
	USBRowCount[3] = 19;
	USBRowCount[4] = 15;
	USBRowCount[5] = 13;
	USBRowCount[6] = 11;
	USBRowCount[7] = 10;  // Eight receivers


//
// If there is no specified MAC address (i.e. wildcard, or anything less than 17 
// characters, then just grab the first Hermes/Metis that
// responds to discovery. If there is a specific MAC address specified, then wait
// until it appears in the Metis cards table, and set the metis table index to match.
// The string is HH:HH:HH:HH:HH:HH\0 formated, where HH is a 2-digital Hexidecimal number
// uppercase, example:    04:7F:3D:0F:28:5A
//

	metis_entry = 0;
	if (strlen(mactarget) != 17)			// Not a fully-qualified MAC address, default to first MAC found
	{
	  while (metis_found() == 0)
		;					// wait until Hermes responds with first discovered MAC
	}
	else						// Search the table for the entry matching requested MAC address
	{
	  bool found = false;
	  while(!found)					// Search for MAC address in the metis_table until the cows come home
	    for(int i=0; i<metis_found(); i++)
	      {
		if (strcmp(mactarget, metis_mac_address(i)) == 0)	// Exact match found
		{
		  metis_entry = i;					// Select entry in metis_table
	          found = true;
		  break;
		}
	      }
	}

	metis_receive_stream_control(RxStream_Off, metis_entry);	// turn off Hermes -> PC streams

	UpdateHermes();					// send specific control registers
							// and initialize 1st Tx buffer
							// before allowing scheduler to Start
};

HermesProxy::~HermesProxy()
{
	fprintf(stderr, "\nLostRxBufCount = %lu  TotalRxBufCount = %lu"
		"  LostTxBufCount = %lu  TotalTxBufCount = %lu"
		"  CorruptRxCount = %lu  LostEthernetRx = %lu\n",
	        LostRxBufCount, TotalRxBufCount, LostTxBufCount,
		TotalTxBufCount, CorruptRxCount, LostEthernetRx);

	metis_receive_stream_control(RxStream_Off, metis_entry);	// stop Hermes data stream
	
	metis_stop_receive_thread();	// stop receive_thread & close socket

	for(int i=0; i<NUMTXBUFS; i++)
		delete [] TxBuf[i];

	for(int i=0; i<NUMRXIQBUFS; i++)
		delete [] RxIQBuf[i];
}


void HermesProxy::Stop()	// stop ethernet I/O
{
	metis_receive_stream_control(RxStream_Off, metis_entry);	// stop Hermes Rx data stream
	TxStop = true;					// stop Tx data to Hermes
};

void HermesProxy::Start()	// start rx stream
{
	TxStop = false;					// allow Tx data to Hermes
	metis_receive_stream_control(RxStream_NB_On, metis_entry);	// start Hermes Rx data stream
	TxHoldOff = true;				// Hold off buffers before bursting Tx
};

void HermesProxy::PrintRawBuf(RawBuf_t inbuf)	// for debugging
{

	fprintf(stderr, "Addr: %p    Dump of Raw Buffer\n", inbuf);
	for(int row=0; row<4; row++)
	{
	    int addr = row * 16;
	    fprintf(stderr, "%04X:  ", addr);
	    for(int column=0; column<8; column++)
	    	fprintf(stderr, "%02X:", inbuf[row*16+column]);
	    fprintf(stderr, "...");
	    for(int column=8; column<16; column++)
	    	fprintf(stderr, "%02X:", inbuf[row*16+column]);
	    fprintf(stderr, "\n");
	}

	fprintf(stderr, "\n");

};

// ********** Routines to receive data from Hermes/Metis and give to Gnuradio ****************

void HermesProxy::ReceiveRxIQ(unsigned char * inbuf)	// called by metis Rx thread.
{

	// look for lost receive packets based on skips in the HPSDR ethernet header
	// sequence number.


//PrintRawBuf(inbuf);	// include Ethernet header

	unsigned int SequenceNum = (unsigned char)(inbuf[4]) << 24;
	SequenceNum += (unsigned char)(inbuf[5]) << 16;
	SequenceNum += (unsigned char)(inbuf[6]) << 8;
	SequenceNum += (unsigned char)(inbuf[7]);

	if(SequenceNum > CurrentEthSeqNum + 1)
	{
	    LostEthernetRx += (SequenceNum - CurrentEthSeqNum);
	    CurrentEthSeqNum = SequenceNum;
	}
	else
	{
	  if(SequenceNum == CurrentEthSeqNum + 1)
	    CurrentEthSeqNum++;
	}
	

	// Metis Rx thread gives us collection of samples including the Ethernet header
	// plus 2 x HPSDR USB frames.

	// TODO - Handle Mic audio from Hermes.


	// For 1 Rx, the frame comes in with I2 I1 I0 Q2 Q1 Q0 M1 M0 repeating
	// starting at location 8 through 511. At total of (512-8)/8 = 63 complex pairs.
	// I2 I1 I0 is 24-bit 2's complement format.
	// There are two of the USB HPSDR frames in the received ethernet buffer.
	// A buffer of 126 complex pairs is about
	//	0.3 milliseconds at 384,000 sample rate
	//	0.6 milliseconds at 192,000 sample rate
	//	2.4 milliseconds at 48,000 sample rate
	//
	//
	// We always allocate one output buffer to unpack every received ethernet frame.
	// Each input Ethernet frame contains a different number of I + Q samples as 2's
	// complement depending on the number of receivers.
	//
 	//    RxWriteCounter - the current Rx buffer we are writing to
	//    RxWriteFill    - #floats we have written to the current Rx buffer (0..255)
	//    RxReadCounter  - the Rx buffer that gnuradio can read
	//


	inbuf += 8;			// skip past Ethernet header

	IQBuf_t outbuf;			// RxWrite output buffer selector
	
	TotalRxBufCount++;

	ScheduleTxFrame(TotalRxBufCount); // Schedule a Tx ethernet frame to Hermes if ready.

	// Need to check for both 1st and 2nd USB frames for the status registers.
	// Some status come in only in the first, and some only in the second.

	// check for proper frame sync

	for (int USBFrameOffset = 0; USBFrameOffset<=512; USBFrameOffset += 512)
	{

		unsigned char s0 = inbuf[0+USBFrameOffset];	// sync register 0
		unsigned char s1 = inbuf[1+USBFrameOffset];	// sync register 0
		unsigned char s2 = inbuf[2+USBFrameOffset];	// sync register 0
		unsigned char c0 = inbuf[3+USBFrameOffset];	// control register 0
		unsigned char c1 = inbuf[4+USBFrameOffset];	// control register 1
		unsigned char c2 = inbuf[5+USBFrameOffset];	// control register 2
		unsigned char c3 = inbuf[6+USBFrameOffset];	// control register 3
		unsigned char c4 = inbuf[7+USBFrameOffset];	// control register 4

		if(s0 == 0x7f && s1 == 0x7f && s2 == 0x7f)
		{
			if((c0 & 0xf8) == 0x00) // Overflow and Version
			{
//			  fprintf(stderr, "Reg:0x00   c0:0x%x c1:0x%x c2:0x%x c3:0x%u c4:0x%x\n", c0, c1, c2, c3, c4);

			  if(c1 & 0x01)
			    ADCoverload = true;
			  else
			    ADCoverload = false;

			  HermesVersion = c4;
			}

			if((c0 & 0xf8) == 0x08)  //AIN5 and AIN1
			{
//			  fprintf(stderr, "Reg:0x08   c0:0x%x c1:0x%x c2:0x%x c3:0x%u c4:0x%x\n", c0, c1, c2, c3, c4);
			  AIN5 = (unsigned int)c1 * 256 + (unsigned int)c2;
			  AIN1 = (unsigned int)c3 * 256 + (unsigned int)c4;
			}

			if((c0 & 0xf8) == 0x10)  //AIN2 and AIN3
			{
//			  fprintf(stderr, "Reg:0x10   c0:0x%x c1:0x%x c2:0x%x c3:0x%u c4:0x%x\n", c0, c1, c2, c3, c4);
			  AIN2 = (unsigned int)c1 * 256 + (unsigned int)c2;
			  AIN3 = (unsigned int)c3 * 256 + (unsigned int)c4;

			}

			if((c0 & 0xf8) == 0x18)  //AIN4 and AIN6
			{
//			  fprintf(stderr, "Reg:0x18   c0:0x%x c1:0x%x c2:0x%x c3:0x%u c4:0x%x\n", c0, c1, c2, c3, c4);
			  AIN4 = (unsigned int)c1 * 256 + (unsigned int)c2;
			  AIN6 = (unsigned int)c3 * 256 + (unsigned int)c4;
			}

			if (Verbose)
			{
			  SlowCount++;
			  if ((SlowCount & 0x1ff) == 0x1ff)
			  {
				float FwdPwr = (float)AIN1 * (float)AIN1 / 145000.0;
				float RevPwr = (float)AIN2 * (float)AIN2 / 145000.0;

				// calculate SWR
				double SWR =  0.0;
				try
				{
					SWR = (1+sqrt(RevPwr/FwdPwr))/(1-sqrt(RevPwr/FwdPwr));
					if(false == std::isnormal(SWR))
					{
						throw 0;
					}
				}
				catch(int& e)
				{
					// there was an anomaly in the SWR calculation, make it obvious ...
					SWR =  99.9;
				}

				fprintf(stderr, "AlexFwdPwr = %4.1f  AlexRevPwr = %4.1f   ", FwdPwr, RevPwr);
				// report SWR if forward power is non-zero
				if(static_cast<int>(FwdPwr) != 0)
				{
					fprintf(stderr, "SWR = %.2f:1   ", SWR);
				}
				fprintf(stderr, "ADCOver: %u  HermesVersion: %d (dec)  %X (hex)\n", ADCoverload, HermesVersion, HermesVersion);
				//fprintf(stderr, "AIN1:%u  AIN2:%u  AIN3:%u  AIN4:%u  AIN5:%u  AIN6:%u\n", AIN1, AIN2, AIN3, AIN4, AIN5, AIN6);  
				}
			}
		} //endif sync is valid
		
		else
		{
			CorruptRxCount++;
//			fprintf(stderr, "HermesProxy: EP6 received from Hermes failed sync header check.\n");
//			int delta = inbuf - inbufptr;
//			fprintf(stderr, "USBFrameOffset: %i  inbufptr: %p  delta: %i \n", USBFrameOffset, inbufptr, delta);
//			PrintRawBuf(inbufptr);	// include Ethernet header
			return;   // error return
		}

	}	// end for two USB frames


//
// In each USB frame, the bytes per sample row, number of sample rows comes from the
// table. The first sample starts at offset 8 bytes.   Each USB frame is 512 bytes
//
// ------------------ Input USB 2's complement buffer ------------------
// # of Rx	# I+Q samples/receiver	#Bytes/row	#Pad Bytes/frame
// -------	---------------------	----------	----------------
//    1			63		    8			0
//    2			36		   14			0
//    3			25		   20			4
//    4			19		   26		       10
//    5			15		   32		       24
//    6			13		   38		       10
//    7			11		   44		       20
//    8			10		   50		        4
//
// It's not straight forward to fully pack an output buffer. Instead, build the
// output buffer with fixed channel alignment. For example, for 3 receivers,
// outputbuf[0,1] is receiver 0 IQ,  outputbuf[2,3] is receiver 1 IQ, outputbuf[4,5] is
// receiver 2 IQ. Then outputbuf[6,7] is receiver 0 IQ next sample. A table similar to
// above for output buffer is needed. One outbuf holds 256 floats and is never fully packed.
//
// ------------ Output 256 float buffer sizing --------------
// # of Rx	# of samples per inBuf		output floats   output complexes
// -------	-----------------------		-------------   ----------------
//    1		 63 * 2(I+Q) * 1(NumRx)	 	     126		63
//    2		 36 * 2(I+Q) * 2(NumRx)		     144		72
//    3		 25 * 2(I+Q) * 3(NumRx)		     150		75
//    4		 19 * 2(I+Q) * 4(NumRx)		     152		76
//    5		 15 * 2(I+Q) * 5(NumRx)		     150		75
//    6		 13 * 2(I+Q) * 6(NumRx)		     156		78
//    7		 11 * 2(I+Q) * 7(NumRx)		     154		77	        	
//    8		 10 * 2(I+Q) * 8(NumRx)		     160		80
//
// The work() routine needs to break up each outputbuf to the gnuradio
// out[] buffer streams (one per Rx).
//
// Pseudocode:
//
// get next (empty) output buffer
// set output indexer to zero
// set input indexer (inbuf, a byte pointer) is already 8 (skip the header)
// for number-of-rows:
//   for number-of-receivers:
//      do twice (for I and for Q):
//        read 2's-complement at [input indexer]
//        input index += 3  (bytes)
//        convert to float
//        write float[output indexer] to output array
//        output indexer += 1 (float)
//      next receiver
//    input indexer += 2 (bytes)  //skip M1,M0 bytes at end of row
//    next row
//
//
//

	unsigned int outindex;
	unsigned char* inbufindex;

	inbuf += 8;		// Skip past USB sync header

	// do two USB frames
	for (unsigned int USBFrameOffset = 0; USBFrameOffset<=512; USBFrameOffset += 512)
	{
	    inbufindex = inbuf + USBFrameOffset;  // inbuf already pointing past Ethernet frame header

	    if ((outbuf = GetNextRxBuf()) == NULL)
	        return;				// all buffers full. Throw away data

	    outindex = 0;

	    // one USB frame
	    for (int row=0; row < USBRowCount[NumReceivers - 1]; row++)
	    {
	        for (int receiver=0; receiver < NumReceivers; receiver++)
	        {
		     outbuf[outindex++] = Unpack2C(inbufindex);	// I
		     inbufindex += 3;
		     outbuf[outindex++] = Unpack2C(inbufindex);	// Q
		     inbufindex += 3;
	        };
	        inbufindex +=2;			// skip microphone samples in the row
	    };
	};

	return;			// normal return;
};

// Unpack an unsigned 2's complement sample into a floating point number
// maximum value of +1.0 and minimum of -1.0
float HermesProxy::Unpack2C(const unsigned char* inptr)
{
	// 24 bit 2's complement --> float (-1.0 ... +1.0)

	if ((PTTOnMutesRx) & (PTTMode == PTTOn))
	    return 0.0;					// if receiver is muted

	int F;

	F = (int)(((signed char)*(inptr))<<16);		// 2C to Float
	F += ((int)((unsigned char)*(inptr+1))<<8);
	F += (int)((unsigned char)*(inptr+2));
	if (F<0)
	     F = -(~F + 1);

	return (float)F/8388607.0;
};

//
// July 2017 change...
// Previously, this checked the fill level of the current buffer.
// New code always needs a new output buffer when called - simplify logic to just check
// to see if one is available and return it, else return NULL if none available (and
// increment lost Rx buffer counter).
//


// New version
IQBuf_t HermesProxy::GetNextRxBuf() // get next Writeable Rx buffer
{
  if (((RxWriteCounter+1) & (NUMRXIQBUFS - 1)) == RxReadCounter)
  {
    LostRxBufCount++;	// No Rx Buffers available. Throw away the data
    return NULL;
  }
  else
  {
    ++RxWriteCounter &= (NUMRXIQBUFS - 1); // get next writeable buffer
    return RxIQBuf[RxWriteCounter];
  }
};


IQBuf_t HermesProxy::GetRxIQ()	// next Readable Rx buffer, called by HermesNB to pickup any RxIQ
{

	//int status = pthread_mutex_trylock(&mutexRPG);	// Don't block gnuradio scheduler
	//  if(status != 0)
	//    return NULL;		// return 'no buffers' if can't acquire the mutex

	if(RxReadCounter == RxWriteCounter)
	{
	  //pthread_mutex_unlock(&mutexRPG);
	  return NULL;				// empty - no buffers to return

	}

	IQBuf_t ReturnBuffer = RxIQBuf[RxReadCounter];	// get the next receiver buffer
	++RxReadCounter &= (NUMRXIQBUFS - 1);		// increment read counter modulo

	//pthread_mutex_unlock(&mutexRPG);

	return ReturnBuffer;			// next readable Rx buffer
};


// ************  Routines to send data from gnuradio to the transmitter ***************


// The Hermes hardware does not have any method to indicate when it wants a frame,
// nor any back pressure mechanism. We derive the Tx timing by counting the Rx frames
// Hermes is sending to us. This depends on the Rx Sample rate and the number of
// receivers because the Tx sample rate is fixed at 48000.
//
// Some of the ratios involve prime numbers, so a fixed rate of Rx frames for each Tx
// frame is not practical.  Instead, a table holds bits that indicate when to queue a
// Tx frame. The RxBufCount is a monotonically increasing long int, each Rx frame
// increments it. The transmitter always transmits 63 I+Q samples per frame.
// The bits in the array are spaced as uniformly as possible given prime numbers.
//
//					      USB		 USB 
// RxSampleRate	#Receivers	#TxSamp*Rate/buffer	#RxSamp/buffer		Ratio Rx to Tx
// -----------	----------	-------------------	--------------		--------------
//    48000		1		63			63		1
//    48000		2		63			36		1.75
//    48000		3		63			25		2.52
//    48000		4		63			19		3.315789476...
//    48000		5		63			15		4.2
//    48000		6		63			13		4.846153846...
//    48000		7		63			11		5.727272727...
//    48000		8		63			10		6.3
//    96000		1		126			63		2
//    96000		2		126			36		3.5
//    96000		3		126			25		5.04
//    96000		4		126			19		6.63157894...
//    96000		5		126			15		8.4
//    96000		6		126			13		9.69230769...
//    96000		7		126			11		11.45454545...
//    96000		8		126			10		12.6
//    192000		1		252			63		4
//    192000		2		252			36		7
//    192000		3		252			25		10.08
//    192000		4		252			19
//    192000		5		252			15		16.8
//    192000		6		252			13
//    192000		7		252			11
//    192000		8		252			10		25.2
//    384000		1		504			63		8
//    384000		2		504			36		14
//    384000		3		504			25		20.16
//    384000		4		504			19
//    384000		5		504			15		33.2
//    384000		6		504			13
//    384000		7		504			11
//    384000		8		504			10		50.4
//
//
//
// Method
// ------
// 
// An Ethernet frame holds 2 HPSDR-USB frames, so the ratios above don't change when
// counting ethernet frames (Tx and Rx both double, ratio stays the same).  We count
// Ethernet frames to determine when to schedule a Tx frame.
//
// For 1 or 2 receivers the ratios involve small common denominators, so we just
// count Ethernet frames by looking at the frame sequence number. For 3 or more
// receivers the ratios are non integer, so we use arrays to tell us when to
// schedule a Tx frame. For example, for 3 receivers and a rx rate of 48ksps, we
// need to almost uniformly in time schedule 25 tx frames for every 63 that are
// received.
//
// Future: If no data to transmit, periodically send a frame so that basic control
// registers get updated.   [Hooks left commented for future use].
//


void HermesProxy::ScheduleTxFrame(unsigned long RxBufCount) // Transmit one ethernet frame to Hermes if ready.
{
	// RxBufCount is a sequential 32-bit unsigned int received etherent frame sequence number


	switch (NumReceivers)
	{
	case 1 :
		if(RxSampleRate == 48000)	// one Tx frame for each Rx frame
		{
			SendTxIQ();
			return;
		}
	
		if(RxSampleRate == 96000)	// one Tx frame for each two Rx frames
		  if((RxBufCount & 0x1) == 0)
		  {
			SendTxIQ();
			return;
		  }
	
		if(RxSampleRate == 192000)	// one Tx frame for each four Tx frames
		  if((RxBufCount & 0x3) == 0)
		  {
			SendTxIQ();
			return;
		  }

		if(RxSampleRate == 384000)	// one Tx frame for each eight Tx frames
		  if((RxBufCount & 0x7) == 0)
		  {
			SendTxIQ();
			return;
		  }
		break;		

	case 2 :
		if(RxSampleRate == 48000)			// one Tx frame for each 1.75 Rx frame
		  if(((RxBufCount % 0x7) & 0x01) == 0)    	// (four Tx frames for each 7 Rx frames)
	    	  {
	        	SendTxIQ();				// 0, 2, 4, 6   (not 1, 3, 5)
			return;
	    	  }

	  	if(RxSampleRate == 96000)			// one Tx frame for each 3.5 Rx frames
	    	  if(((RxBufCount % 0x7) & 0x03) == 0) 	// (two Tx frames for each 7 Rx frames)
	    	  {
			SendTxIQ();				// 0, 4    (not 1, 2, 3, 5, 6)
			return;
	    	  }
	
	  	if(RxSampleRate == 192000)			// one Tx frame for each seven Tx frames
	    	  if((RxBufCount % 0x7) == 0)
	    	  {
			SendTxIQ();
			return;
	    	  }
	
	  	if(RxSampleRate == 384000)			// one Tx frame for each fourteen Tx frames
	    	  if((RxBufCount % 14) == 0)
	    	{
			SendTxIQ();
			return;
	    	}
		break;


	default :						// 3 or more receivers

//	// Compute a selector to decide which scheduler vector to use

		int FrameIndex;
		int RxNumIndex = NumReceivers-3;	  //   3,  4,  5,  6,  7  -->  0, 1, 2, 3, 4

		int SpeedIndex = (RxSampleRate) / 48000;  // 48k, 96k, 192k, 384k -->  1, 2, 4, 8
		SpeedIndex = SpeedIndex >> 1;		  // 48k, 96k, 192k, 384k -->  0, 1, 2, 4
		if (SpeedIndex == 4)  SpeedIndex = 3;	  // 48k, 96k, 192k, 384k -->  0, 1, 2, 3

		int selector = RxNumIndex * 4 + SpeedIndex;	//  0 .. 19

	// Compute the frame number within a vector

		if(RxSampleRate == 48000)
		    FrameIndex = RxBufCount % 63;	// FrameIndex is 0..62
	  	if(RxSampleRate == 96000)
		    FrameIndex = RxBufCount % 126;	// FrameIndex is 0..125
	  	if(RxSampleRate == 192000)
		    FrameIndex = RxBufCount % 252;	// FrameIndex is 0..251
	  	if(RxSampleRate == 384000)
		    FrameIndex = RxBufCount % 504;	// FrameIndex is 0..503


		std::vector<int> * p;
		p = schedulevector[selector];		// pick the schedule vector matching NumRx,RxSpeed

		std::vector<int>::iterator itr;
		itr = find (p->begin(), p->end(), FrameIndex);
		if (itr != p->end())
		{
		    SendTxIQ();		// if the Vector contains FrameIndex then schedule Tx Ethernet packet

//fprintf(stderr, "Scheduled 3..7  NumReceivers: %i   RxSampleRate: %i    selector: %i   SampleIndex: %i\n",
// NumReceivers, RxSampleRate, selector, SampleIndex);

		}
		break;

	}; // switch

	return;
};

void HermesProxy::UpdateHermes()	// send a set of control registers to hardware with naught Tx data
{

	// Repurposed to send the initial registers to Hermes before starting the stream.
	// Ought to rename this as InitializeHermes or something similar.

	// DEBUG
	//fprintf(stderr, "UpdateHermes called\n");

	unsigned char buffer[512];	// dummy up a USB HPSDR buffer;
	for(int i=0; i<512; i++)
		buffer[i] = 0;

	int length = 512;		// metis_write ignores this value
	unsigned char ep = 0x02;	// all Hermes data is sent to end point 2

	// metis_write needs to be called twice to make one ethernet write to the hardware
	// Set these registers before starting the receive stream

	BuildControlRegs(0, buffer);
	metis_write(ep, buffer, length);
	BuildControlRegs(2, buffer);
	metis_write(ep, buffer, length);

	BuildControlRegs(0, buffer);
	metis_write(ep, buffer, length);
	BuildControlRegs(4, buffer);
	metis_write(ep, buffer, length);

	BuildControlRegs(0, buffer);
	metis_write(ep, buffer, length);
	BuildControlRegs(6, buffer);
	metis_write(ep, buffer, length);

	// Initialize the first TxBuffer (currently empty) with a valid control frame (on startup only)
	
	BuildControlRegs(0, buffer);
	RawBuf_t initial = TxBuf[0];
	for(int i=0; i<512; i++)
		initial[i] = buffer[i];

	return;
}


void HermesProxy::BuildControlRegs(unsigned RegNum, RawBuf_t outbuf)
{
	// create the sync + control register values to send to Hermes
	// base on RegNum and the various parameter values.
	// RegNum must be even.

	unsigned char Speed = 0;	// Rx sample rate
	unsigned char RxCtrl = 0;	// Rx controls
	unsigned char Ctrl4 = 0;	// Rx register C4 control

	outbuf[0] = outbuf[1] = outbuf[2] = 0x7f;	// HPSDR USB sync

	outbuf[3] = RegNum;		// C0 Control Register (Bank Sel + PTT)
	if (PTTMode == PTTOn)
	  outbuf[3] |= 0x01;				// set MOX bit

	switch(RegNum)
	{
	  case 0:
	    Speed = ClockSource;	// Set clock Source from user input
	    if(RxSampleRate == 384000)
		Speed |= 0x03;
	    if(RxSampleRate == 192000)
		Speed |= 0x02;
	    if(RxSampleRate == 96000)
		Speed |= 0x01;
	    if(RxSampleRate == 48000)
		Speed |= 0x00;

	    RxCtrl = 0x00;
	    if(RxPreamp)
		RxCtrl |= 0x04;
	    if(ADCdither)
		RxCtrl |= 0x08;
	    if(ADCrandom)
		RxCtrl |= 0x10;


	    Ctrl4 |= ((NumReceivers-1) << 3) & 0x38;	// Number of receivers
							// V1.58 of protocol_1 spec allows
							// setting up to 8 receivers, but
							// I can't find which register to set the
							// Rx Frequency of the 8th receiver.
							// Hermes sends corrupted USB frames if
							// NumReceivers > 4.   Theoretically
							// Red Pitaya is OK to 6 (to be tested by
							// someone else).


	    if(Duplex)
		Ctrl4 |= 0x04;

	    outbuf[4] = Speed;				// C1
	    outbuf[5] = 0x00;				// C2
	    outbuf[6] = RxCtrl | AlexRxAnt;		// C3
	    outbuf[7] = Ctrl4 | AlexTxAnt;		// C4 - #Rx, Duplex

		// branch: TestMercClock - turn on Mercury Common Frequency bit
		// if this works (I cannot test) then add some GUI code
		outbuf[7] = outbuf[7] | 0x80;

          break;

	  case 2:					// Tx NCO freq (and Rx1 NCO for special case)
	    outbuf[4] = ((unsigned char)(TransmitFrequency >> 24)) & 0xff;	// c1 RxFreq MSB
	    outbuf[5] = ((unsigned char)(TransmitFrequency >> 16)) & 0xff;	// c2
	    outbuf[6] = ((unsigned char)(TransmitFrequency >> 8)) & 0xff;	// c3
	    outbuf[7] = ((unsigned char)(TransmitFrequency)) & 0xff;		// c4 RxFreq LSB
          break;

	  case 4:					// Rx1 NCO freq (out port 0)
	    outbuf[4] = ((unsigned char)(Receive0Frequency >> 24)) & 0xff;	// c1 RxFreq MSB
	    outbuf[5] = ((unsigned char)(Receive0Frequency >> 16)) & 0xff;	// c2
	    outbuf[6] = ((unsigned char)(Receive0Frequency >> 8)) & 0xff;	// c3
	    outbuf[7] = ((unsigned char)(Receive0Frequency)) & 0xff;	// c4 RxFreq LSB
	  break;

	  case 6:					// Rx2 NCO freq (out port 1)
	    outbuf[4] = ((unsigned char)(Receive1Frequency >> 24)) & 0xff; // c1 RxFreq MSB
	    outbuf[5] = ((unsigned char)(Receive1Frequency >> 16)) & 0xff; // c2
	    outbuf[6] = ((unsigned char)(Receive1Frequency >> 8)) & 0xff;	 // c3
	    outbuf[7] = ((unsigned char)(Receive1Frequency)) & 0xff;	 // c4 RxFreq LSB
	  break;

	  case 8:					// Rx3 NCO freq (out port 2)
	    outbuf[4] = ((unsigned char)(Receive2Frequency >> 24)) & 0xff; // c1 RxFreq MSB
	    outbuf[5] = ((unsigned char)(Receive2Frequency >> 16)) & 0xff; // c2
	    outbuf[6] = ((unsigned char)(Receive2Frequency >> 8)) & 0xff;	 // c3
	    outbuf[7] = ((unsigned char)(Receive2Frequency)) & 0xff;	 // c4 RxFreq LSB
	  break;

	  case 10:					// Rx4 NCO freq (out port 3)
	    outbuf[4] = ((unsigned char)(Receive3Frequency >> 24)) & 0xff; // c1 RxFreq MSB
	    outbuf[5] = ((unsigned char)(Receive3Frequency >> 16)) & 0xff; // c2
	    outbuf[6] = ((unsigned char)(Receive3Frequency >> 8)) & 0xff;	 // c3
	    outbuf[7] = ((unsigned char)(Receive3Frequency)) & 0xff;	 // c4 RxFreq LSB
	  break;

	  case 12:					// Rx5 NCO freq (out port 4)
	    outbuf[4] = ((unsigned char)(Receive4Frequency >> 24)) & 0xff; // c1 RxFreq MSB
	    outbuf[5] = ((unsigned char)(Receive4Frequency >> 16)) & 0xff; // c2
	    outbuf[6] = ((unsigned char)(Receive4Frequency >> 8)) & 0xff;	 // c3
	    outbuf[7] = ((unsigned char)(Receive4Frequency)) & 0xff;	 // c4 RxFreq LSB
	  break;

	  case 14:					// Rx6 NCO freq (out port 5)
	    outbuf[4] = ((unsigned char)(Receive5Frequency >> 24)) & 0xff; // c1 RxFreq MSB
	    outbuf[5] = ((unsigned char)(Receive5Frequency >> 16)) & 0xff; // c2
	    outbuf[6] = ((unsigned char)(Receive5Frequency >> 8)) & 0xff;	 // c3
	    outbuf[7] = ((unsigned char)(Receive5Frequency)) & 0xff;	 // c4 RxFreq LSB
	  break;

	  case 16:					// Rx7 NCO freq (out port 6)
	    outbuf[4] = ((unsigned char)(Receive6Frequency >> 24)) & 0xff; // c1 RxFreq MSB
	    outbuf[5] = ((unsigned char)(Receive6Frequency >> 16)) & 0xff; // c2
	    outbuf[6] = ((unsigned char)(Receive6Frequency >> 8)) & 0xff;	 // c3
	    outbuf[7] = ((unsigned char)(Receive6Frequency)) & 0xff;	 // c4 RxFreq LSB
	  break;


	//	Note:  While Ver 1.58 of the HPSDR USB protocol doucment specifies up to 8 receivers,
	//	It only defines 7 receive frequency control register addresses. So we are currently
	//	limited to 7 receivers implemented.


	  case 18:					// drive level & filt select (if Alex)
	    if (PTTOffMutesTx & (PTTMode == PTTOff))
		outbuf[4] = 0;				// (almost) kill Tx when PTTOff and PTTControlsTx
	    else
		outbuf[4] = TxDrive;			// c1


	    unsigned char RxHPF, TxLPF;

	    RxHPF = AlexRxHPF;
	    if (AlexRxHPF == 0)				// if Rx autotrack
	    {
		if (Receive0Frequency < 1500000)
		  RxHPF = 0x20;				// bypass
		else if (Receive0Frequency < 6500000)
	          RxHPF = 0x10;				// 1.5 MHz HPF
		else if (Receive0Frequency < 9500000)
		  RxHPF = 0x08;				// 6.5 MHz HPF
		else if (Receive0Frequency < 13000000)
		  RxHPF = 0x04;				// 9.5 mHz HPF
		else if (Receive0Frequency < 20000000)
		  RxHPF = 0x01;				// 13 Mhz HPF
		else if (Receive0Frequency < 50000000)
		  RxHPF = 0x02;				// 20 MHz HPF
		else RxHPF = 0x40;			// 6M BPF + LNA
	    }

	    TxLPF = AlexTxLPF;
	    if (AlexTxLPF == 0)				// if Tx autotrack
	    {
		if (TransmitFrequency > 30000000)
		  TxLPF = 0x10;				// 6m LPF
		else if (TransmitFrequency > 19000000)
		  TxLPF = 0x20;				// 10/12m LPF
		else if (TransmitFrequency > 14900000)
		  TxLPF = 0x40;				// 15/17m LPF
		else if (TransmitFrequency > 9900000)
		  TxLPF = 0x01;				// 30/20m LPF
		else if (TransmitFrequency > 4900000)
		  TxLPF = 0x02;				// 60/40m LPF
		else if (TransmitFrequency > 3400000)
		  TxLPF = 0x04;				// 80m LPF
		else TxLPF = 0x08;			// 160m LPF
	    }

	    outbuf[5] = 0x40;				// c2 - Alex Manual filter control enabled
	    outbuf[6] = RxHPF & 0x7f;			// c3 - Alex HPF filter selection
	    outbuf[7] = TxLPF & 0x7f;			// c4 - Alex LPF filter selection
	  break;

	  case 20:					// Hermes input attenuator setting
	    outbuf[4] = 0;				//
	    outbuf[5] = 0x17;				// Not implemented yet, should not be called by
	    outbuf[6] = 0;				// TxControlCycler yet.
	    outbuf[7] = RxAtten;			// 0..31 db attenuator setting (same function as preamp)
	  break;
	
	  case 22:
	    outbuf[4] = 0;				// Register not documented, but zeroed by
	    outbuf[5] = 0;				// PowerSDR...
	    outbuf[6] = 0;				//
	    outbuf[7] = 0;				//
	  break;					

	  default:
	    fprintf(stderr, "Invalid Hermes/Metis register selection: %d\n", RegNum);
	    break;
	};

};


// hermesNB calls this routine to give IQ data from the block input connector to the proxy.
// Packs transformed data into one HPSDR USB buffer with control registers.
// HermesNB gives us 63 complex samples from in0, we fill one USB buffer with them.
// Audio output could come from in1 but that forms a flowgraph loop in most useful cases
// which is disallowed by GNU Radio, so that code is commented out.

 // called by HermesNB to give us IQ data to send
int HermesProxy::PutTxIQ(const gr_complex * in0, /*const gr_complex * in1,*/ int nsamples)
{

        RawBuf_t outbuf;
	int A, B, I, Q;

	outbuf = GetNextTxBuf();	// get a Txbuffer

	if (outbuf == NULL)		// Could not get a Tx buffer
	  return 0;		 	// Tell hermeNB we didn't consume any input

	// format a HPSDR USB frame to send to Hermes.

	TxControlCycler += 2;		// advance to next register bank, modulo
	if (TxControlCycler > 0x14)	// 11 register banks (0..10). Note: Bank 10
	  TxControlCycler = 0;		//    (Hermes attenuator) requires firmware V2.0

	BuildControlRegs(TxControlCycler, outbuf);	// First 8 bytes are the control registers.


	// Next 63 * 8 bytes are the IQ data and the Audio data.
	// TODO - the L/R audio data to Hermes is not implemented yet.


	for (int i=0; i<nsamples; i++)			// put 63 IQ samples into frame
        {

	// Note: cannot implement audio output because the flowgraph would form a flow loop
	// for any Hermes received data which is not allowed in GNU Radio.

/*	  A = (int)(in1[i].real() * 32767.0);	// scale to 16 bits
	  B = (int)(in1[i].imag() * 32767.0);	// scale to 16 bits
          I = (unsigned int)A;
	  Q = (unsigned int)B;

 	  // convert float to 2's complement 16-bit

	  outbuf[i*8 + 8] = (unsigned char)((I & 0xff00) >> 8);  // L1 MSB audio channel out
	  outbuf[i*8 + 9] = (unsigned char)(I & 0xff);		 // L0 LSB
	  outbuf[i*8 + 10] = (unsigned char)((Q & 0xff00) >> 8); // R1 MSB audio channel out
	  outbuf[i*8 + 11] = (unsigned char)(Q & 0xff);		 // R0 LSB
*/

	// Zero out the audio Left and Right channel outputs.

	  outbuf[i*8 + 8] = 0;		// L1 MSB audio channel out
	  outbuf[i*8 + 9] = 0;		// L0 LSB
	  outbuf[i*8 + 10] = 0;		// R1 MSB audio channel out
	  outbuf[i*8 + 11] = 0;		// R0 LSB   

 	  // convert float to 2's complement 16-bit

	  A = (int)(in0[i].real() * 32767.0);	// scale to 16 bits
	  B = (int)(in0[i].imag() * 32767.0);	// scale to 16 bits


//        I = (unsigned int)A;
//	  Q = (unsigned int)B;
// 03-13-2014  Note: Hermes FPGA reverses transmit I & Q (thus contrary to documentation V1.43)
// Put them back into the correct places.
          Q = (unsigned int)A;
	  I = (unsigned int)B;



	  if(PTTOffMutesTx & (PTTMode == PTTOff))	// Kill Tx if in Rx and PTTControls the Tx
	  {
	    I = 0;
	    Q = 0;
	  };

	  outbuf[i*8 + 12] = (unsigned char)((I & 0xff00) >> 8); // I1 MSB
	  outbuf[i*8 + 13] = (unsigned char)(I & 0xff);		 // I0 LSB
	  outbuf[i*8 + 14] = (unsigned char)((Q & 0xff00) >> 8); // Q1 MSB
	  outbuf[i*8 + 15] = (unsigned char)(Q & 0xff);		 // Q0 LSB

        };


	if(PTTMode == PTTVox)		// if we are in Vox mode, check frame IQ contents
	{
          bool activity = false;

	  for (int i=0; i<nsamples; i++)	 // if any IQ sample is nonzero (VOX) then key Tx
	    if ((outbuf[i*8 + 12] != 0) ||  
	        (outbuf[i*8 + 13] != 0) ||
	        (outbuf[i*8 + 14] != 0) ||
	        (outbuf[i*8 + 15] != 0) )
	    {
		activity = true;
		break;
	    };

	    if(activity)
		outbuf[3] |= 1;		// enable MOX PTT	    
	};

//	fprintf(stderr, "PutTxIQ: Consumed %d samples, TxControlCycler = %d\n",
//		nsamples, TxControlCycler);


	return nsamples;
};


RawBuf_t HermesProxy::GetNextTxBuf()		// get a TXBuf if available
{

	  //int status = pthread_mutex_trylock(&mutexGPT); // Don't block gnuradio scheduler
	  //if(status != 0)
	 //   return NULL;	// No buffers if can't acquire mutex

	  if (((TxWriteCounter+1) & (NUMTXBUFS - 1)) == TxReadCounter)
	  {
	    //pthread_mutex_unlock(&mutexGPT);
	    return NULL;
	  }
	 
	  ++TxWriteCounter &= (NUMTXBUFS - 1); // get next writeable buffer

	  //pthread_mutex_unlock(&mutexGPT);
	  return TxBuf[TxWriteCounter];
};


// SendTxIQ() is called on a periodic basis to send Tx Ethernet frames to the 
// Hermes/Metis hardware.


void HermesProxy::SendTxIQ()
{

	if(TxStop)				// Kill Tx frames if stopped
		return;

	unsigned char ep = 0x2;			// Tx data goes to end point 2

//	fprintf(stderr, "SendTxIQ: TxReadCounter = %d   TxWriteCounter = %d  TxFrameIdleCount = %d\n",
//		TxReadCounter, TxWriteCounter, TxFrameIdleCount); 

	// Time to send one Tx Eth frame (2 x USB frames).
	// If there are at least two buffers in the queue, send then free them.

	//pthread_mutex_lock(&mutexGPT);

	bool bufempty = (TxReadCounter == TxWriteCounter);
	bool bufone = ((TxReadCounter+1 & (NUMTXBUFS - 1)) == TxWriteCounter);

	int TempWriteCounter = TxWriteCounter;
	if (TxWriteCounter < TxReadCounter)
	  TempWriteCounter += NUMTXBUFS;
	bool bufburst = ((TempWriteCounter - TxReadCounter) >= (TXINITIALBURST * 2));

	//pthread_mutex_unlock(&mutexGPT);

	TotalTxBufCount++;

	if(TxHoldOff)	    	// Hold back initial burst of Tx Eth frames
	{
	  if (!bufburst)	// Not enough frames to send a burst
	    return;
	  			// Have enough frames to send the burst 
	  TxHoldOff = false;	// clear the holdoff flag

	  for (int i=0; i<(TXINITIALBURST * 2); i++)	// 2 USB frames per Ethernet frame
 	  {
	    metis_write(ep, TxBuf[TxReadCounter], 512);	// write one USB frame to metis
	    ++TxReadCounter &= (NUMTXBUFS - 1);		// and free it
	  }

	  return;
	}

 	// We're out of bursting mode and into one-at-a-time mode

	if ( bufempty | bufone )    // zero or one buffer ready	
	{
	  LostTxBufCount++;
	  return;
	}
	else	// two or more buffers ready
	{
	//fprintf(stderr, "SendTxIQ02: TxReadCounter = %d   TxWriteCounter = %d  TxFrameIdleCount = %d\n",
		//TxReadCounter, TxWriteCounter, TxFrameIdleCount); 

	  metis_write(ep, TxBuf[TxReadCounter], 512);	// write one USB frame to metis

	 // pthread_mutex_lock(&mutexGPT);
	  ++TxReadCounter &= (NUMTXBUFS - 1);		// and free it
	  //pthread_mutex_unlock(&mutexGPT);

	  metis_write(ep, TxBuf[TxReadCounter], 512);	// write next USB frame to metis

	  //pthread_mutex_lock(&mutexGPT);
	  ++TxReadCounter &= (NUMTXBUFS - 1);		// and free it
	  //pthread_mutex_unlock(&mutexGPT);

	 // TxFrameIdleCount = 0;				// have just sent a frame
	};

	return;
};


// TODO not yet implemented
void HermesProxy::ReceiveMicLR() {};	// receive an LR audio bufer from Hermes hardware



