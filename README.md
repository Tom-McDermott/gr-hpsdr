gr-hpsdr
========

gnuradio 3.7 module for HPSDR Hermes / Metis.

Compatible with version 3.7 of gnuradio and versions of Hermes firmware 1.8 through at least 3.1. It will not be compatible with the new HPSDR protocol under development that could be released for Hermes perhaps sometime in 2015.

The Alex branch adds fields to control Alexaries (Alex) LPF and HPF filters, transmit and receive antenna selection, and 6m LNA. Verbose mode ON prints out the rough (uncalibrated) Alex Forward power and Reverse power measurements in the console area.



To build:
---------

    mkdir build 
    cd build 
    cmake ../ 
    make 
    sudo make install 
    sudo ldconfig 


