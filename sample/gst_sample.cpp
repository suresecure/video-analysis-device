#include <gst/gst.h>
//#include <gst/app/gstappsrc.h>
//#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

// static GstElement *appsrc;
static GstElement *filesrc;
static GstElement *rtspsrc, *rtph264depay;
// static GstElement *appsink;
// static GstElement *pipeline, *vidconv, *x264enc, *qtmux;
static GstElement *pipeline, *qtdemux, *h264parse, *h264dec;
static GstElement *avdec_h264;
static GstElement *imagesink;
static GstBus *bus;

static unsigned int last_frame_clk;

unsigned int get_cur_second() {
  struct timeval tim;
  gettimeofday(&tim, NULL);
  return tim.tv_sec;
}

unsigned int MyGetTickCount() {
  struct timeval tim;
  gettimeofday(&tim, NULL);
  unsigned int t = ((tim.tv_sec * 1000) + (tim.tv_usec / 1000)) & 0xffffffff;
  return t;
}

// static void cb_new_pad(GstElement *element, GstPad *pad, gpointer data) {
// gchar *name;

// name = gst_pad_get_name(pad);
// if (strcmp(name, "video_0") == 0 &&
//!gst_element_link_pads(qtdemux, name, avdec_h264, "sink")) {
// printf("link demux-decoder fail\n");
//}
// g_free(name);
//}

static void new_pad_call_back(GstElement *element, GstPad *pad, gpointer data) {
  g_print("rtspsrc new pad callback thread: %lx\n", pthread_self());
  gchar *name;
  name = gst_element_get_name(element);
  g_print("%s\n", name);
  g_free(name);
  gst_element_link(rtspsrc, rtph264depay);
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
      // Call bus message handler
      bus_call(bus, msg, NULL);
      gst_message_unref(msg);
    }
    unsigned int cur_clk = get_cur_second();
    if (cur_clk > last_frame_clk + 10) {
      g_print("too long not to get new frame, restart the pipeline!\n");
      last_frame_clk = cur_clk;
      gst_element_set_state(pipeline, GST_STATE_READY);
      gst_element_set_state(pipeline, GST_STATE_PLAYING);
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

int main(int argc, char *argv[]) {
  // g_print("main thread: %lx\n", pthread_self());
  gst_init(NULL, NULL); // Will abort if GStreamer init error found

  pipeline = gst_pipeline_new("mypipeline");

  filesrc = gst_element_factory_make("filesrc", "myfilesrc");
  // rtspsrc = gst_element_factory_make("rtspsrc", "myrtspsrc");
  // rtph264depay = gst_element_factory_make("rtph264depay", "myrtph264depay");
  // appsink = gst_element_factory_make("appsink", "myappsink");
  qtdemux = gst_element_factory_make("qtdemux", "myqtdemux");
  avdec_h264 = gst_element_factory_make("avdec_h264", "myavdec_h264");
  imagesink = gst_element_factory_make("xvimagesink", "myxvimagesink");

  g_object_set(G_OBJECT(filesrc), "location", "/home/mythxcq/test_cif.mp4",
               NULL);
  // g_object_set(G_OBJECT(rtspsrc), "location", "rtsp://localhost:8554/live",
  // NULL);
  // g_object_set(G_OBJECT(rtspsrc), "udp-reconnect", true, NULL);

  gst_bin_add_many(GST_BIN(pipeline), rtspsrc, rtph264depay, avdec_h264,
                   appsink, NULL);

  // elements must be linked after been added
  gst_element_link(filesrc, qtdemux);
  //gst_element_link(rtspsrc, rtph264depay);
  //gst_element_link(rtph264depay, avdec_h264);
  gst_element_link(avdec_h264, appsink);

  // add a message handler
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

  g_signal_connect(rtspsrc, "pad-added", G_CALLBACK(new_pad_call_back), NULL);

  // qtdemux and decoder are dynamically linked by signal callback
  // g_signal_connect(qtdemux, "pad-added", G_CALLBACK(cb_new_pad), NULL);
  last_frame_clk = get_cur_second();
  // start_bus_msg_thread();

  fprintf(stderr, "Setting g_main_loop_run to GST_STATE_PLAYING\n");
  // Start pipeline so it could process incoming data
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  int ticks = MyGetTickCount();
  for (int i = 0;; ++i) {
    int wait_ret = wait_for_new_sample_buffer();
    if (wait_ret == 0) {
      last_frame_clk = get_cur_second();
      unsigned char *buffer;
      int width, height, depth;
      pop_sample_buffer(buffer, width, height, depth);
      cv::Mat img(height, width, CV_8UC1, buffer);
      cv::imshow("yes", img);
      cv::waitKey(60);
    } else {
      unsigned int cur_clk = get_cur_second();
      if (cur_clk - last_frame_clk > 10) {
        g_print("too long to not get new buffer, restart pipeline!\n");
        last_frame_clk = cur_clk;
        gst_element_set_state(pipeline, GST_STATE_READY);
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
      }
    }
  }
  // bus_msg_thread(NULL);

  // for (int i = 0;; ++i) {
  //// process msg in my own main thread instead of calling g_main_loop

  //// g_print("before pull sampe");
  // GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
  // if (sample == NULL) {
  // g_print("gst_app_sink_pull_sample returned null\n");
  ////sleep(5);
  //} else {
  // g_print("gst_app_sink_pull_sample returned\n");
  // last_frame_clk = get_cur_second();
  // GstCaps *caps = gst_sample_get_caps(sample);
  // GstStructure *capsStruct = gst_caps_get_structure(caps, 0);

  // int width, height;
  // gst_structure_get_int(capsStruct, "width", &width);
  // gst_structure_get_int(capsStruct, "height", &height);

  //// Actual compressed image is stored inside GstSample.
  // GstBuffer *buffer = gst_sample_get_buffer(sample);
  // GstMapInfo map;
  // gst_buffer_map(buffer, &map, GST_MAP_READ);

  // cv::Mat img(height, width, CV_8UC1, map.data);
  // cv::imshow("yes", img);
  // cv::waitKey(1);

  // gst_buffer_unmap(buffer, &map);
  // gst_sample_unref(sample);
  //}
  //}

  // Get total conversion time
  int ms = MyGetTickCount() - ticks;

  // Stop pipeline to be released
  // gst_element_set_state(pipeline, GST_STATE_NULL);

  // gst_object_unref(bus);
  // THis will also delete all pipeline elements
  // gst_object_unref(GST_OBJECT(pipeline));

  return 0;
}
