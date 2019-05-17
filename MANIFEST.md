title: gr-hpsdr
brief: modules for OpenHPSDR Hermes / Metis and Red Pitaya
tags:
  - Hermes
  - Metis
  - Red Pitaya
  - OpenHpsdr

author: 
  - Tom McDermott, N5EG

copyright_owner: 
  - Tom McDermott, N5EG
  
repo: https://github.com/Tom-McDermott/gr-hpsdr
website: https://github.com/Tom-McDermott/gr-hpsdr
--- 
gnuradio 3.7 modules for OpenHPSDR Hermes / Metis and Red Pitaya using the OpenHpsdr protocol. July 2017

hermesNB sources decimated downconverted 48K-to-384K receiver complex stream(s), and sinks one 48k sample rate transmit complex stream.

hermesWB sources raw ADC samples as a vector of floats, with vlen=16384. Each individual vector contains time contiguous samples. However there are large time gaps between between vectors. This is how HPSDR produces raw samples, it is due to Ethernet interface rate limitations between HPSDR and the host computer.

The modules are compatible with version 3.7.x of gnuradio and Hermes firmware version 1.8 through 3.2 (known as OpenHPSDR protocol 1). It is not compatible with the new OpenHPSDR protocol 2.

Updated to increase the maximum number of receivers to 7. Hermes only supports 4 receivers due to limited FPGA capacity. Red Pitaya with the OpenHPSDR protocol supports 6 receivers. Note that beyond 4 receivers and 384k sample rate exceeds 100 Mb/s Ethernet interface capacity.

If no Alex module is present (just Hermes/Metis) then the Alex control fields will have no effect, and the Verbose mode will produce nonsense for Fwd and Rev power measurements, but valid Hermes FPGA revision string.

It is sometimes necessary to delete all files inside the build subdirectory before re-running cmake.
