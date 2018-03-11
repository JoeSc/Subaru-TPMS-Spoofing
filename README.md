Subaru TPMS Faker
=====================================


# Motivation

The 2012 Subaru WRX only allows programming 4 TPMS sensors to the TPMS computer.  This means that when switching from summer to winter or vice-versa you either need to go to the dealer to get the TPMS computer reporgrammed or deal with the tire pressure light being on. Note that Schrader now seels TPMS sensors that can be programmed with the same address as your existing sensors to avoid this problem.

# First Step - Decoding Packets after the fact
Record some TPMS sensor traffic to determine the Frequency and modulation of the traffic.  From my tirerack order form I knew the output was at 315MHz.  I then went for a drive and recorded the IQ with SDRSharp to determine if it was AM or FM modulation.  Looking at the IQ it is simple to see that its AM Modulation with OOK

![ScreenShot](/Examples/0/Tutorial_0_IQ_OOK.png)

Next up was to record the AM Modulated data to make it easier to decode with a python script.  That results in something that looks like this

![ScreenShot](/Examples/0/Tutorial_0_AM_Blank.png)

To validate the script I broke the packets into multiple portions and marked up the AM modulated data

![ScreenShot](/Examples/0/Tutorial_0_AM_Marked.png)

The layout of a packet is as follows
* Packet starts with `0b11110` the "start" pattern.
* There are then 13 clocks to allow the receiver to synchronize
   * One bit time works out to be 125&mu;s (8Kbps)
* Another "start" pattern, used to indicate the start of data
* The data is then sent as manchester encoded and is 74 bits long(37 manchester bits)
   * Tire Rack provided me with ID codes for my four TPMS sensors.  I searched through the manchester decoded bits to try and find the tire id codes.

```bash
  Tire Rack
   decimal      hex
   7135718   =  0x6CE1E6
   7128045   =  0x6CC3ED
   7134437   =  0x6CDCE5
   7127775   =  0x6CC2DF
```

The packet breaks down as follows
* **PREPEND** 3 bits long.  I only know of the below codes.
    * 0x7:(no bits cleared) Sent during every "normal" transmission.
    * 0x5:(bit 1 cleared) sent on the first transmission after the car has moved after sitting stationary for > ~1 minute.
	* 0x3:(bit 2 cleared) I captured this when deflating tires. 
* **ADDRESS** 24 bits long.
* **PRESSURE** 10 bits long. Formatted in twentieth's of a PSI.

The python script I wrote looks at the wav file recorded by a program like SDRsharp and is able to decode the packets and print them to the prompt. It expects a WAV file of the Audio Frequency AM decoded signal.  It should look something like the above picture.
```bash
$ python scripts/wav_to_packet_decoded.py Examples/0/Tutorial_0_AM.wav 
GOOD Packet  0 @      690  = 0x15b30b7ea8  pre = 5  addr = 6cc2df  34.0 psi
$ 
```

#### FCC Search
I searched ebay for "2012 Subaru WRX TPMS Sensor" led me to decide that my TPMS sensors had FCC ID MRXSMD3MA4.  The FCC requires testing for each device that receives or radiates RF and those reports are published on their website.  I was able to search for that FCC ID and found this [FCC Filing for MRXSMD3MA4](http://fcc.io/MRXSMD3MA4).  The Test Report in that filing has some information about the waveform.
![ScreenShot](/Examples/0/FCC_Filing_Snip.png)
The wake pulse, timing pulse and manchester data correlate to what we saw above.  Additionally the Filing includes some oscilloscope captures of the data being sent
![ScreenShot](/Examples/0/FCC_Filing_Pulse_Train.png)
That data looks similar to what we see in the captures above.

The "Description 2" document also contains some information about hte operation.  They describe a "wake mode"  which is the first transmission in normal operation,  I assume that refers to the 0x5(bit 1 cleared) transmitted in what I call the PREPEND field.  They also describe a pressure re-meausure mode that is triggered whenever the tire pressure varies by more than 5 counts, I assume this corresponds to 0x3(bit 2  cleared).  It also describes a low battery mode.  I'd guess that this corresponds to bit 0 cleared, but to verify I'd have to purchase a TPMS sensor, remove the potting and feed in an adjustable battery voltage.

# Second Step - Have GNU Radio parse the IQ wav data
The end goal is to get a live decoder setup.  And then bench test a transmitter to ensure that gnuradio can parse the fake transmitter.  I had recorded IQ data from SDRsharp and wanted to start with parsing that. I also made some recordings with rtl_sdr to work with that data.  The below flow graph can parse either the raw rtl_sdr file(Green Section), The IQ data from SDRSharp, or the rtl_sdr file which has been converted with [rtlsdr-to-gqrx](https://gist.github.com/DrPaulBrewer/917f990cc0a51f7febb5).  The flow of the graph is as follows, graph source files are in gnuradio/rtl_sdr_recording_to_packets/
*  Source the Complex IQ data
    * If rtl_sdr raw file then convert to complex by performing operations taken from [Osmocom Wiki](http://osmocom.org/projects/sdr/wiki/rtl-sdr#rtl_sdr)
	* If SDRSharp IQ data then convert to complex directly
    * If converted rtl_sdr data then its already in complex format
*  Throttle the sample rate to the source frequency of 2.048M(This is bypassed since its not necessary for this decoding
*  A Custom [sample_per_second block](/gnuradio/custom_blocks/gr-custom/python/sample_per_second_c.py) prints out how many samples were processed in the last second(Useful for debugging)
*  Use the AM Demodulation block to convert the complex data into a floating point output representing the AM demodulated data
*  Use the Binary Slicer to turn the AM float data into 0's an 1's
*  Pass the data to the Subaru TPMS Decoding Block
    * This is a custom written block that essentially does what `wav_to_packet_decoded.py` does but in C.  See [subaru_tpms_decode_cpp_char_impl.cc](/gnuradio/custom_blocks/gr-custom/lib/subaru_remote_decode_cpp_char_impl.cc)

![ScreenShot](/Examples/1/rtl_sdr_recording_to_packets_marked.grc.png)

The output looks like this.  "GOOD" means that I received the proper amount of bits for the packet.  "BAD" means that I got the start pattern but did not receive the proper amount of bits.
```
Status	                     Sample#  Hex Data      Prepend        Address  Pressure
BAD      Packet Number  0 @    67338  0x1d9
GOOD     Packet Number  1 @    83408  0x1db3879acf  pre = 7  addr = 6ce1e6  36.0 psi
GOOD     Packet Number  2 @   103880  0x1db3879acf  pre = 7  addr = 6ce1e6  36.0 psi
GOOD     Packet Number  3 @   119240  0x1db3879acf  pre = 7  addr = 6ce1e6  36.0 psi
GOOD     Packet Number  4 @   134600  0x1db3879acf  pre = 7  addr = 6ce1e6  36.0 psi
BAD      Packet Number  5 @   144136  0x1db
GOOD     Packet Number  6 @   155093  0x1db3879acf  pre = 7  addr = 6ce1e6  36.0 psi
GOOD     Packet Number  7 @   165339  0x1db3879acf  pre = 7  addr = 6ce1e6  36.0 psi
BAD      Packet Number  8 @   370054  0x76c4
GOOD     Packet Number  9 @   380833  0x1db30b7ece  pre = 7  addr = 6cc2df  35.9 psi
GOOD     Packet Number 10 @   416609  0x1db30b7ece  pre = 7  addr = 6cc2df  35.9 psi
GOOD     Packet Number 11 @   431944  0x1db30b7ece  pre = 7  addr = 6cc2df  35.9 psi
GOOD     Packet Number 12 @   457507  0x1db30b7ece  pre = 7  addr = 6cc2df  35.9 psi
GOOD     Packet Number 13 @   467735  0x1db30b7ece  pre = 7  addr = 6cc2df  35.9 psi
```
* Debugging can be enabled to view the Complex IQ data, the AM demodulated output and then the binary sliced output. This is helpful in debugging different input IQ signal levels.

### Compiling and running gnuradio with custom blocks
Below are the commands used to compile and get gnuradio up and running with the above flowgraph.  The custom blocks perform the demodulation of the TPMS packets.  In theory I could use the clock recovery block of gnuradio, but I never was able to get that working correctly.
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

# Third Step - Use RTL-SDR dongle as a source for gnuradio
With the ability to parse IQ data recorded with rtl_sdr I moved on to having gnuradio decode the packets live from an rtl_sdr dongle.  I ran into some issues with the rtl_sdr source block provided by gr-osmosdr apt package.  It seems that setting automatic gain in the rtl_sdr block is not the same as using automatic gain with the rtl_sdr program.  I had to make some tweaks to the gr-osmosdr source code since I didn't feel like messing around with the gains and knew that rtl_sdr with automatic gain worked so wanted the rtl_sdr block to work the same way.  I wrapped the building of librtlsdr and gr-osmosdr with mychanges into a script.
```bash
$ cd gnuradio/custom_blocks/ 
& bash build_custom_rtl_sdr_source.sh
$ bash launch_gnuradio_custom_block.sh
```
The live gnuradio flow graph is located in gnuradio/rtl_sdr_live_to_packets/  It has the same flow as the above recording flow graph except it sources data from the rtl_sdr dongle.


# Fourth Step - Send TPMS data using cc1101
The [TI cc1101](http://www.ti.com/lit/ds/symlink/cc1101.pdf) Sub-1 GHz RF Transceiver should be capable of transmitting and receiving in the 315MHz band and decoding the tpms packets.  The cc1101's sister product the cc1111 is used in the [rfcat](https://github.com/atlas0fd00m/rfcat) project so the cc1101 should work well.  I have some [panStamp AVR 2](https://github.com/panStamp/panstamp/wiki/panStamp%20AVR%202.-Technical%20details) modules that include a Atmega328p and a TI cc1101.  This will be the development platform for this step.
### Sending a tpms packet
The cc1101 packet format is shown in the picture below.  
![ScreenShot](/Examples/4/cc1101_packet_format.png)
Based on that format I'd need to disable preamble and sync words and just stuff all the tpms packet data into the data field since what I really needed was a sync+preamble+sync+data.  I decided to disable the preamble/sync by setting SYNC_MODE[2:0] to 0x4. The cc1101 also supports manchester encoding, but per [this](https://e2e.ti.com/support/wireless_connectivity/low_power_rf_tools/f/155/t/126101) TI e2e posting, when manchester encoding is enabled the cc1101 encodes/decodes all sent/received information.  since I needed to send `0b1111` as the "start" pattern I would have to do my own manchester encoding.

I used the panstamp codebase as a start since it had the libraries(spi, cc1101) that I needed.  I wanted to be able to build outside arduino since I didn't need any of the special features so I ported the libraries to "raw gcc" and added a simple uart library.  I used smartrf studio to determine new register settings, and added a patable_OOK function to fill in the patable for OOK transmission.

With all of this setup I wanted to determine if the car was able to receive data from the cc1101.  Since I didn't want to switch my tires out and verify that I could spoof a tire I decided to spoof a low pressure value on a tire.  Using the [tpms_sender_raw_gcc](/C_Code/tpms_sender_raw_gcc/main.cpp) program I am able to force the TPMS light to come on in ~20 seconds.  When I swap out tires I will likely revisit this simple program to ensure I can keep the TPMS light off without having the programmed tires on the car.

### Receiving a tpms packet
With the sender app written it was fairly easy to create a receiver app.  All that was required was determining the sync word for the cc1101 to know when a packet started.  I used `0xaaaf` as the sync word.  This value occurs at the end of the preamble right before the data section starts.  With this figured out I could receive packets on the cc1101 without the burden of running gnuradio.  The code can be found at [tpms_receiver_raw_gcc](/C_Code/tpms_receiver_raw_gcc/main.cpp)

### Current status
The winter tires are programmed into my car.
With my four winter tires on and using the `tpms_sender_raw_gcc` application I was able to get the TPMS light to turn on.  Before doing the full swap I tried driving around with 3 winter tires and one summer tire.  I tried spoofing the missing tire sensor and can't get it to work.  I was able to get the TPMS light to come on though.  Things to try:
* Even though the waveforms of my spoofed packets versus actual packets look identical, perhpaps the TPMS receiver box is not receiving my packets
   * I have the cc1101 set to full output power, but maybe since I'm inside the cabin when sending, which is where the TPMS receiver is(rear left quarter panel), I'm saturating the receivers input.
   * Tape the receiving antenna from rtl_sdr to the rear left quarter panel and tweak cc1101 output power so the rtl_sdr sees the same power for cc1101 packets and tpms packets.

[//]: # (# Final Step - Receiver and Send TPMS data using cc1101)
[//]: # (The goal for this step is to create a tpms spoofer so that when I switch to summer wheels the tpms light will stay off.  The program will receive tpms messages, modify the address in the message and send a new message with the new address.  I could take the easy way out and just send out 32.0PSI for all four tires all the time, however I still wanted to keep the TPMS functionality since it may come in handy sometime.  I envision the flow of the program to be.)
[//]: # "* Initial power up into learm mode.  Learnd mode will run until it has received messages from 4x sensors 12x times(the sensors send 5 messages per transmission so this should mean we receve 3 transmissions per sensors)  It will then save those addresses into the EEPROM"
[//]: # (  * Optionally this mode could be bypassed by programming the EEPROM with the known tire addresses.)
[//]: # ( On subsequent power ups with the EEPROM programmed the program will enter the spoof learn mode.  This mode acts similar to the above learn mode, however it will keep a sorted list of the four most frequently seen addresses in the first 2 minutes of running.  After that point it will consider those addresses to be the tires on the car.)
[//]: # (  * The reason I need to keep it sorted is if we happen to receive a transmission from another car during the initial spoof learn period.  Consider the situation where my car has tires A,B,C,D on it and another car has W,X,Y,Z.)
[//]: # (     * After powering up my program sees sensors [A, B, W, X, C, D]  if I mapped the spoof addresses to the learned addresses with a moduls that would mean that when I received data from sensors A and C I would send it to the 0th saved address)
[//]: # (     * Actually this won't work.  Need to rethink this	flow.  What do we sort by?  if we sort by times seen it will be constantly changing order.  I guess we could bin it by 4's.  Sort by groups of 4 and the sort within the group by address. )
[//]: # (### Future)
[//]: # ( Design a PCB that is a simple USB stick that plugs into the cars AUX USB port.  Or maybe have it run off +12V power and hardwire it somewhere else)
[//]: # ( If theres intrest in this look into TI CC1350 part.  This part is dual band bluetooth and low frequecny.  A cell phone app could be created to assist with learning and dumping tire pressure addresses and pressures.)
