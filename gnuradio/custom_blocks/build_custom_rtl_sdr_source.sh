DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

git clone git://git.osmocom.org/rtl-sdr $DIR/rtl-sdr
cd $DIR/rtl-sdr
git checkout 4520f001d85f01d051eaa42af7b18b6ef0837e14
mkdir build && cd build && cmake  -DDETACH_KERNEL_DRIVER=ON -DINSTALL_UDEV_RULES=ON ../ && sudo make install


git clone git://git.osmocom.org/gr-osmosdr $DIR/gr-osmosdr
cd $DIR/gr-osmosdr
git checkout c653754dde5e2cf682965e939cc016fbddbd45e4
cat << EOF | git am -
From 217f0cd7a63b3d5aae74f1fcebd849548e2ade7e Mon Sep 17 00:00:00 2001
From: Joe Schaack <JoeSc@users.noreply.github.com>
Date: Sun, 11 Mar 2018 03:39:50 +0000
Subject: [PATCH] Tweak autoamtic gain mode to behave like rtl_sdr application.

---
 lib/rtl/rtl_source_c.cc | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)

diff --git a/lib/rtl/rtl_source_c.cc b/lib/rtl/rtl_source_c.cc
index a371464..52b5257 100644
--- a/lib/rtl/rtl_source_c.cc
+++ b/lib/rtl/rtl_source_c.cc
@@ -574,7 +574,6 @@ bool rtl_source_c::set_gain_mode( bool automatic, size_t chan )
       _auto_gain = automatic;
     }
 
-    rtlsdr_set_agc_mode(_dev, int(automatic));
   }
 
   return get_gain_mode(chan);
@@ -587,6 +586,7 @@ bool rtl_source_c::get_gain_mode( size_t chan )
 
 double rtl_source_c::set_gain( double gain, size_t chan )
 {
+	return 0.0;
   osmosdr::gain_range_t rf_gains = rtl_source_c::get_gain_range( chan );
 
   if (_dev) {
-- 
2.11.0
EOF
mkdir build && cd build && cmake  -DENABLE_RTL=ON -DENABLE_REDPITAYA=OFF -DENABLE_RFSPACE=OFF -DENABLE_UHD=OFF -DENABLE_FCDPP=OFF -DENABLE_FCD=OFF -DCMAKE_INSTALL_PREFIX=../../gnuradio-custom ../ && make install
