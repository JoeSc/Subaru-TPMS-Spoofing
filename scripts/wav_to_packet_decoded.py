import wave
import sys
from collections import deque
import logging
import math
import progressbar
from struct import unpack

logging.basicConfig()
statelog = logging.getLogger("STATE")
statelog.setLevel(logging.WARNING)

log = logging.getLogger("")
log.setLevel(logging.WARNING)

states = open("states.txt", "w")


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
    addr = (packet >> 10) & 0xffffff
    not_addr = int(format(addr,"023b").replace("0","Z").replace("1","0").replace("Z","1"), 2)

    pressure = packet & 0x3ff
    return "0x%x  pre = %x  addr = %x (%x) pressure = %.1f psi"%(packet, prepend, addr, not_addr, pressure/10)
    return "0x%x"%int(bit_string,2)



sample_rate = 0
bit_time = 125E-6

def signal_gen():
    global sample_rate
    f = wave.open(sys.argv[1])
    samplesize = f.getsampwidth() * 8
    sample_rate = f.getframerate()
    channels = f.getnchannels()

    ac_couple_samples = deque(maxlen = 100)

    signal_val = 0

    #bar = progressbar.ProgressBar(redirect_stdout=True, max_value = f.getnframes())
    for samp in range(f.getnframes()):
        #if samp % 1000 == 0:
            #bar.update(samp)


        frame = f.readframes(1)
        mag = None
        if f.getsampwidth() == 1:
            mag, = unpack('B', frame)
            #print("dec = %6d"%mag, end ="  ")
            mag =  (mag / 256)*2 - 1
        elif f.getsampwidth() == 2:
            mag, = unpack('<h', frame[0:2]) 
            #print("dec = %6d"%mag, end ="  ")
            mag /= 32767



        #ac_couple_samples.append(mag)
        #ac_val = sum(ac_couple_samples) / float(len(ac_couple_samples))
        ac_val = 0

        # Include a slight offset so static will result in signal_val of 0
        if mag > ac_val + .0500:
            signal_val = 1
        elif mag < ac_val +.0000:
            signal_val = 0

        yield samp, mag, signal_val, ac_val
    #bar.finish()


last_signal_val = 0
state = "waiting"
transitions = []
packet_number = 0
bin_data = []
man_data = []

for samp, mag, signal_val, avg in signal_gen():

    if state is "waiting":
        state_val = 0
    elif state is "start":
        state_val = 1
    elif state is "sync":
        state_val = 2
    elif state is "preamble":
        state_val = 3
    elif state is "data":
        state_val = 4
    else:
        state_val = 99

    states.write("%d    %.3f    %d    %d  %.3f\n"%(samp, mag, signal_val, state_val, avg))

    transition = signal_val - last_signal_val
    last_signal_val = signal_val

    if transition != 0:
        last_signal_transition = samp
        log.info("%d  TRANSITION %d"%(samp, transition))


    if state is "waiting":
        if transition == 1:
            del transitions[:]
            transitions.append(samp)
            state = "start"
            log.info("entering start")
    elif state is not "data":
        if (samp - last_signal_transition) > 100:
            log.info("%d  LOST SYNC GOING TO WAITING"%(samp))
            state = "waiting"
            signal_val = 0


    if state is "start":
        if transition == -1:
            time_since = (samp - transitions[-1]) / sample_rate
            if 3.5*bit_time < time_since < 4.5*bit_time:
                del transitions[:]
                transitions.append(samp)
                state = "sync"
                log.info("entering sync")

    elif state is "sync":
        if transition != 0:
            transitions.append(samp)
            if len(transitions) == 28:
                clk_period = 0
                x2 = 0
                for j in range(1,len(transitions)):
                    cur_clk_period = transitions[j] - transitions[j-1]
                    clk_period += cur_clk_period
                    x2 += math.pow(cur_clk_period,2)

                clk_period = float(clk_period) / (len(transitions) - 1)
                stdev = math.sqrt((float(x2) / (len(transitions) - 1)) - (clk_period * clk_period))      


                # We captured rising and falling edges so real period is 2x what was found above
                clk_period = 2 * clk_period * (1/sample_rate)
                freq = 1/(clk_period)


                # Use the stddev as a marker of a valid clock vs noise
                if stdev < 3.0 and 1.75*bit_time < clk_period < 2.25*bit_time:
                    log.info("%d GOOD clk len is %.2f (%.2f Hz) stdev= %.2f"%(samp, clk_period, freq, stdev))
                    log.info("entering PREAMBLE")
                    state = "preamble"
                    del transitions[:]
                else:
                    log.info("%d BAD clk len is %.2f (%.2f Hz) stdev= %.2f"%(samp, clk_period, freq, stdev))
                    log.info("bad sync CLOCKS going to waiting")
                    state = "waiting"
                    del transitions[:]

    elif state is "preamble":
        if transition == -1:    
            transitions.append(samp)
            log.info("DATA STARTING! @ %d next after %d", samp, clk_period * .60)
            state = "data"
            del man_data[:]
            del bin_data[:]
            #man_data.append(0)

    elif state is "data":
        time_since = (samp - transitions[-1]) / sample_rate
        if time_since > (clk_period * .75) and transition != 0:
            log.info("DATA bit @ %d", samp)
            transitions.append(samp)
            if transition == 1:
                bin_data.append(0)
                bin_data.append(1)
                man_data.append(1)
                log.info("Got 1")
            else:
                bin_data.append(1)
                bin_data.append(0)
                man_data.append(0)
                log.info("Got 0")
        
        elif time_since > (clk_period * 1.5) and len(man_data) != 37:
            print("BAD  Packet Number %2d @ %8d  = %s  man_bits =%d"%(packet_number, transitions[-1], packet2hex(man_data), len(man_data)))
            #print("last transition = %d, current = %d"%(transitions[-1], samp))
            #print("clk_period = ", clk_period)
            packet_number += 1
            state = "waiting"

        elif time_since > (clk_period * 1.5) and len(man_data) == 37:
            log.info("LOST SYNC GOING TO WAITING")
            print("GOOD Packet Number %2d @ %8d  = %s  man_bits =%d"%(packet_number, transitions[-1], packet2hex(man_data), len(man_data)))
            #print(" " + "".join([str(x) for x in bin_data]))
            #print("  " + " ".join([str(x) for x in man_data]))
            packet_number += 1
            state = "waiting"
