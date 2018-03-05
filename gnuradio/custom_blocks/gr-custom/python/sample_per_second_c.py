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

import time
import numpy
from gnuradio import gr

class sample_per_second_c(gr.sync_block):
    """
    docstring for block sample_per_second_c
    """
    def __init__(self):
        gr.sync_block.__init__(self,
            name="sample_per_second_c",
            in_sig=[numpy.complex64],
            out_sig=[numpy.complex64])
        self.count = 0
        self.last_update_count = 0
        self.last_update_time = time.time()


    def work(self, input_items, output_items):
        in0 = input_items[0]
        out = output_items[0]
        self.count += len(in0)

        if time.time() - self.last_update_time > 1:
            print("Samples in last second = %d"%(self.count - self.last_update_count))
            self.last_update_count = self.count
            self.last_update_time = time.time()

        # <+signal processing here+>
        out[:] = in0
        return len(output_items[0])

