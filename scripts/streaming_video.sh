gst-launch-1.0 videotestsrc ! video/x-raw,width=704,height=576 ! x264enc ! rtph264pay ! udpsink host=0.0.0.0 port=5000
#gst-launch-1.0 -v videotestsrc ! x264enc ! rtph264pay ! udpsink host=127.0.0.1 port=5000
