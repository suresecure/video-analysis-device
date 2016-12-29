#include <gst/gst.h>
//#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#include "rtspdec.h"

namespace srzn_video_analysis_device {

static GstBus *bus;

static unsigned int get_cur_second() {
  struct timeval tim;
  gettimeofday(&tim, NULL);
  return tim.tv_sec;
}

static void rtspsrc_new_pad_call_back(GstElement *element, GstPad *pad,
                                      gpointer data) {
  g_print("Link rtspsrc and rtph264depay!\n");
  gst_element_link(element, GST_ELEMENT(data));
}

void RtspDec::PushSampleBuffer(unsigned char *buffer, int width, int height,
                               int depth) {
  last_frame_clk = get_cur_second();
  int buffer_size = width * height * depth;
  pthread_mutex_lock(&sample_mutex);
  if (sample_buffer == NULL || width != sample_width ||
      height != sample_height || depth != sample_depth) {
    if (sample_buffer != NULL)
      delete[] sample_buffer;
    sample_buffer = new unsigned char[buffer_size * sample_buffer_cap];
    sample_width = width;
    sample_height = height;
    sample_depth = depth;
  }

  cur_sample_idx = (cur_sample_idx + 1) % sample_buffer_cap;
  if (last_sample_idx == cur_sample_idx) {
    g_print("drop one buffer!\n");
    last_sample_idx = (last_sample_idx + 1) % sample_buffer_cap;
  }

  memcpy(sample_buffer + cur_sample_idx * buffer_size, buffer, buffer_size);

  pthread_mutex_unlock(&sample_mutex);

  pthread_cond_signal(&sample_cond);
  // g_print("Push a new sample buffer!\n");
}

void RtspDec::PopSampleBuffer(unsigned char *&buffer, int &width, int &height,
                              int &depth) {
  // read cur_sample_idx and last_sample_idx only first to determine if there is
  // a buffer available
  if (cur_sample_idx == last_sample_idx) {
    buffer = NULL;
    return;
  }
  pthread_mutex_lock(&sample_mutex);
  width = sample_width;
  height = sample_height;
  depth = sample_depth;
  int buffer_size = width * height * depth;
  buffer = sample_buffer + buffer_size * last_sample_idx;
  last_sample_idx = (last_sample_idx + 1) % sample_buffer_cap;

  pthread_mutex_unlock(&sample_mutex);
}

int RtspDec::WaitForNewSampleBuffer() {
  if (cur_sample_idx == last_sample_idx) {
    // buffer = NULL;
    // return;
    timespec time_out_interval;
    time_out_interval.tv_sec = 2;
    time_out_interval.tv_nsec = 0;
    pthread_mutex_lock(&sample_cond_mutex);
    int wait_ret = pthread_cond_timedwait(&sample_cond, &sample_cond_mutex,
                                          &time_out_interval);
    pthread_mutex_unlock(&sample_cond_mutex);
    if (wait_ret == 0)
      return 0;
    else
      return ETIMEDOUT;
  } else
    return 0;
}

static GstFlowReturn appsink_new_sample(GstAppSink *appsink,
                                        gpointer user_data) {
  //g_print("new sample thread: %lx\n", pthread_self());
  GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
  GstCaps *caps = gst_sample_get_caps(sample);
  GstStructure *capsStruct = gst_caps_get_structure(caps, 0);

  int width, height;
  gst_structure_get_int(capsStruct, "width", &width);
  gst_structure_get_int(capsStruct, "height", &height);

  // Actual compressed image is stored inside GstSample.
  GstBuffer *buffer = gst_sample_get_buffer(sample);
  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_READ);

  RtspDec *rtsp_dec = (RtspDec *)user_data;

  rtsp_dec->PushSampleBuffer(map.data, width, height, 1);

  gst_buffer_unmap(buffer, &map);
  gst_sample_unref(sample);
  // sleep(5);
  //g_print("push sample buffer over!\n");
  return GST_FLOW_OK;
}

// Bus messages processing, similar to all gstreamer examples
gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
  GMainLoop *loop = (GMainLoop *)data;

  switch (GST_MESSAGE_TYPE(msg)) {

  case GST_MESSAGE_EOS:
    g_print("Bus call: end of stream\n");
    // g_main_loop_quit(loop);
    // restart pipeline
    // gst_element_set_state(pipeline, GST_STATE_NULL);
    // gst_element_set_state(pipeline, GST_STATE_PLAYING);
    // gst_element_set_state(pipeline, GST_STATE_READY);
    break;

  case GST_MESSAGE_ERROR:
    gchar *debug;
    GError *error;

    gst_message_parse_error(msg, &error, &debug);
    g_free(debug);

    g_print("Bus call: error %s\n", error->message);
    g_error_free(error);

    // restart pipeline periodically
    // gst_element_set_state(pipeline, GST_STATE_READY);
    // gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // g_main_loop_quit(loop);

    break;

  case GST_MESSAGE_ELEMENT:
    g_print("Element %s message: %s\n", GST_MESSAGE_SRC_NAME(msg),
            gst_structure_to_string(gst_message_get_structure(msg)));
    // printf("ELEMENT message:\n");
    // printf("%s!\n", GST_MESSAGE_SRC_NAME(msg));
    break;

  case GST_MESSAGE_STATE_CHANGED:
    GstState old_state, new_state;
    gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
    g_print("State Changed Message Element %s changed state from %s to %s.\n",
            GST_OBJECT_NAME(msg->src), gst_element_state_get_name(old_state),
            gst_element_state_get_name(new_state));
    break;

  default:
    // printf("%s!\n", GST_MESSAGE_TYPE_NAME(msg));
    break;
  }

  return TRUE;
}

// Creates and sets up Gstreamer pipeline for JPEG encoding.
void *bus_msg_thread(void *pThreadParam) {
  g_print("bus_msg_thread start!\n");
  while (1) {
    GstMessage *msg;
    while ((msg = gst_bus_pop(bus))) {
      bus_call(bus, msg, NULL);
      gst_message_unref(msg);
    }
    sleep(1);
  }
  return NULL;
}

// Starts GstreamerThread that remains in memory and compresses frames as being
// fed by user app.
bool start_bus_msg_thread() {
  unsigned long GtkThreadId;
  pthread_attr_t GtkAttr;

  // Start thread
  int result = pthread_attr_init(&GtkAttr);
  if (result != 0) {
    fprintf(stderr, "pthread_attr_init returned error %d\n", result);
    return false;
  }

  void *pParam = NULL;
  result = pthread_create(&GtkThreadId, &GtkAttr, bus_msg_thread, pParam);
  if (result != 0) {
    fprintf(stderr, "pthread_create returned error %d\n", result);
    return false;
  }

  return true;
}

RtspDec::RtspDec() {
  gst_init(NULL, NULL); // Will abort if GStreamer init error found

  pthread_mutex_init(&sample_mutex, NULL);
  pthread_mutex_init(&sample_cond_mutex, NULL);
  pthread_cond_init(&sample_cond, NULL);
  sample_buffer = NULL;
  sample_width = 0;
  sample_height = 0;
  sample_depth = 0;
  cur_sample_idx = 0;
  last_sample_idx = 0;
  sample_buffer_cap = 3;

  pipeline = gst_pipeline_new("mypipeline");

  rtspsrc = gst_element_factory_make("rtspsrc", "myrtspsrc");
  rtph264depay = gst_element_factory_make("rtph264depay", "myrtph264depay");
  appsink = gst_element_factory_make("appsink", "myappsink");
  avdec_h264 = gst_element_factory_make("avdec_h264", "myavdec_h264");

  g_object_set(G_OBJECT(appsink), "emit-signals", true, NULL);

  gst_bin_add_many(GST_BIN(pipeline), rtspsrc, rtph264depay, avdec_h264,
                   appsink, NULL);

  gst_element_link(rtspsrc, rtph264depay);
  gst_element_link(rtph264depay, avdec_h264);
  gst_element_link(avdec_h264, appsink);

  // add a message handler
  // bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

  g_signal_connect(rtspsrc, "pad-added", G_CALLBACK(rtspsrc_new_pad_call_back),
                   rtph264depay);
  g_signal_connect(appsink, "new-sample", G_CALLBACK(appsink_new_sample), this);

  last_frame_clk = get_cur_second();
  new_frame = true;

  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  start_bus_msg_thread();
}
RtspDec::~RtspDec() {}

void RtspDec::SetRtspSrcUri(const std::string &uri) {
  g_object_set(G_OBJECT(rtspsrc), "location", uri.c_str(), NULL);
  //gst_element_set_state(pipeline, GST_STATE_READY);
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  new_frame = true;
}

void RtspDec::GetNextSample(cv::Mat &img, bool &first_frame) {
  int wait_ret = WaitForNewSampleBuffer();
  if (wait_ret == 0) {
    unsigned char *buffer;
    int width, height, depth;
    PopSampleBuffer(buffer, width, height, depth);
    img = cv::Mat(height, width, CV_8UC1, buffer);
    first_frame = new_frame;
    new_frame = false;
  } else {
    unsigned int cur_clk = get_cur_second();
    if (cur_clk - last_frame_clk > 10) {
      g_print("too long to not get new buffer, restart pipeline!\n");
      last_frame_clk = cur_clk;
      gst_element_set_state(pipeline, GST_STATE_READY);
      gst_element_set_state(pipeline, GST_STATE_PLAYING);
      new_frame = true;
    }
  }
  return;
}
}

using namespace srzn_video_analysis_device;
int main(int argc, char *argv[]) {
  RtspDec rtsp_dec;
  rtsp_dec.SetRtspSrcUri("rtsp://localhost:8554/live");

  for (int i = 0;; ++i) {
    cv::Mat img;
    bool first_frame;
    rtsp_dec.GetNextSample(img, first_frame);
    if (img.cols > 0) {
      cv::imshow("img", img);
      cv::waitKey(1);
      if (first_frame)
        g_print("First Frame!\n");
    }
  }

  return 0;
}
