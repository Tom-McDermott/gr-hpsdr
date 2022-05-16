/* -*- c++ -*- */
/*
 * Copyright 2013-2022 Thomas C. McDermott, N5EG.
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

// -----------------------------------------------------------------
// 				Revisions
//
// Mar 1, 2015 - Additions for ALEX friendly registers.
//
// July 2017 -	Modified to allow up to 8 receivers. No longer emits
//		256 sample buffers to gnuradio, rather each stream
//		contains a smaller number of samples dependent on
//		the number of receivers.
//
// April 2020 - Update to gnuradio 3.8
// -----------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "hermesNB_impl.h"

#include "HermesProxy.h"
#include <stdio.h>	// for DEBUG PRINTF's

HermesProxy* Hermes;	// make it visible to metis.cc


namespace gr {
  namespace hpsdr {


    using input_type = gr_complex;
    using output_type = gr_complex;
    hermesNB::sptr
    hermesNB::make(int RxFreq0, int RxFreq1, int RxFreq2, int RxFreq3,
			 int RxFreq4, int RxFreq5, int RxFreq6, int RxFreq7,
			 int TxFreq, int RxPre,
			 int PTTModeSel, int PTTTxMute, int PTTRxMute,
			 unsigned char TxDr, int RxSmp, const char* Intfc, 
			 const char * ClkS, int AlexRA, int AlexTA,
			 int AlexHPF, int AlexLPF, int Verbose, int NumRx,
			 const char* MACAddr)
    {
      return gnuradio::make_block_sptr<hermesNB_impl>(
        RxFreq0, RxFreq1, RxFreq2, RxFreq3, RxFreq4, RxFreq5,
	RxFreq6, RxFreq7, TxFreq, RxPre, PTTModeSel, PTTTxMute, PTTRxMute,
 	TxDr, RxSmp, Intfc, ClkS, AlexRA, AlexTA, AlexHPF, AlexLPF, Verbose,
	NumRx, MACAddr);
    }


    /*
     * The private constructor
     */
    hermesNB_impl::hermesNB_impl(int RxFreq0, int RxFreq1, int RxFreq2, int RxFreq3,
			 int RxFreq4, int RxFreq5, int RxFreq6, int RxFreq7, 
			 int TxFreq, int RxPre,
			 int PTTModeSel, int PTTTxMute, int PTTRxMute,
			 unsigned char TxDr, int RxSmp, const char* Intfc, 
			 const char * ClkS, int AlexRA, int AlexTA,
			 int AlexHPF, int AlexLPF, int Verbose, int NumRx,
			 const char* MACAddr)
      : gr::block("hermesNB",
              gr::io_signature::make(1, 1, sizeof(input_type)),		// inputs to hermesNB block
              gr::io_signature::make(1, MAXRECEIVERS, sizeof(output_type)) )	// outputs from hermesNB block
    {
	Hermes = new HermesProxy(RxFreq0, RxFreq1, RxFreq2, RxFreq3, RxFreq4,
		 RxFreq5, RxFreq6, RxFreq7, TxFreq, RxPre, PTTModeSel, PTTTxMute,
		 PTTRxMute, TxDr, RxSmp, Intfc, ClkS, AlexRA, AlexTA,
		 AlexHPF, AlexLPF, Verbose, NumRx, MACAddr);	// Create proxy, do Hermes ethernet discovery
	//Hermes->RxSampleRate = RxSmp;
	//Hermes->RxPreamp = RxPre;

	gr::block::set_output_multiple(256);		// process outputs in groups of at least 256 samples
	//gr::block::set_relative_rate((double) NumRx);	// FIXME - need to also account for Rx sample rate

    }

    /*
     * Our virtual destructor.
     *  NOTE: In V3.9 of gnuradio, destructor never gets called,
     *        but all resources are cleaned up anyway.
     *        Move to ::stop() so that statistics are printed.
     */
    hermesNB_impl::~hermesNB_impl()
    {
	//delete Hermes;
    }



bool hermesNB::stop()		// override base class
    {
	Hermes->Stop();			// stop ethernet activity on Hermes
        delete Hermes;			// Stop is guaranteed to be called
					// by gnuradio.
	return gr::block::stop();	// call base class stop()
    }

bool hermesNB::start()		// override base class
    {
	Hermes->Start();		// start rx stream on Hermes
	return gr::block::start();	// call base class start()
    }

void hermesNB::set_Receive0Frequency (float Rx0F) // callback to allow slider to set frequency
    {
	Hermes->Receive0Frequency = (unsigned)Rx0F;	// slider must be of type real, convert to unsigned
    }

void hermesNB::set_Receive1Frequency (float Rx1F) // callback to allow slider to set frequency
    {
	Hermes->Receive1Frequency = (unsigned)Rx1F;	// slider must be of type real, convert to unsigned
    }

void hermesNB::set_Receive2Frequency (float Rx2F) // callback to allow slider to set frequency
    {
	Hermes->Receive2Frequency = (unsigned)Rx2F;	// slider must be of type real, convert to unsigned
    }

void hermesNB::set_Receive3Frequency (float Rx3F) // callback to allow slider to set frequency
    {
	Hermes->Receive3Frequency = (unsigned)Rx3F;	// slider must be of type real, convert to unsigned
    }

void hermesNB::set_Receive4Frequency (float Rx4F) // callback to allow slider to set frequency
    {
	Hermes->Receive4Frequency = (unsigned)Rx4F;	// slider must be of type real, convert to unsigned
    }

void hermesNB::set_Receive5Frequency (float Rx5F) // callback to allow slider to set frequency
    {
	Hermes->Receive5Frequency = (unsigned)Rx5F;	// slider must be of type real, convert to unsigned
    }

void hermesNB::set_Receive6Frequency (float Rx6F) // callback to allow slider to set frequency
    {
	Hermes->Receive6Frequency = (unsigned)Rx6F;	// slider must be of type real, convert to unsigned
    }

void hermesNB::set_Receive7Frequency (float Rx7F) // callback to allow slider to set frequency
    {
	Hermes->Receive7Frequency = (unsigned)Rx7F;	// slider must be of type real, convert to unsigned
    }

void hermesNB::set_TransmitFrequency (float TxF) // callback to allow slider to set frequency
    {
	Hermes->TransmitFrequency = (unsigned)TxF;	// slider must be of type real, convert to unsigned
    }

void hermesNB::set_RxSampRate(int RxSmp)	// callback to set RxSampleRate
    {
	Hermes->RxSampleRate = RxSmp;
    }

void hermesNB::set_RxPreamp(int RxPre)	// callback to set RxPreamp on or off
    {
	Hermes->RxPreamp = (bool)RxPre;
    }

void hermesNB::set_PTTMode(int PTTmode)	// callback to set PTTMode (Off, Vox, On)
    {
	Hermes->PTTMode = PTTmode;
    }

void hermesNB::set_PTTOffMutesTx(int PTTTx)	// callback to set PTTOffMmutesTx (Off, On)
    {
	Hermes->PTTOffMutesTx = PTTTx;
    }

void hermesNB::set_PTTOnMutesRx(int PTTRx)	// callback to set PTTOnMutesRx (Off, On)
    {
	Hermes->PTTOnMutesRx = PTTRx;
    }
 
void hermesNB::set_TxDrive(int TxD)	// callback to set Transmit Drive Level (0..255)
    {
	Hermes->TxDrive = (unsigned char)TxD;
    }

void hermesNB::set_ClockSource(const char * ClkS)	// callback to set Clock source
    {
	unsigned int ck;
	sscanf(ClkS, "%x", &ck);   	// convert char string to 8 bits
	ck &= 0xFC;			// mask lower bits
	Hermes->ClockSource = ck;
    }

void hermesNB::set_AlexRxAntenna(int RxA)		// callback to set Alex Rx Antenna Selector
{
	Hermes->AlexRxAnt = RxA;
}

void hermesNB::set_AlexTxAntenna(int TxA)		// callback to set Alex Tx Antenna Selector
{
	Hermes->AlexTxAnt = TxA;
}

void hermesNB::set_AlexRxHPF(int HPF)		// callback to select Alex Rx High Pass Filter
{
	Hermes->AlexRxHPF = HPF;
}

void hermesNB::set_AlexTxLPF(int LPF)		// callback to set Alex Tx Low Pass filter
{
	Hermes->AlexTxLPF = LPF;
}

void hermesNB::set_Verbose(int Verb)		// callback to turn Verbose mode on or off
{
	Hermes->Verbose = Verb;
}

void hermesNB_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
        /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }


int hermesNB_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {



// 3.10 uses the following declarations of in, out.
// Keep this in the code in case we have to make modifications here while debugging.
// The 3.10 syntax uses newer C++ feature 'auto'.
//
//	auto in = static_cast<const input_type*>(input_items[0]);
//	auto out = static_cast<output_type*>(output_items[0]);



      const input_type *in = reinterpret_cast<const input_type*>(input_items[0]);  // Tx Samples
      output_type *out = reinterpret_cast<output_type*>(output_items[0]); // Rx Samples


// Send I and Q samples received on input port to HermesProxy, it may or may not
// consume them. Hermes needs 63 complex samples in each HPSDR-USB frame.

       if ((ninput_items[0] >= 63))
       {
         int consumed = Hermes->PutTxIQ(in, 63);
         consume_each(consumed); // Tell runtime system how many input items we consumed on
  				 // each input stream.
       };

//
// Get partially-filled 256-float buffers. The packing level is different dependent on the
// number of receivers.  The buffers are sequentially packed, all receivers IQ first
// sample then all receiver IQ second sample, etc.  The global variable USBRowCount[] tells
// us how many time samples per receiver there are in one USB frame.

	IQBuf_t Rx;
	int NumRx = Hermes->NumReceivers;

        if( (Rx = Hermes->GetRxIQ()) == NULL)	//no more available from the radio
            return(0);				// tell gnuradio we did not produce any samples

	int SamplesPerRx = Hermes->USBRowCount[NumRx-1];

	// Send buffered complex samples to our block's output port(s)

	for (int index=0; index<SamplesPerRx; index++)
	    for (int receiver=0; receiver < NumRx; receiver++)
	        ((gr_complex *)output_items[receiver])[index] = gr_complex(*Rx++, *Rx++);


	return(SamplesPerRx);

    }	// general_work

  } /* namespace hpsdr */
} /* namespace gr */

