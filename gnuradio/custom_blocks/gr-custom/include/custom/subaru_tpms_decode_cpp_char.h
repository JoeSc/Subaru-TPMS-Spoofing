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


#ifndef INCLUDED_CUSTOM_SUBARU_TPMS_DECODE_CPP_CHAR_H
#define INCLUDED_CUSTOM_SUBARU_TPMS_DECODE_CPP_CHAR_H

#include <custom/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace custom {

    /*!
     * \brief <+description of block+>
     * \ingroup custom
     *
     */
    class CUSTOM_API subaru_tpms_decode_cpp_char : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<subaru_tpms_decode_cpp_char> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of custom::subaru_tpms_decode_cpp_char.
       *
       * To avoid accidental use of raw pointers, custom::subaru_tpms_decode_cpp_char's
       * constructor is in a private implementation
       * class. custom::subaru_tpms_decode_cpp_char::make is the public interface for
       * creating new instances.
       */
      static sptr make(double samp_rate);
    };

  } // namespace custom
} // namespace gr

#endif /* INCLUDED_CUSTOM_SUBARU_TPMS_DECODE_CPP_CHAR_H */

