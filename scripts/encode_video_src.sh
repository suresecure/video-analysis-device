gst-launch-1.0 videotestsrc num-buffers=10000 ! video/x-raw,width=352,height=288 ! x264enc ! h264parse ! qtmux ! filesink location=test.mp4 -e
