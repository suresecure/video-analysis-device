gst-launch-1.0 -v filesrc location=test.mp4 ! qtdemux ! h264parse ! rtph264pay ! udpsink host=127.0.0.1 port=5000

