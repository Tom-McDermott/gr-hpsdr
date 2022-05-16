gr-hpsdr
========

gnuradio modules for OpenHPSDR Hermes / Metis and Red Pitaya using the OpenHpsdr protocol.   May 2021.

* hermesNB  sources decimated downconverted 48K-to-384K receiver complex stream(s), and sinks one 48k sample rate transmit complex stream.
* hermesWB  sources raw ADC samples as a vector of floats, with vlen=16384. Each individual vector contains time contiguous samples. However there are large time gaps between between vectors. This is how HPSDR produces raw samples, it is due to Ethernet interface rate limitations between HPSDR and the host computer.

There are several branches, depending on which version of gnuradio you are using.
Git checkout the branch you need.  The instrucitons for configuraing and buld ARE DIFFERRENT
for the different vsersions.

* gr_3.7 - branch for gnuradio 3.7
* gr_3.8 - branch for gnuradio 3.8
* gr_3.9 - branch for gnuradio 3.9
* gr_3.10 - branch for gnuradio 3.10

* master - currently tracks gr_3.7 to support legacy PYBOMBS 3.7

The modules are compatible with gnuradio and Hermes firmware version 1.8 through 3.2 (known as OpenHPSDR
protocol 1). It is not compatible with the new OpenHPSDR protocol 2.

It is sometimes necessary to delete all files inside the build subdirectory before re-running cmake.

The gr_3.8 branch has been verified but minimally tested on Ubuntu 20.04


To Start:
---------

	Edit ~/.profile adding these two lines (for gr_3.8):
		export PYTHONPATH=/usr/local/lib/python3/dist-packages:/usr/local/lib/python3.6/dist-packages:$PYTHONPATH
		export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
	
	For gr_3.8 you may need to install:
		$ sudo apt install liborc-0.4-dev
		$ sudo apt install swig
		
	You may get an error message:  x-term missing.   It is not needed, however
	most Linux systems will have one available at:
		/usr/bin/gnome-terminal
	If you want, edit the configuration file to add.
		Note that 3.7, 3.8, and 3.9 have different config files.
		
Preparation:
------------
For version 3.9 and beyond you need to install pybind11.  Gnuradio 3.9.1.0 was built against pybind11 version 2.5.0.   The supported version of pybind11 has changed (and even retrograded).
Follow the instructions on the gnuradio wiki about identifying and installing the correct version (2.5.0).
 
 


To build:
---------
     
    git checkout the_branch_you_want  (i.e. gr_3.7, gr_3.8, or gr_3.9 or gr_3.10)
    mkdir build 
    cd build 

    NOTE: For 3.8 only: if you have already previously built gr_3.8, then you need to
      sudo make uninstall it before updating 3.8. This is because of a problem
      with gnuradio 3.8.2 not always updating symlinks on revised builds.

    cmake .. -DCMAKE_INSTALL_PREFIX=/usr
    make 
    sudo make install 
    sudo ldconfig 

Note: If CMAKE_INSTALL_PREFIX is not defined on the cmake line above, the build configuration will write files to locations prefixed with  /usr/local  This is appropriate for gnuradio that has been installed and built from source. If gnuradio was installed from a binary (for example using apt install) it will expect Out-Of-Tree modules to be installed in /usr.

Release Tags:
-------------

Gnuradio 3.7

* v1.0 - An older version provided for backwards compatibility (for gnuradio 3.7.2 to 3.7.9, and Ubuntu 14.04).
* v1.1 - Supports 1 or 2 receivers, gnuradio 3.7.10 and later, and Ubuntu 16.04. The buffer handling in this version is more efficient than v1.2 and later, but that may not be important in your application.
* v1.2 - Supports 1 to 7 receivers. Your actual hardware likely supports fewer than 7 receivers. Hermes supports 4, Red Pitaya 6. More than 4 receivers / 384k requires the use of gigabit Ethernet.

Gnuradio 3.8

* v2.0 - Supports 1 to 7 receivers. Your actual hardware likely supports fewer than 7 receivers. Hermes supports 4, Red Pitaya 6. More than 4 receivers / 384k requires the use of gigabit Ethernet.

Gnuradio 3.9

* v3.0 - Supports 1 to 7 receivers. Your actual hardware likely supports fewer than 7 receivers. Hermes supports 4, Red Pitaya 6. More than 4 receivers / 384k requires the use of gigabit Ethernet.
