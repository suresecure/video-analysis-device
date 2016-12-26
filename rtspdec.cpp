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

// Command line parameters
static char *infile = NULL;
static int width = 0;
static int height = 0;
static char *outfile = NULL;
static int quality = 60;

// Buffer for incoming data (FullHD is max, you can increase it as you need)
static char rawdata[1920 * 1080];

// Flags to indicate to parent thread that GstreamerThread started and finished
static volatile bool bGstreamerThreadStarted = false;
static volatile bool bGstreamerThreadFinished = false;

// static GstElement *appsrc;
static GstElement *filesrc;
static GstElement *appsink;
static GstElement *uridecodebin;
// static GstElement *pipeline, *vidconv, *x264enc, *qtmux;
static GstElement *pipeline, *qtdemux, *h264parse, *h264dec;
static GstElement *avdec_h264;

struct FrameBuffer {
  char *data;
  int width;
  int height;
};

unsigned int MyGetTickCount() {
  struct timeval tim;
  gettimeofday(&tim, NULL);
  unsigned int t = ((tim.tv_sec * 1000) + (tim.tv_usec / 1000)) & 0xffffffff;
  return t;
}

// Bus messages processing, similar to all gstreamer examples
gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
  g_print("bus call: ");
  GMainLoop *loop = (GMainLoop *)data;

  switch (GST_MESSAGE_TYPE(msg)) {

  case GST_MESSAGE_EOS:
    fprintf(stderr, "End of stream\n");
    g_main_loop_quit(loop);
    break;

  case GST_MESSAGE_ERROR:
    gchar *debug;
    GError *error;

    gst_message_parse_error(msg, &error, &debug);
    g_free(debug);

    g_printerr("Error: %s\n", error->message);
    g_error_free(error);

    g_main_loop_quit(loop);

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
void *GstreamerThread(void *pThreadParam) {
  GMainLoop *loop;
  GstBus *bus;
  guint bus_watch_id;

  char *strPipeline = new char[8192];

  // pipeline = gst_pipeline_new("mypipeline");
  // pipeline = gst_parse_launch(
  //"uridecodebin uri=rtsp://localhost:8554/live ! appsink name=mysink",
  // NULL);
  // pipeline = gst_parse_launch ("filesrc location=/home/mythxcq/test.mp4 !
  // qtdemux ! avdec_h264 ! appsink name=mysink", NULL);
  // pipeline = gst_parse_launch("rtspsrc location=rtsp://localhost:8554/live !
  // "
  //"rtph264depay ! h264parse ! avdec_h264 ! video/x-raw, format=RGB ! appsink "
  //"name=mysink",
  // NULL);
  pipeline = gst_parse_launch(
      "filesrc name=myfilesrc location=/home/mythxcq/test.mp4 ! "
      "qtdemux name=myqtdemux ! avdec_h264 name=myavdec_h264 ! "
      " appsink name=mysink",
      NULL);
  // gst_parse_launch("filesrc location=/home/mythxcq/test.mp4 ! qtdemux !
  // h264parse ! avdec_h264 ! "
  //"video/x-raw,format=RGB ! appsink name=mysink",
  // NULL);
  appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
  // name=demux demux.video_00 ! ffdec_h264 ! xvimagesink");
  // appsrc = gst_element_factory_make("appsrc", "mysource");
  // uridecodebin = gst_element_factory_make("uridecodebin", "myuridecodebin");
  // filesrc = gst_element_factory_make("filesrc", "myfilesrc");
  // appsink = gst_element_factory_make("appsink", "mysink");

  // qtdemux = gst_element_factory_make("qtdemux", "myqtdemux");
  // h264parse = gst_element_factory_make("h264parse", "myh264parse");
  // h264dec = gst_element_factory_make("avdec_h264", "myavdec_h264");

  // vidconv = gst_element_factory_make("videoconvert", "myvideoconvert");
  // x264enc = gst_element_factory_make("x264enc", "myx264enc");
  // qtmux = gst_element_factory_make("qtmux", "myqtmux");

  // Check if all elements were created
  // if (!pipeline || !uridecodebin || !filesrc || !qtdemux || !h264parse ||
  // !h264dec || !appsink) {
  // fprintf(stderr, "Could not gst_element_factory_make, terminating\n");
  // bGstreamerThreadStarted = bGstreamerThreadFinished = true;
  // return (void *)0xDEAD;
  //}

  // appsrc should be linked to jpegenc with these caps otherwise jpegenc does
  // not know size of incoming buffer
  // GstCaps *cap_appsrc_to_x264enc;
  // cap_appsrc_to_x264enc =
  // gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "GRAY8",
  //"width", G_TYPE_INT, width, "height", G_TYPE_INT,
  // height, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);

  // GstCaps *cap_x264enc_to_sink;
  // cap_x264enc_to_sink = gst_caps_new_simple(
  //"video/x-h264", "stream-format", G_TYPE_STRING, "byte-stream",
  //"alignment", G_TYPE_STRING, "au", NULL);

  // GstCaps *cap_h264dec_to_sink;
  // cap_h264dec_to_sink = gst_caps_new_simple(
  //"

  // blocksize is important for jpegenc to know how many data to expect from
  // appsrc in a single frame, too
  char szTemp[64];
  sprintf(szTemp, "%d", width * height);
  // g_object_set(G_OBJECT(filesrc), "location", "/home/mythxcq/test.mp4",
  // NULL);
  // g_object_set(G_OBJECT(uridecodebin), "uri",
  // "file:///home/mythxcq/test.mp4", NULL);
  // g_object_set(G_OBJECT(appsrc), "blocksize", szTemp, NULL);
  // g_object_set(G_OBJECT(x264enc), "sync-lookahead", 2, NULL);
  // g_object_set(G_OBJECT(x264enc), "rc-lookahead", 2, NULL);
  // g_object_set(G_OBJECT(x264enc), "bframes", 0, NULL);
  // g_object_set(G_OBJECT(appsrc), "stream-type", 0, "format", GST_FORMAT_TIME,
  // NULL);
  // g_object_set(G_OBJECT(appsrc), "caps",
  // gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING,
  //"GRAY8", "width", G_TYPE_INT, width,
  //"height", G_TYPE_INT, height, "framerate",
  // GST_TYPE_FRACTION, 30, 1, NULL),
  // NULL);

  // Create gstreamer loop
  loop = g_main_loop_new(NULL, FALSE);

  // add a message handler
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);

  // gst_bin_add_many(GST_BIN(pipeline), filesrc, vidconv, x264enc, appsink,
  // NULL);
  // gst_bin_add_many(GST_BIN(pipeline), filesrc, qtdemux, h264parse, h264dec,
  // appsink, NULL);
  // gst_bin_add_many(GST_BIN(pipeline), uridecodebin, appsink, NULL);

  // gst_element_link(uridecodebin, appsink);
  // gst_element_link(filesrc, qtdemux);
  // gst_element_link(qtdemux, appsink);
  // gst_element_link(qtdemux, h264parse);
  // gst_element_link(h264parse, h264dec);
  // gst_element_link(h264dec, appsink);

  // gst_element_link_filtered(appsrc, vidconv, cap_appsrc_to_x264enc);
  // gst_element_link(vidconv, x264enc);
  // gst_element_link_filtered(x264enc, appsink, cap_x264enc_to_sink);

  fprintf(stderr, "Setting g_main_loop_run to GST_STATE_PLAYING\n");
  // Start pipeline so it could process incoming data
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  // Indicate that thread was started
  bGstreamerThreadStarted = true;

  int x;
  scanf("%d", &x);
  // Loop will run until receiving EOS (end-of-stream), will block here
  g_main_loop_run(loop);

  fprintf(stderr, "g_main_loop_run returned, stopping playback\n");

  // Stop pipeline to be released
  gst_element_set_state(pipeline, GST_STATE_NULL);

  fprintf(stderr, "Deleting pipeline\n");
  // THis will also delete all pipeline elements
  gst_object_unref(GST_OBJECT(pipeline));

  g_source_remove(bus_watch_id);
  g_main_loop_unref(loop);

  // Indicate that thread was finished
  bGstreamerThreadFinished = true;

  return NULL;
}

// Starts GstreamerThread that remains in memory and compresses frames as being
// fed by user app.
bool StartGstreamer() {
  // GstreamerThread(NULL);
  // return true;
  unsigned long GtkThreadId;
  pthread_attr_t GtkAttr;

  // Start thread
  int result = pthread_attr_init(&GtkAttr);
  if (result != 0) {
    fprintf(stderr, "pthread_attr_init returned error %d\n", result);
    return false;
  }

  void *pParam = NULL;
  result = pthread_create(&GtkThreadId, &GtkAttr, GstreamerThread, pParam);
  if (result != 0) {
    fprintf(stderr, "pthread_create returned error %d\n", result);
    return false;
  }

  return true;
}

// Puts raw data for encoding into gstreamer. Must put exactly width*height
// bytes.
// void PushBuffer(int idx) {
// GstFlowReturn ret;
// GstBuffer *buffer;

// int size = width * height;
// buffer = gst_buffer_new_allocate(NULL, size, NULL);

// GstMapInfo info;
// gst_buffer_map(buffer, &info, GST_MAP_WRITE);
// unsigned char *buf = info.data;
// memmove(buf, rawdata, size);
// gst_buffer_unmap(buffer, &info);

// GST_BUFFER_DTS(buffer) = GST_BUFFER_PTS(buffer) = idx * 33333333;

// ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
//}
//
static void cb_new_pad(GstElement *element, GstPad *pad, gpointer data) {
  gchar *name;

  name = gst_pad_get_name(pad);
  if (strcmp(name, "video_0") == 0 &&
      !gst_element_link_pads(qtdemux, name, avdec_h264, "sink")) {
    printf("link demux-decoder fail\n");
  }
  g_free(name);
}

// Reads compressed jpeg frame. Will block if there is nothing to read out.
char *PullBuffer(int *outlen) {
  // Will block until sample is ready. In our case "sample" is encoded picture.
  GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));

  GstCaps *caps = gst_sample_get_caps(sample);
  // QString strCaps = gst_caps_to_string(caps);
  // qDebug() << "strCAPS=" << strCaps;
  GstStructure *capsStruct = gst_caps_get_structure(caps, 0);

  // g_print("Frame info: %s\n", gst_structure_to_string(capsStruct));
  int width, height;
  gst_structure_get_int(capsStruct, "width", &width);
  gst_structure_get_int(capsStruct, "height", &height);
  // printf("Frame width %d, height %d\n", width, height);

  if (sample == NULL) {
    fprintf(stderr, "gst_app_sink_pull_sample returned null\n");
    return NULL;
  }

  // Actual compressed image is stored inside GstSample.
  GstBuffer *buffer = gst_sample_get_buffer(sample);
  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_READ);

  cv::Mat img(height, width, CV_8UC1, map.data);
  cv::imshow("yes", img);
  cv::waitKey(1);

  // Allocate appropriate buffer to store compressed image
  // char *pRet = new char[map.size];
  // Copy image
  // memmove(pRet, map.data, map.size);

  gst_buffer_unmap(buffer, &map);
  gst_sample_unref(sample);

  // Inform caller of image size
  //*outlen = map.size;

  return NULL;
}

int main(int argc, char *argv[]) {
  gst_init(NULL, NULL); // Will abort if GStreamer init error found
  GstBus *bus;
  // pipeline = gst_parse_launch(
  //"filesrc name=myfilesrc location=/home/mythxcq/test_cif.mp4 ! "
  //"qtdemux name=myqtdemux ! avdec_h264 name=myavdec_h264 ! appsink
  //name=mysink",
  // NULL);
  //pipeline = gst_parse_launch(
      //"filesrc name=myfilesrc location=/home/mythxcq/test_cif.mp4 ! "
      //"qtdemux name=myqtdemux ! avdec_h264 ! videoconvert ! ximagesink ",
      //NULL);
  // pipeline = gst_parse_launch("rtspsrc location=rtsp://localhost:8554/live !
  // "
  //"rtph264depay ! avdec_h264 ! appsink max-buffers=2 drop=true name=mysink",
  // NULL);
  // pipeline = gst_parse_launch("rtspsrc location=rtsp://localhost:8554/live !
  // "
  //"rtph264depay ! avdec_h264 ! appsink max-buffers=2 drop=true name=mysink",
  // NULL);
  // pipeline = gst_parse_launch("filesrc location=/home/mythxcq/test.mp4 ! "
  //"qtdemux ! avdec_h264 ! videoconvert ! "
  //"video/x-raw,format=BGR ! appsink name=mysink",
  // NULL);
  // appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
  // filesrc = gst_bin_get_by_name(GST_BIN(pipeline), "myfilesrc");
  // qtdemux = gst_bin_get_by_name(GST_BIN(pipeline), "myqtdemux");
  // avdec_h264 = gst_bin_get_by_name(GST_BIN(pipeline), "myavdec_h264");
  pipeline = gst_pipeline_new("mypipeline");

  filesrc = gst_element_factory_make("filesrc", "myfilesrc");
  appsink = gst_element_factory_make("appsink", "myappsink");
  qtdemux = gst_element_factory_make("qtdemux", "myqtdemux");
  avdec_h264 = gst_element_factory_make("avdec_h264", "myavdec_h264");

  gst_element_link(filesrc, qtdemux);
  gst_element_link(avdec_h264, appsink);

  gst_bin_add_many(GST_BIN(pipeline), filesrc, qtdemux, avdec_h264, appsink,
                   NULL);

  // add a message handler
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  // bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
  // gst_object_unref(bus);
  g_signal_connect(qtdemux, "pad-added", G_CALLBACK(cb_new_pad), NULL);

  fprintf(stderr, "Setting g_main_loop_run to GST_STATE_PLAYING\n");
  // Start pipeline so it could process incoming data
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  // Stop pipeline to be released
  // gst_element_set_state(pipeline, GST_STATE_NULL);

  // THis will also delete all pipeline elements
  // gst_object_unref(GST_OBJECT(pipeline));

  /* Check input arguments */
  // if (argc < 4 || argc > 5) {
  // fprintf(stderr, "Usage: %s rawfile width height outfile [quality]\n"
  //"Rawfile must be one raw frame of GRAY8, outfile will be "
  //"JPG encoded. 10 outfiles will be created\n",
  // argv[0]);
  // return -1;
  //}
  /* Initialization */

  // Read command line arguments
  // width = atoi(argv[1]);
  // height = atoi(argv[2]);
  // outfile = argv[3];

  // Validate command line arguments
  // if (width < 100 || width > 4096 || height < 100 || height > 4096) {
  // fprintf(stderr,
  //"width and/or height or quality is bad, not running conversion\n");
  // return -1;
  //}

  // Init raw frame
  // for (int i = 0; i < 1920 * 1080; ++i) {
  // rawdata[i] = i % 255;
  //}

  // Start conversion thread
  // StartGstreamer();

  //// Ensure thread is running (or ran and stopped)
  // while (bGstreamerThreadStarted == false)
  // usleep(10000); // Yield to allow thread to start
  // if (bGstreamerThreadFinished == true) {
  // fprintf(stderr, "Gstreamer thread could not start, terminating\n");
  // return -1;
  //}

  int ticks = MyGetTickCount();

  // int prefilled = 12;
  // for (int i = 0; i < prefilled; ++i) {
  // PushBuffer(i);
  //}
  // FILE *of = fopen("out.h264", "w");
  // Compress raw frame 10 times, adding horizontal stripes to ensure resulting
  // images are different
  // for (int i = 0; i < 10; i++) {
  for (int i = 0;; ++i) {
    // write stripes into image to see they are really different
    // memset(rawdata + (i * 20) * width, 0xff, width);
    // memset(rawdata + ((i + 3) * 20) * width, 0x00, width);

    // Push raw buffer into gstreamer
    // PushBuffer(i + prefilled);

    // Pull compressed buffer from gstreamer
    // g_print("\n\n\n");
    // GstMessage *msg;
    // while ((msg = gst_bus_pop(bus))) {
    //// Call your bus message handler
    // bus_call(bus, msg, NULL);
    // gst_message_unref(msg);
    //}

    if (appsink != NULL) {
      int len;
      char *buf = PullBuffer(&len);
    } else
      usleep(30000);

    if (i == 10) {
      //gst_element_set_state(pipeline, GST_STATE_READY);
      // gst_element_set_state(pipeline, GST_STATE_PAUSED);
      // GstState state = GST_STATE_PLAYING;
      // while (state != GST_STATE_READY) {
      // g_print("Get state!\n");
      // gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
      // g_print("State got: %d\n", state);
      //}
      // g_print("after state changed!\n");
      // g_object_set(G_OBJECT(filesrc), "location",
      //"/home/mythxcq/test_d1.mp4",
      // NULL);
      // g_print("READY!\n");
      // gst_element_link(qtdemux, avdec_h264);
      //gst_element_set_state(pipeline, GST_STATE_PLAYING);
      // while (state != GST_STATE_PLAYING) {
      // g_print("Get state!\n");
      // gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
      // g_print("State got: %d\n", state);
      //}
    }
    // fwrite(buf, 1, len, of);
    // if (NULL == buf)
    // break;
    // else {
    //// printf("get frame %d!\n", i);
    // delete[] buf;
    //}
  }

  // Get total conversion time
  int ms = MyGetTickCount() - ticks;

  return 0;
}