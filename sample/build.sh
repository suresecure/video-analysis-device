../scripts/encode_video_src.sh
cp ../scripts/test.mp4 ./
g++ gst_sample.cpp `pkg-config --cflags --libs gstreamer-1.0` -o gst_sample.out
