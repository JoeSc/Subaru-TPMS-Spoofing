Subaru TPMS Faker
=====================================


Motivation
----------

The 2012 Subaru WRX only allows programming 4 TPMS sensors to the TPMS computer.  This means that when switching from summer to winter or vice-versa you either need to go to the dealer to get the TPMS computer reporgrammed or deal with the tire pressure light being on. Note that Schrader now seels TPMS sensors that can be programmed with the same address as your existing sensors to avoid this problem.

First Step - Decoding Packets after the fact
--------
Record some TPMS sensor traffic to determine the Frequency and modulation of the traffic.  From my tirerack order form I knew the output was at 315MHz.  I then went for a drive and recorded the IQ with SDRSharp to determine if it was AM or FM modulation.  Looking at the IQ it is simple to see that its AM Modulation with OOK

![ScreenShot](/Examples/0/Tutorial_0_IQ_OOK.png)

Next up was to record the AM Modulated data to make it easier to decode with a python script.  That results in something that looks like this

![ScreenShot](/Examples/0/Tutorial_0_AM_Blank.png)

To validate the script I broke the packets into multiple portions and marked up the AM modulated data

![ScreenShot](/Examples/0/Tutorial_0_AM_Marked.png)

1. One bit time was calculated to be 125us
1. The Preamble starts with `0b11110` I call this the "start" pattern
1. There are then 13 rising edges until the next "start" pattern, the sync section
1. Then another "start" pattern
1. The data section starts on the next falling edge, the start of the manchester encoded data.
1. 74 bits of raw data are sent which translates into 37 bits of manchester data
   1. Tirerack provided me with ID codes for my four TPMS sensors.  I searched through the manchester decoded bits to try and find the tire id codes.  I determined that the address provided by tirerack needs to be inverted to get the addresses sent over RF,  I could be interpreting the manchester coding incorrectly, but I decided due to the pressure section looking correct that my manchester coding must be correct since the pressure section inverted would be just wrong.
```
   * decimal     hex          inverted hex
   * 7135718  =  0x6CE1E6  =  0x931e19
   * 7128045  =  0x6CC3ED  =  0x933C12
   * 7134437  =  0x6CDCE5  =  0x93231A
   * 7127775  =  0x6CC2DF  =  0x933D20
```
The packet breaks down as follows
* **PREPEND** 3 bits long. Not sure what this data is or what it does.  TODO:  Decrease tire PSI and see if any of these trigger at low pressure
* **ADDRESS** 24 bits long.
* **PRESSURE** 10 bits long. Formatted in tenth's of PSI

The python script I wrote looks at the wav file recorded by a program like SDRsharp and is able to decode the packets and print them to the prompt. It expects a WAV file of the AF AM decoded signal.  It should look like the above picture
```bash
$ python scripts/wav_to_packet_decoded.py Examples/0/Tutorial_0_AM.wav 
Packet Number 0 @      709  = 0xa4cf48157  pre = 2  addr = 933d20 (6cc2df) pressure = 34.3 psi  man_bits =37
$ 
```

## Second Step - Have GNU Radio parse the IQ wav data
The end goal here is to get a live decoder setup.  And then bench test a transmitter to ensure that gnuradio can parse the fake transmitter.  I had recorded IQ data from SDRsharp and wanted to start with parsing that. I also made some recordings with rtl_sdr to work with that data.  The below flow graph can parse either the raw rtl_sdr file(Green Section), The IQ data from SDRSharp, or the rtl_sdr file which has been converted with [rtlsdr-to-gqrx](https://gist.github.com/DrPaulBrewer/917f990cc0a51f7febb5).  The flow of the graph is as follows
*  Source the Complex IQ data
    * If rtl_sdr raw file then convert to complex by performing operations taken from [Osmocom Wiki](http://osmocom.org/projects/sdr/wiki/rtl-sdr#rtl_sdr)
	* If SDRSharp IQ data then convert to complex directly
    * If converted rtl_sdr data then its already in complex format
*  Throttle the sample rate to the source frequency of 2.048M(This is bypassed since its not necessary for this decoding
*  A Custom sample_per_second block prints out how many samples were processed in the last second(Useful for debugging)
*  Use the AM Demodulation block to convert the complex data into a floating point output representing the AM demodulated data
*  Use the Binary Slicer to turn the AM float data into 0's an 1's
*  Pass the data to the Subaru TPMS Decoding Block
    * This is a custom written block that essentially does what `wav_to_packet_decoded.py` does but in C

![ScreenShot](/Examples/1/rtl_sdr_recording_to_packets_marked.grc.png)

The output looks like this
```
Status	                     Sample#  Hex Data    Prepend         Address                    Pressure
BAD      Packet Number 0 @   161687
GOOD     Packet Number 1 @   172885  0xa4c786565  pre = 2  addr = 931e19 (6ce1e6) pressure = 35.7 psi
BAD      Packet Number 2 @   182138
BAD      Packet Number 3 @   192375
GOOD     Packet Number 4 @   203607  0xa4c786565  pre = 2  addr = 931e19 (6ce1e6) pressure = 35.7 psi
BAD      Packet Number 5 @   218994
BAD      Packet Number 6 @   253781
GOOD     Packet Number 7 @   647834  0xa4cf4816c  pre = 2  addr = 933d20 (6cc2df) pressure = 36.4 psi
GOOD     Packet Number 8 @   673445  0xa4cf4816c  pre = 2  addr = 933d20 (6cc2df) pressure = 36.4 psi
GOOD     Packet Number 9 @   683692  0xa4cf4816c  pre = 2  addr = 933d20 (6cc2df) pressure = 36.4 psi
```
* Debugging can be enabled to view the Complex IQ data, the AM demodulated output and then the binary sliced output. This is helpful in debugging different signal levels

### Compiling and running gnuradio
Below are the commands used to compile and get gnuradio up and running with the above flowgraph
```bash
$ cd gnuradio/custom_blocks/gr-custom/
$ mkdir build
$ cd build/
$ cmake -DCMAKE_INSTALL_PREFIX=../../gnuradio-custom ../
$ make
$ make install
$ cd ../..
# The below bash script is just a wrapper around launching gnuradio-companion  
# It allows sourcing the blocks and running them without installing anything
$ bash launch_gnuradio_custom_block.sh
```


## Third Step - Use RTL-SDR dongle as a source for gnuradio

## Fourth Step - Send TPMS data using panstamp
The panstamp is an ATMEGA328 married to a Texas Instruments cc1101. Its a nice small module and should be capable of sending and receiving the tpms data.  The cc1101 supports manchester encoded data, however I couldn't manage to get it to work



# FLOW
* Capture using `rtl_sdr`
* Convert formats using `./rtlsdr-to-gqrx`

