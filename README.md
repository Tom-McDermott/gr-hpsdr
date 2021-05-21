gr-hpsdr
========

gnuradio modules for OpenHPSDR Hermes / Metis and Red Pitaya using the OpenHpsdr protocol.   May 2021

* hermesNB  sources decimated downconverted 48K-to-384K receiver complex stream(s), and sinks one 48k sample rate transmit complex stream.
* hermesWB  sources raw ADC samples as a vector of floats, with vlen=16384. Each individual vector contains time contiguous samples. However there are large time gaps between between vectors. This is how HPSDR produces raw samples, it is due to Ethernet interface rate limitations between HPSDR and the host computer.

There are several branches, depending on which version of gnuradio you are using:
* gr_3.7 - branch for gnuradio 3.7
* gr_3.8 - branch for gnuradio 3.8
* gr_3.9 - branch for gnuradio 3.9
* master - currently tracks gr_3.7 to support legacy PYBOMBS 3.7

The modules are compatible with gnuradio and Hermes firmware version 1.8 through 3.2 (known as OpenHPSDR
protocol 1). It is not compatible with the new OpenHPSDR protocol 2.

It is sometimes necessary to delete all files inside the build subdirectory before re-running cmake.


To Start:
---------

	You may need to edit ~/.profile adding these two lines:
		export PYTHONPATH=/usr/local/lib/python3/dist-packages:/usr/local/lib/python3.6/dist-packages:$PYTHONPATH
		export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
	or whatever python packages path is appropriate for your installation.
	
	For gr_3.8 you may need to install:
		$ sudo apt install liborc-0.4-dev
		$ sudo apt install swig
		
	For gr_3.9 you may need pybind11-dev installed.
		$ sudo apt install pybind11-dev
		$ sudo ldconfig
		
	You may get an error message:  x-term missing.   It is not needed, however
	most Linux systems will have one available at:
		/usr/bin/gnome-terminal
	If you want, edit the configuration file to add.
		Note that 3.7, 3.8, and 3.9 have different config files.
		


To build:
---------

	git checkout the_branch_you_want  (i.e. gr_3.7 or gr_3.8 or gr_3.9)
    mkdir build 
    cd build 
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr
    make 
    sudo make install 
    sudo ldconfig 

Note: If gnuradio was installed from a binary (for example using apt-get) it may expect Out-Of-Tree modules to be installed at /usr    If gnuradio was installed and built from source
it may expect the build configuration files at locations prefixed with  /usr/local 

The cmake command may need to be modified to change the desired installation path:

    cmake .. -DCMAKE_INSTALL_PREFIX=/the-prefix-to-utilize


Release Tags:
-------------

Gnuradio 3.7

* v1.0 - An older version provided for backwards compatibility (for gnuradio 3.7.2 to 3.7.9, and Ubuntu 14.04).
* v1.1 - Supports 1 or 2 receivers, gnuradio 3.7.10 and later, and Ubuntu 16.04. The buffer handling in this version is more efficient than v1.2 and later, but that may not be important in your application.
* v1.2 - Supports 1 to 7 receivers. Your actual hardware likely supports fewer than 7 receivers. Hermes supports 4, Red Pitaya 6. More than 4 receivers / 384k requires the use of gigabit Ethernet.

Gnuradio 3.8

* v2.0 - Supports 1 to 7 receivers. Your actual hardware likely supports fewer than 7 receivers. Hermes supports 4, Red Pitaya 6. More than 4 receivers / 384k requires the use of gigabit Ethernet.

* v2.1 - modify one Linux-only system call to be compatible with BSD to enable additional OS support.


Gnuradio 3.9

* v3.0 - Supports 1 to 7 receivers. Your actual hardware likely supports fewer than 7 receivers. Hermes supports 4, Red Pitaya 6. More than 4 receivers / 384k requires the use of gigabit Ethernet.


