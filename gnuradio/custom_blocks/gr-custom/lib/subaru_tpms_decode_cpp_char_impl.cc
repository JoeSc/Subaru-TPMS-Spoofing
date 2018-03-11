/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "subaru_tpms_decode_cpp_char_impl.h"

namespace gr {
  namespace custom {

    subaru_tpms_decode_cpp_char::sptr
    subaru_tpms_decode_cpp_char::make(double samp_rate)
    {
      return gnuradio::get_initial_sptr
        (new subaru_tpms_decode_cpp_char_impl(samp_rate));
    }

    /*
     * The private constructor
     */
    subaru_tpms_decode_cpp_char_impl::subaru_tpms_decode_cpp_char_impl(double samp_rate)
      : gr::sync_block("subaru_tpms_decode_cpp_char",
              gr::io_signature::make(1, 1, sizeof(uint8_t)),
              gr::io_signature::make(0, 0, 0)),
		sample_rate(samp_rate)
    {
	  sample = 0;
	  level = 0;
	  packet_state = state_waiting;
	  transition_cnt = 0;
	  packet_number = 0;
	  bit_time = 125E-6;
	}

    /*
     * Our virtual destructor.
     */
    subaru_tpms_decode_cpp_char_impl::~subaru_tpms_decode_cpp_char_impl()
    {
    }

    char *subaru_tpms_decode_cpp_char_impl::packet_to_string(uint64_t packet)
	{
		uint32_t prepend = packet >> 34;
		uint32_t addr = (packet >> 10) & 0xffffff;
		uint32_t pressure = packet & 0x3ff;
		sprintf(buffer, " pre = %x  addr = %x  %.1f psi", prepend, addr, pressure/20.0);
		return buffer;
	}

    int
    subaru_tpms_decode_cpp_char_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const uint8_t *in = (const uint8_t *) input_items[0];

	  //printf("NUMBER OF ITEMS = %d\n", noutput_items);
      // Do <+signal processing+>
	  int8_t transition;
	  for(int i=0; i<noutput_items; i++, sample++)
	  {
		transition = in[i] - last_level;
        last_level = in[i];

		if(transition != 0)
			last_transition = sample;

		if(packet_state == state_waiting)
		{
			if (transition == 1)
			{
				transition_cnt = 0;
				transitions[transition_cnt++] = sample;
				packet_state = state_start;
				//printf("Going to start @ %ld\n", sample);
			}
		} else if (packet_state != state_data) {
			//printf("HERE @ %ld\n", sample);
            if (((sample - last_transition) / sample_rate) > (6*bit_time))
			{
            	packet_state = state_waiting;
				//printf("FALLING BACK TO Going to waiting state = %d @ %ld\n", sample, packet_state);
			}
		}



        if (packet_state == state_start) 
		{
            if (transition == -1)
			{
                double time_since = (sample - transitions[transition_cnt - 1]) / sample_rate;
                //printf("time since = %.8f\n", time_since);
                //printf("bit time = %.8f\n", bit_time);

                if ( ((3.5 * bit_time) < time_since) && (time_since < (4.5 * bit_time)))
				{
                	packet_state = state_sync;
					transition_cnt = 0;
					transitions[transition_cnt++] = sample;
					//printf("Going to sync @ %ld\n", sample);
				}
			}
		} else if (packet_state == state_sync) {
            if (transition != 0)
			{
				transitions[transition_cnt++] = sample;
				if (transition_cnt == 28) {
					clk_period = 0;
					double x2 = 0;

					uint32_t cur_clk_period = 0;
                    for (int j = 1; j < transition_cnt; j++)
					{
                    	cur_clk_period = transitions[j] - transitions[j-1];
                    	clk_period += cur_clk_period;
						x2 += pow(cur_clk_period,2);
					}
					clk_period = clk_period / (transition_cnt - 1);
					double stdev = sqrt((x2 / (transition_cnt - 1)) - (clk_period * clk_period));

                    // We captured rising and falling edges so real period is 2x what was found above
                    clk_period = 2 * clk_period * (1/sample_rate);
                    double freq = 1/(clk_period);

                    // Use the stddev as a marker of a valid clock vs noise
                    if ((stdev < 3.0) && (1.75*bit_time < clk_period) && (clk_period < 2.25*bit_time))
					{
                        //printf("%d GOOD clk len is %.1fus (%.2f Hz) stdev= %.2f",sample, clk_period*1E6, freq, stdev);
                        //#print("entering PREAMBLE")
                        packet_state = state_preamble;
						transition_cnt = 0;
						//printf("Going to preamble @ %ld\n", sample);
					} else {
                        //#print("%d BAD clk len is %.1fus (%.2f Hz) stdev= %.2f"%(sample, clk_period*1E6, freq, stdev))
                        //#print("bad sync CLOCKS going to state_waiting")
                        packet_state = state_waiting;
						transition_cnt = 0;
					}





				}

			}






		} else if (packet_state == state_preamble) {
        	if (transition == -1) {
				transitions[transition_cnt++] = sample;
            	//#print("DATA STARTING! @ %d next after %d", self.sample, self.clk_period * .60)
            	packet_state = state_data;
				packet_data = 0;
				packet_bit_cnt = 0;
			}
		} else if (packet_state == state_data) {
            double time_since = (sample - transitions[transition_cnt - 1]) / sample_rate;
            if ((time_since > (clk_period * .75)) && (transition != 0) )
			{
				transitions[transition_cnt++] = sample;
				packet_bit_cnt += 1;
				if (transition == 1)
				{
					//packet_data |= (1ULL << (64 - packet_bit_cnt));
					packet_data = (packet_data << 1) | 0;
				} else {
					packet_data = (packet_data << 1) | 1;
				}
			} else if ((time_since > (clk_period * 1.5)) && (packet_bit_cnt != 37)) {
				printf("BAD      Packet Number %d @ %8d  0x%" PRIx64 "\n", packet_number++, transitions[transition_cnt - 1], packet_data);
				packet_state = state_waiting;
			} else if ((time_since > (clk_period * 1.5)) && (packet_bit_cnt == 37)) {
				printf("GOOD     Packet Number %d @ %8d  0x%" PRIx64 " %s\n", packet_number++, transitions[transition_cnt - 1], packet_data, packet_to_string(packet_data));
				packet_state = state_waiting;
			}
		}
	  }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace custom */
} /* namespace gr */

