#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Top Block
# Generated: Tue Mar 20 07:40:28 2018
##################################################

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"

from gnuradio import analog
from gnuradio import blocks
from gnuradio import digital
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import chdir  # embedded python module
import custom
import wx


class top_block(grc_wxgui.top_block_gui):

    def __init__(self):
        grc_wxgui.top_block_gui.__init__(self, title="Top Block")
        _icon_path = "/usr/share/icons/hicolor/32x32/apps/gnuradio-grc.png"
        self.SetIcon(wx.Icon(_icon_path, wx.BITMAP_TYPE_ANY))

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 2048000
        self.decim_rate = decim_rate = 20

        ##################################################
        # Blocks
        ##################################################
        self.digital_binary_slicer_fb_0 = digital.binary_slicer_fb()
        self.custom_subaru_tpms_decode_cpp_char_0 = custom.subaru_tpms_decode_cpp_char(samp_rate/decim_rate)
        self.custom_sample_per_second_c_0 = custom.sample_per_second_c()
        self.blocks_uchar_to_float_0_0 = blocks.uchar_to_float()
        self.blocks_uchar_to_float_0 = blocks.uchar_to_float()
        self.blocks_multiply_const_vxx_0_0 = blocks.multiply_const_vff((1/128., ))
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vff((1/128., ))
        self.blocks_float_to_complex_0 = blocks.float_to_complex(1)
        self.blocks_file_source_1 = blocks.file_source(gr.sizeof_char*1, "../../Examples/1/rtl_sdr_example.c8", False)
        self.blocks_deinterleave_0 = blocks.deinterleave(gr.sizeof_char*1, 1)
        self.blocks_add_const_vxx_0_0 = blocks.add_const_vff((-127, ))
        self.blocks_add_const_vxx_0 = blocks.add_const_vff((-127, ))
        self.analog_am_demod_cf_0 = analog.am_demod_cf(
        	channel_rate=30000,
        	audio_decim=decim_rate,
        	audio_pass=750,
        	audio_stop=1500,
        )

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_am_demod_cf_0, 0), (self.digital_binary_slicer_fb_0, 0))    
        self.connect((self.blocks_add_const_vxx_0, 0), (self.blocks_multiply_const_vxx_0, 0))    
        self.connect((self.blocks_add_const_vxx_0_0, 0), (self.blocks_multiply_const_vxx_0_0, 0))    
        self.connect((self.blocks_deinterleave_0, 0), (self.blocks_uchar_to_float_0, 0))    
        self.connect((self.blocks_deinterleave_0, 1), (self.blocks_uchar_to_float_0_0, 0))    
        self.connect((self.blocks_file_source_1, 0), (self.blocks_deinterleave_0, 0))    
        self.connect((self.blocks_float_to_complex_0, 0), (self.custom_sample_per_second_c_0, 0))    
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.blocks_float_to_complex_0, 0))    
        self.connect((self.blocks_multiply_const_vxx_0_0, 0), (self.blocks_float_to_complex_0, 1))    
        self.connect((self.blocks_uchar_to_float_0, 0), (self.blocks_add_const_vxx_0, 0))    
        self.connect((self.blocks_uchar_to_float_0_0, 0), (self.blocks_add_const_vxx_0_0, 0))    
        self.connect((self.custom_sample_per_second_c_0, 0), (self.analog_am_demod_cf_0, 0))    
        self.connect((self.digital_binary_slicer_fb_0, 0), (self.custom_subaru_tpms_decode_cpp_char_0, 0))    

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)

    def get_decim_rate(self):
        return self.decim_rate

    def set_decim_rate(self, decim_rate):
        self.decim_rate = decim_rate


def main(top_block_cls=top_block, options=None):

    tb = top_block_cls()
    tb.Start(True)
    tb.Wait()


if __name__ == '__main__':
    main()
