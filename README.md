gr-hpsdr
========

gnuradio 3.7 modules for HPSDR Hermes / Metis.

* hermesNB  sources decimated downconverted 48K-to-384K receiver complex stream(s), and sinks one 48k sample rate transmit complex stream.
* hermesWB  sources raw ADC samples as a vector of floats, with vlen=16384. Each individual vector contains time contiguous samples. However there are large time gaps between between vectors. This is how HPSDR produces raw samples, it is due to Ethernet interface rate limitations between HPSDR and the host computer.

The modules are compatible with version 3.7 of gnuradio and versions of Hermes firmware 1.8 through at least 3.1. It will not be compatible with the new HPSDR protocol under development that could be released for Hermes perhaps sometime in 2015.

Updated to merge the 'alex' branch into 'master'. This adds fields to control Alexaries (Alex) LPF and HPF filters, transmit and receive antenna selection, and 6m LNA. Verbose mode ON prints out the rough (uncalibrated) Alex Forward power and Reverse power measurements in the console area.

If no Alex module is present (just Hermes/Metis) then the Alex control fields will have no effect, and the Verbose mode will produce nonsense for Fwd and Rev power measurements, but valid Hermes FPGA revision string.

It is sometimes necessary to delete all files inside the build subdirectory before re-running cmake.

To build:
---------

    mkdir build 
    cd build 
    cmake ../ 
    make 
    sudo make install 
    sudo ldconfig 


Note:
-----

Sept 3, 2016  --  Upon upgrade to Xenial (Ubuntu 16.04.1) and gnuradio 3.10, some swig-->python type checking changed. If you are using either 16.04,
or gnuradio 3.10, then git checkout the xenial310 branch rather than master, and build that instead.


