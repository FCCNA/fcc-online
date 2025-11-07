# FCC Naples Online
This repository contains [MIDAS](https://midas.triumf.ca/)  frontends for FCC Naples R&D projects

Supported items:
* Tektronix 4 Series MSO waveform readout
* CAEN [FELib](https://github.com/caenspa/caen-felib) digitizer readout (tested on VX2730)
* CAEN HV based on [dsproto_sy4527](https://bitbucket.org/ttriumfdaq/dsproto_sy4527/src) frontend
* CAEN N1081 (basic) control using NuclearInstruments [SDK](https://github.com/NuclearInstruments/N1081B-SDK-Python/tree/c89553356f719bb53258f034d7bf09418b219fa5$0)
* Basic Keithley control using pyvisa
* Lauda chiller control through serial
* Arduino based temperature and rotating stage

## Compilation
Make sure MIDAS is installed following the [Quickstart guide](https://daq00.triumf.ca/MidasWiki/index.php/Quickstart_Linux). 
ROOT is not required.
Python bindings are required for Arduino, Lauda and Keithley frontends.

Make sure submodules are also checkout:

`git submodule update --init --recursive`

