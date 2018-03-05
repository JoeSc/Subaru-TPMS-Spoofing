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

#ifndef INCLUDED_CUSTOM_SUBARU_TPMS_DECODE_CPP_CHAR_IMPL_H
#define INCLUDED_CUSTOM_SUBARU_TPMS_DECODE_CPP_CHAR_IMPL_H

#include <custom/subaru_tpms_decode_cpp_char.h>
#include <math.h>
#include <inttypes.h>

namespace gr {
  namespace custom {

    class subaru_tpms_decode_cpp_char_impl : public subaru_tpms_decode_cpp_char
    {
     private:
		 double sample_rate;
      // Nothing to declare in this block.
	  double bit_time;
	  uint32_t sample;
	  uint32_t last_transition;
	  uint8_t level;
	  uint8_t last_level;
	  enum states {state_waiting, state_start, state_sync, state_preamble, state_data}packet_state;
	  uint32_t transitions[128];
	  uint8_t transition_cnt;
	  uint64_t packet_data;
	  uint8_t packet_bit_cnt;

	  uint32_t packet_number;

	  double clk_period;

	  char buffer [128];

     public:
      subaru_tpms_decode_cpp_char_impl(double samp_rate);
      ~subaru_tpms_decode_cpp_char_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
      char *packet_to_string(uint64_t packet);
    };


  } // namespace custom
} // namespace gr

#endif /* INCLUDED_CUSTOM_SUBARU_TPMS_DECODE_CPP_CHAR_IMPL_H */

