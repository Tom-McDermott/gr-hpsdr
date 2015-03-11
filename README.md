gr-hpsdr
========

gnuradio 3.7 module for HPSDR Hermes / Metis.

Compatible with version 3.7 of gnuradio and versions of Hermes firmware 1.8 through at least 3.1. It will not be compatible with the new HPSDR protocol under development that could be released for Hermes perhaps sometime in 2015.

The vesion on the master branch has minimal Alexaries controls. The version on the alex branch has more extensive alex control, and should be used if you have the Alex filter/switch modules connected to the HPSDR radio.

To build:
---------

    mkdir build 
    cd build 
    cmake ../ 
    make 
    sudo make install 
    sudo ldconfig 


