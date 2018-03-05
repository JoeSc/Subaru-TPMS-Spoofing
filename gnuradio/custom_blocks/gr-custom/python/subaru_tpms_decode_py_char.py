#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
# Copyright 2018 <+YOU OR YOUR COMPANY+>.
# 
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

import numpy
from gnuradio import gr
import math

class subaru_tpms_decode_py_char(gr.sync_block):
    """
    docstring for block subaru_tpms_decode_py_char
    """
    def __init__(self, samp_rate):
        gr.sync_block.__init__(self,
            name="subaru_tpms_decode_py_char",
            in_sig=[numpy.uint8],
            out_sig=None)

        self.sample = 0
        self.level = 0
        self.last_level = 0
        self.state="waiting"
        self.transitions = []
        self.man_data = []
        self.bin_data = []
        self.packet_number = 0


        self.sample_rate = float(samp_rate)
        self.bit_time = 125E-6


    def packet2hex(self, packet):
        if len(packet) == 0:
            return "NO DATA"
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


    def work(self, input_items, output_items):
        in0 = input_items[0]
        # Requires 0/1 inputs
        for x in in0:
            transition = x - self.last_level
            self.last_level = int(x)

            if transition != 0:
                self.last_transition = self.sample

            if self.state is "waiting":
                if transition == 1:
                    #print("Going to start @ %d"%self.sample)
                    del self.transitions[:]
                    self.transitions.append(self.sample)
                    self.state = "start"
            elif self.state is not "data":
                if (self.sample - self.last_transition) > 200:
                    self.state = "waiting"

            if self.state is "start":
                if transition == -1:
                    time_since = (self.sample - self.transitions[-1]) / self.sample_rate
                    #print("python time since = %.8f"%(time_since))
                    if 3.5*self.bit_time < time_since < 4.5*self.bit_time:
                        #print("Going to sync @ %d"%self.sample)
                        self.state = "sync"
                        del self.transitions[:]
                        self.transitions.append(self.sample)

            elif self.state is "sync":
                if transition != 0:
                    self.transitions.append(self.sample)
                    if len(self.transitions) == 28:
                        self.clk_period = 0
                        x2 = 0
                        for j in range(1,len(self.transitions)):
                            cur_clk_period = self.transitions[j] - self.transitions[j-1]
                            self.clk_period += cur_clk_period
                            x2 += math.pow(cur_clk_period,2)

                        self.clk_period = float(self.clk_period) / (len(self.transitions) - 1)
                        stdev = math.sqrt((float(x2) / (len(self.transitions) - 1)) - (self.clk_period * self.clk_period))      


                        # We captured rising and falling edges so real period is 2x what was found above
                        self.clk_period = 2 * self.clk_period * (1/self.sample_rate)
                        freq = 1/(self.clk_period)


                        # Use the stddev as a marker of a valid clock vs noise
                        if stdev < 3.0 and 1.75*self.bit_time < self.clk_period < 2.25*self.bit_time:
                            #print("%d GOOD clk len is %.1fus (%.2f Hz) stdev= %.2f"%(self.sample, self.clk_period*1E6, freq, stdev))
                            #print("entering PREAMBLE")
                            self.state = "preamble"
                            del self.transitions[:]
                        else:
                            #print("%d BAD clk len is %.1fus (%.2f Hz) stdev= %.2f"%(self.sample, self.clk_period*1E6, freq, stdev))
                            #print("bad sync CLOCKS going to waiting")
                            self.state = "waiting"
                            del self.transitions[:]

            elif self.state is "preamble":
                if transition == -1:    
                    self.transitions.append(self.sample)
                    #print("DATA STARTING! @ %d next after %d", self.sample, self.clk_period * .60)
                    self.state = "data"
                    del self.man_data[:]
                    del self.bin_data[:]


            elif self.state is "data":
                time_since = (self.sample - self.transitions[-1]) / self.sample_rate
                #print("time_since = %.2fus"%(time_since*1E6))
                #print("self.clk_period = %.2fus"%(self.clk_period*1E6))

                if time_since > (self.clk_period * .75) and transition != 0:
                    #print("DATA bit @ %d", self.sample)
                    self.transitions.append(self.sample)
                    if transition == 1:
                        self.bin_data.append(0)
                        self.bin_data.append(1)
                        self.man_data.append(1)
                        #print("Got 1")
                    else:
                        self.bin_data.append(1)
                        self.bin_data.append(0)
                        self.man_data.append(0)
                        #print("Got 0")
                
                elif time_since > (self.clk_period * 1.5) and len(self.man_data) != 37:
                    #print("last transition = %d, current = %d"%(self.transitions[-1], self.sample))
                    #print("self.clk_period = ", self.clk_period)
                    print("BAD Packet Number %d @ %8d  = %s  man_bits =%d"%(self.packet_number, self.transitions[-1], self.packet2hex(self.man_data), len(self.man_data)))
                    self.packet_number += 1
                    self.state = "waiting"

                elif time_since > (self.clk_period * 1.5) and len(self.man_data) == 37:
                    #print("LOST SYNC GOING TO WAITING")
                    print("Complete Packet Number %d @ %8d  = %s  man_bits =%d"%(self.packet_number, self.transitions[-1], self.packet2hex(self.man_data), len(self.man_data)))
                    #print(" " + "".join([str(x) for x in self.bin_data]))
                    #print("  " + " ".join([str(x) for x in self.man_data]))
                    self.packet_number += 1
                    self.state = "waiting"






















            #if self.state is "waiting":
            #    state_val = 0
            #elif self.state is "start":
            #    state_val = 1
            #elif self.state is "sync":
            #    state_val = 2
            #elif self.state is "preamble":
            #    state_val = 3
            #elif self.state is "data":
            #    state_val = 4
            #else:
            #    state_val = 99
            #self.f.write("%d    %.3f   %d   %d\n"%(self.sample, x, self.level, state_val))
            self.sample += 1





        #print(in0)
        return len(input_items[0])

