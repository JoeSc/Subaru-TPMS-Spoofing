import wave
import math
import sys
import time
import collections
import logging
import progressbar

logging.basicConfig()
statelog = logging.getLogger("STATE")
statelog.setLevel(logging.WARNING)

log = logging.getLogger("")
log.setLevel(logging.WARNING)


states = open("old_states.txt", "w")

def dec2signed(val,size):
    if val > ((1 << (size - 1 )) - 1):
        val -= (1 << size)
    return val

def packet2hex(packet):
    bit_string = ''
    for bit in packet:
        bit_string += str(bit)

    packet = int(bit_string,2)
    prepend = packet >> 34
    addr = (packet >> 10) & 0x7fffff
    not_addr = int(format(addr,"023b").replace("0","Z").replace("1","0").replace("Z","1"), 2)

    pressure = packet & 0x3ff
    return "0x%x  pre = %x  addr = %x (%x) pressure = %d"%(packet, prepend, addr, not_addr, pressure)
    return "0x%x"%int(bit_string,2)



f = wave.open(sys.argv[1])
samplesize = f.getsampwidth() * 8
channels = f.getnchannels()

packet_number = 0
bin_data = []
cur_data = []
sync_clk_pulse = []
state = "waiting"
clk_period = 0
last_clk_pulse = 0
last_signal_level = 0

slp = collections.deque(3*[0], 3)
val = 0
transition = 0
transition_pt = 0

#bar = progressbar.ProgressBar(redirect_stdout=True, max_value = f.getnframes())
#for i in range(f.getnframes()):
    #if i % 1000 == 0:
        #bar.update(i)
for i in range(f.getnframes()):
    frame = f.readframes(1)
    mag = 0
    for j in range(f.getsampwidth()):
        if sys.version_info < (3, 0):
            mag += ord(frame[j]) << (8*j)
        else:
            mag += frame[j] << (8*j)
    mag = dec2signed(mag, samplesize)

    slp.append(mag)
    slope = (slp[2] - slp[0])

#    if i > 11823 and i < 12560:
#        _ = 0
#    else:
#        continue

    if abs(slope) > 2000:
        if slope > 0:
            transition = 1
        else:
            transition = -1

    val += transition
    if val < 0:
        val = 0
    if val > 1:
        val = 1

#    if state is "waiting":
#        print(i, mag, "    ", val , "    ", slope, "    ", 0)
#    elif state is "sync":
#        print(i, mag, "    ", val , "    ", slope, "    ", 1)
#    elif state is "preamble":
#        print(i, mag, "    ", val , "    ", slope, "    ", 2)
#    elif state is "data":
#        print(i, mag, "    ", val , "    ", slope, "    ", 3)

    states.write("%d    %d    %d\n"%(i, mag, val))
    mag = val
#       if mag < 0:
#           mag = 0
#       else:
#           mag = 1


    transition = mag - last_signal_level
    last_signal_level = mag

    if state is "waiting":
        if transition == 1:
            state = "sync"
            del sync_clk_pulse[:]
            last_clk_pulse = i
            log.info("entering sync")
        statelog.info("%d  %d  %s  0", i, mag, state)
    
    elif state is "sync":
        if transition == 1:
            #print "got rising edge"
            last_clk_pulse = i
            sync_clk_pulse.append(i)
            #print "sync_clk_pulse len = %d"%(len(sync_clk_pulse))

            # If we get 26 rising edges without a large pause(100 clocks)
            # Consider it a valid sync
            if len(sync_clk_pulse) == 14:
                clk_period = 0
                x2 = 0
                for j in range(1,len(sync_clk_pulse)):
                    cur_clk_period = sync_clk_pulse[j] - sync_clk_pulse[j-1]
                    clk_period += cur_clk_period
                    x2 += math.pow(cur_clk_period,2)

                clk_period = float(clk_period) / (len(sync_clk_pulse) - 1)
                stdev = math.sqrt((float(x2) / (len(sync_clk_pulse) - 1)) - (clk_period * clk_period))      

                log.info("%d  clk len is %.2f (%.2f Hz) stdev= %.2f"%(i, clk_period, 1/(clk_period / 50000.0), stdev))

                # Use the stddev as a marker of a valid clock vs noise
                if stdev < 0.9 and clk_period < 13 and clk_period > 11:
                    log.info("entering PREAMBLE")
                    state = "preamble"
                else:
                    log.info("bad sync CLOCKS going to waiting")
                    state = "waiting"
                    val = 0

        
        elif (i - last_clk_pulse) > 100:
            log.info("LOST SYNC GOING TO WAITING")
            state = "waiting"
            val = 0

        statelog.info("%d  %d  %s  1", i, mag, state)


    elif state is "preamble":
        if transition == -1:    
            #if i - last_clk_pulse < (clk_period * .85):
            log.info("DATA STARTING! @ %d next after %d", i, clk_period * .70)
            state = "data"
            del cur_data[:]
            del bin_data[:]
            cur_data.append(0)
            last_clk_pulse = i
        elif i - last_clk_pulse > (clk_period * 8):
            log.info("LOST SYNC GOING TO WAITING")
            state = "waiting"
            val = 0
        
        statelog.info("%d  %d  %s  2", i, mag, state)
    

    elif state is "data":
        if i > (last_clk_pulse + clk_period * .70) and transition != 0:
            log.info("DATA bit @ %d", i)
            last_clk_pulse = i
            if transition == 1:
                bin_data.append(0)
                bin_data.append(1)
                cur_data.append(1)
                #print "Got 1"
            else:
                bin_data.append(1)
                bin_data.append(0)
                cur_data.append(0)
                #print "Got 0"
        
        elif i - last_clk_pulse > (clk_period * 1.15):
            log.info("LOST SYNC GOING TO WAITING")
            print("Packet Number %d @ %8d  = %s %d"%(packet_number, i, packet2hex(cur_data), len(cur_data)))
            #print(" ".join([str(x) for x in cur_data]))
            #print(" " + "".join([str(x) for x in bin_data]))
            packet_number += 1
            state = "waiting"
            val = 0

        statelog.info("%d  %d  %s  3", i, mag, state)

#bar.finish()
