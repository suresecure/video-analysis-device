gst-launch-1.0 filesrc location=test.mp4 ! qtdemux ! avdec_h264 ! videoconvert ! ximagesink --gst-debug-level=4
