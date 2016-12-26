gst-launch-1.0 rtspsrc location=rtsp://192.168.3.83:8554/live ! queue ! rtph264depay ! queue ! avdec_h264 ! xvimagesink
