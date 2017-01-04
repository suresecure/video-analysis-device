gst-launch-1.0 rtspsrc location=rtsp://192.168.3.83:8554/live ! rtph264depay ! h264parse ! omxh264dec ! xvimagesink
