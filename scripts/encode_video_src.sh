gst-launch-1.0 videotestsrc pattern=18 num-buffers=100 ! video/x-raw,width=1280,height=720 ! x264enc ! h264parse ! qtmux ! filesink location=test.mp4 -e
