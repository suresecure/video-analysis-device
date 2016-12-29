#pragma once

#include <pthread.h>
#include <opencv2/opencv.hpp>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

namespace srzn_video_analysis_device {

class RtspDec {
public:
  RtspDec();
  ~RtspDec();

  void SetRtspSrcUri(const std::string &uri);
  void GetNextSample(cv::Mat &img, bool &first_frame);

  //internal using
  void PushSampleBuffer(unsigned char *buffer, int width, int height,
                        int depth);
private:
  // helpers
  void RestartPipeline();
  int WaitForNewSampleBuffer();
  void PopSampleBuffer(unsigned char *&buffer, int &width, int &height,
                       int &depth);
  //friend GstFlowReturn appsink_new_sample(GstAppSink *appsink, gpointer user_data);

  // members
  GstElement *rtspsrc;
  GstElement *rtph264depay;
  GstElement *appsink;
  GstElement *pipeline;
  GstElement *avdec_h264;

  unsigned int last_frame_clk;

  pthread_mutex_t sample_mutex;
  pthread_mutex_t sample_cond_mutex;
  pthread_cond_t sample_cond;
  unsigned char *sample_buffer;
  int sample_width;
  int sample_height;
  int sample_depth;
  int cur_sample_idx;
  int last_sample_idx;
  int sample_buffer_cap;

  bool new_frame;
};
}
