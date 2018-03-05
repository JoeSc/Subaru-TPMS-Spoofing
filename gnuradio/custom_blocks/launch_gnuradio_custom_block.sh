DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

GRC_BLOCKS_PATH=$DIR/gnuradio-custom/share/gnuradio/grc/blocks/:$GRC_BLOCKS_PATH \
CPATH=$DIR/gnuradio-custom/include:$CPATH \
LD_LIBRARY_PATH=$DIR/gnuradio-custom/lib/:$LD_LIBRARY_PATH \
PYTHONPATH=$DIR/gnuradio-custom/lib/python2.7/dist-packages/:/usr/local/lib/python2.7/dist-packages/osmosdr/:$PYTHONPATH \
gnuradio-companion
