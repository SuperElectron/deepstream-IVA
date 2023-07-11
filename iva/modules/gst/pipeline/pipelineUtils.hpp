#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>

#include "Application.h"
#include "date/tz.h"
#include "logging.hpp"

namespace fs = std::filesystem;

/**
 * @namespace pipelineUtils
 * @brief utilities for Pipeline module
 *
 */
namespace pipelineUtils {

/**
 * @struct BusStruct
 * @brief holds application information for bus_call in pipeline
 * @var loop
 * the loop responsible to run the gstreamer pipeline
 * @var timeout_counter
 * the number of cycles caught where the source was inactive
 * @var timeout_counter_max
 * the maximum number of cycles that can be caught with an inactive source before exiting
 */
struct BusStruct {
  GMainLoop *loop;
  int timeout_counter = 0;
  int timeout_counter_max = 5;
};

/**
 * @brief Translates link_pad() error code to a human readable error
 * @param status_code a numbered error code that describes the link
 * @return  std::string a human readable string that describes the error code
 */
inline std::string get_link_status(int status_code)
{
  std::string ret_str = "ERROR: get_link_status() doesn't recognize status_code";
  switch (status_code) {
    case 0:
      ret_str = "GST_PAD_LINK_OK (0) – link succeeded";
      break;
    case -1:
      ret_str = "GST_PAD_LINK_WRONG_HIERARCHY (-1) – pads have no common grandparent";
      break;
    case -2:
      ret_str = "GST_PAD_LINK_WAS_LINKED (-2) – pad was already linked";
      break;
    case -3:
      ret_str = "GST_PAD_LINK_WRONG_DIRECTION (-3) – pads have wrong direction";
      break;
    case -4:
      ret_str = "GST_PAD_LINK_NOFORMAT (-4) – pads do not have common format";
      break;
    case -5:
      ret_str = "GST_PAD_LINK_NOSCHED (-5) – pads cannot cooperate in scheduling";
      break;
    case -6:
      ret_str = "GST_PAD_LINK_REFUSED (-6) – refused for some reason";
      break;
  }
  return ret_str;
}

/**
 * @brief   manage bus callbacks from the pipeline and output join signal to
 * clients threads on application termination.
 * @reference:
 * https://www.manpagez.com/html/gstreamer-1.0/gstreamer-1.0-1.14.3/GstMessage.php
 *
 * @param   bus         @description application bus from pipeline.
 * @param   msg         @description bus message received from pipeline.
 * @param   data        @description unique data tag for user display.
 * @return  boolean     @description to indicate status of GLib loop (true if
 * running)
 */
inline static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
  const std::string log_prefix = "[bus_call] \n -- ";
  auto bus_store = (BusStruct *)data;
  GMainLoop *loop = (GMainLoop *)bus_store->loop;

  // Get the source object of the message
  GstObject *src = GST_MESSAGE_SRC(msg);

  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS: {
      LOG(WARNING) << log_prefix << "EOS (end of stream) ... terminating gstreamer pipeline";
      g_main_loop_quit(loop);
      break;
    }
    case GST_MESSAGE_ERROR: {
      gchar *debug;
      GError *error;

      gst_message_parse_error(msg, &error, &debug);
      g_free(debug);
      LOG(INFO) << log_prefix << "Error: " << error->message;
      g_error_free(error);
      g_main_loop_quit(loop);
      break;
    }
    case GST_MESSAGE_ELEMENT: {
      const GstStructure *message_structure = gst_message_get_structure(msg);
      const gchar *gst_structure_name = gst_structure_get_name(message_structure);
      VLOG(DEBUG) << log_prefix << "Got message from element (" << GST_MESSAGE_SRC_NAME(msg) << ") message: " << gst_structure_name;
    }
    case GST_MESSAGE_NEW_CLOCK: {
      VLOG(DEBUG) << log_prefix << "Starting pipeline clock, resetting connection timeout";
      bus_store->timeout_counter = 0;
      break;
    }
    default:
      break;
  }
  return true;
}

/**
 * @brief   Parses a GstElement and retrieves its property `name`
 *
 * @param   element     @description GstElement with the name property set
 */
inline void get_element_name(gpointer data, std::string &name)
{
  GstElement *element = (GstElement *)data;
  gchar *temp_name;
  g_object_get(element, "name", &temp_name, NULL);
  name = temp_name;
  g_free(temp_name);
}

/**
 * @brief Creates a GST_DEBUG_DUMP_DOT_DIR ("file.dot") which contains pipeline
 * information.
 * @note                    reference
 * https://api.gtkd.org/gstreamer.c.types.GstDebugGraphDetails.html
 * @dependency              sudo apt-get install -y graphviz;
 * @usage                   dot -Tpng fileName.dot > fileName.png
 *
 * @param pipeline      @description Pipeline for which you want to capture a
 * file.dot
 * @param log_dir       @description Root directory where the log will be
 * located
 * @param state         @description Current state of pipeline (to name the log)
 *
 */
inline void save_debug_dot(GstElement *pipeline, std::string log_dir, std::string state_name)
{
  std::string log_prefix = "[save_debug_dot]\n -- ";

  //  create objects for files and name of file
  std::string log_file_base, log_dot, log_png, pipeline_name;

  get_element_name(pipeline, pipeline_name);
  if (pipeline_name.empty()) {
    LOG(ERROR) << log_prefix << "Pipeline must have a name! pipeline_name=(" << pipeline_name;
    throw -1;
  }

  // set paths and filename
  log_file_base = log_dir + "/gst_debug_dot/" + pipeline_name + "." + state_name;
  log_dot = log_file_base + ".dot";
  log_png = log_file_base + ".png";

  try {
    std::ofstream DotFile;
    DotFile.open(log_dot.c_str());
    auto writable = gst_debug_bin_to_dot_data(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL);
    DotFile << writable;
    DotFile.close();
    std::string cmd = "dot -Tpng " + log_dot + " > " + log_png;
    bool png_created = std::system(cmd.c_str());
    if (png_created) {
      LOG(WARNING) << "Error converting .dot file to png";
    }
    cmd = "rm " + log_dot;
    bool dot_file_removed = std::system(cmd.c_str());
    if (dot_file_removed) {
      LOG(WARNING) << "Error removing .dot file (clean up)";
    }
    fs::permissions(log_png, fs::perms::all);
  }
  catch (const std::exception &e) {
    LOG(ERROR) << log_prefix << "Could not create dot file\n\t -- log_file=(" << log_dot << ")\n\t -- ERROR \n\t" << e.what();
    throw -1;
  }
  LOG(INFO) << log_prefix << "Created GST_DEBUG_DUMP_DOT_DIR file: " << log_png;
}


/**
 * @brief A dynamic callback for a demux elements that links to an element during runtime (a.k.a a dynamic callback).
 *  A pad is created in runtime (and not during compilation) because the demux element can have video, audio, and data
 *   pads; therefore, a dynamic callback must link "a pad" when it is created so its "appropriate element.
 *
 * @copydoc  Expects that the sink element is called 'src_parser' in its pipelineBin
 *
 * @param src_element  the element on which a pad is being added
 * @param src_pad      the pad in the element which was just created
 * @param data         custom data passed into the callback; this is the element we want to link to
 */
inline void on_pad_added(GstElement *src_element, GstPad *src_pad, gpointer data)
{
  // unpack the pointer
  std::string sink_element_name, src_element_name, bin_name;
  GstElement *sink_element;
  GstElement *bin = (GstElement *)data;
  pipelineUtils::get_element_name(bin, bin_name);
  // get the data cap type from the source pad
  GstCaps *caps = gst_pad_get_current_caps(src_pad);
  pipelineUtils::get_element_name(src_element, src_element_name);

  LOG(INFO) << "[on_pad_added] called (bin=" << bin_name << ", src_element=" << src_element_name << ")";

  // checks to ensure the right pad is being linked
  if (g_str_has_prefix(GST_PAD_NAME(src_pad), "video")) {
    LOG(INFO) << "[on_pad_added]\n -- dynamic linking with video prefix for src element: " << src_element_name;
    sink_element = gst_bin_get_by_name(GST_BIN(bin), "src_parser");
  }
  else {
    LOG(WARNING) << "[on_pad_added]\n -- [Ignore] Unexpected pad type=(" << GST_PAD_NAME(src_pad) << ") ** [ignore link] ** ";
    return;
  }

  // get element details and its pad for linking
  pipelineUtils::get_element_name(sink_element, sink_element_name);
  GstPad *sink_pad;
  sink_pad = gst_element_get_static_pad(sink_element, "sink");
  if (gst_pad_is_linked(sink_pad)) {
    LOG(INFO) << "[on_pad_added]\n -- sinkpad of element " << sink_element_name << " is already linked. ** [ignore link] ** ";
    gst_object_unref(sink_pad);
    return;
  }

  LOG(INFO) << "[on_pad_added]\n -- linking elements src(" << src_element_name << ") and sink(" << sink_element_name << ")";

  // attempt to link the pads
  GstPadLinkReturn ret;
  ret = gst_pad_link(src_pad, sink_pad);

  if (GST_PAD_LINK_FAILED(ret)) {
    LOG(ERROR) << "[on_pad_added]\n -- [FAILURE] dynamic link on src(" << src_element_name << ") and sink(" << sink_element_name
               << "). ERROR: " << pipelineUtils::get_link_status(ret) << "\n ** [ignore link] ** ";
    return;
  }

  LOG(INFO) << "[on_pad_added]\n -- Dynamic link successful for elements: " << src_element_name << " and " << sink_element_name;
  gst_object_unref(sink_pad);
}

inline bool doesFileExist(const std::string &filepath)
{
  std::ifstream f(filepath.c_str());
  return f.good();
}

inline GstElement* createMp4SrcBin(std::string binName, std::string filesrcLocation)
{
	// create bin
	GstElement* bin = gst_bin_new(binName.c_str());
	// create elements
	GstElement *filesrc, *qtdemux, *h264parse, *nvv4l2decoder, *queue, *tee;
	filesrc = gst_element_factory_make("filesrc", "source");
	qtdemux = gst_element_factory_make("qtdemux", "src_demux");
	h264parse = gst_element_factory_make("h264parse", "src_parser");
	nvv4l2decoder = gst_element_factory_make("nvv4l2decoder", "src_decoder");
	queue = gst_element_factory_make("queue", "src_queue");

	// set properties
	g_object_set(filesrc, "location", filesrcLocation.c_str(), NULL);
	// set dynamic callback for 'sometimes' pad
	g_signal_connect(qtdemux, "pad-added", G_CALLBACK(pipelineUtils::on_pad_added), (gpointer)bin);

	// add elements to the bin
	gst_bin_add_many(GST_BIN(bin), filesrc, qtdemux, h264parse, nvv4l2decoder, queue, NULL);

	// link elements
	if (!gst_element_link(filesrc, qtdemux))
		LOG(FATAL) << "Failed to link elements in bin=" << binName << ": Elements=(filesrc, qtdemux)";
	if(gst_element_link_many(h264parse, nvv4l2decoder, queue, NULL) != TRUE)
		LOG(FATAL) << "Failed to link many elements in bin=" << binName << ": Elements=(h264parse, nvv4l2decoder, queue)";

	// create ghost pad at output for future linking on tee pad (src_%u)
	int num_pads = 0;
	// Add ghost pads to access src in bin
	std::string ghostPadName = (std::string) "output" + std::to_string(num_pads);
	std::string padName = (std::string) "src";
	GstPad* binPad0 = gst_element_get_static_pad(queue, padName.c_str());
	if(!gst_element_add_pad(bin, gst_ghost_pad_new(ghostPadName.c_str(), binPad0)))
		LOG(FATAL) << "Could not add the ghostPad to the bin=" << binName << ", pad=" << ghostPadName;
	gst_pad_set_active (GST_PAD_CAST (binPad0), 1);
	gst_object_unref(GST_OBJECT(binPad0));
	LOG(INFO) << "Added ghost pad to bin=" << binName << " with pad=" << ghostPadName;
	return bin;
}

inline GstElement* createV4l2SrcBin(std::string binName, std::string deviceId)
{
	// create bin
	GstElement* bin = gst_bin_new(binName.c_str());
	// create elements
	GstElement *source, *source_queue, *src_rate, *src_scale, *src_conv, *src_caps, *src_decoder, *src_queue, *tee;
	source = gst_element_factory_make("v4l2src", "source");
	g_object_set(G_OBJECT(source),"device", deviceId.c_str(), NULL);
	source_queue = gst_element_factory_make("queue", "source_queue");
	src_rate = gst_element_factory_make("videorate", "src_rate");
	src_scale = gst_element_factory_make("videoscale", "src_scale");
	src_conv = gst_element_factory_make("videoconvert", "src_conv");
	src_caps = gst_element_factory_make("capsfilter", "src_caps");
	g_object_set(G_OBJECT(src_caps),"caps", gst_caps_from_string("video/x-raw,framerate=(fraction)12/1,width=(int)1920,height=(int)1080"), NULL);
	src_decoder = gst_element_factory_make("nvvideoconvert", "src_decoder");
	src_queue = gst_element_factory_make("queue", "src_queue");

	// add elements to the bin
	gst_bin_add_many(GST_BIN(bin), source, source_queue, src_rate, src_scale, src_conv, src_caps, src_decoder, src_queue, NULL);

	// link elements
	if(gst_element_link_many(source, source_queue, src_rate, src_scale, src_conv, src_caps, src_decoder, src_queue, NULL) != TRUE)
		LOG(FATAL) << "Failed to link many elements in bin=" << binName << ": Elements=(v4l2src, queue, videorate, videoscale, videoconvert, capsfilter, nvvideoconvert, queue)";

	// create ghost pad at output for future linking on tee pad (src_%u)
	int num_pads = 0;
	// Add ghost pads to access src_0 in bin
	std::string ghostPadName = (std::string) "output" + std::to_string(num_pads);
	std::string padName = (std::string) "src_" + std::to_string(num_pads);
	GstPad* binPad0 = gst_element_get_static_pad(src_queue, padName.c_str());
	if(!gst_element_add_pad(bin, gst_ghost_pad_new(ghostPadName.c_str(), binPad0)))
		LOG(FATAL) << "Could not add the ghostPad to the bin=" << binName << ", pad=" << ghostPadName;
	gst_pad_set_active (GST_PAD_CAST (binPad0), 1);
	gst_object_unref(GST_OBJECT(binPad0));
	LOG(INFO) << "Added ghost pad to bin=" << binName << " with pad=" << ghostPadName;
	return bin;
}

inline GstElement* createInferenceBinToFakeSink(std::string binName, std::string inputPadName, int num_src)
{
  // create bin
  GstElement* bin = gst_bin_new(binName.c_str());
  // create elements
  GstElement *nvstreammux, *nvinfer, *nvtracker, *queue, *fakesink;
  nvstreammux = gst_element_factory_make("nvstreammux", "nv_mux");
  g_object_set(nvstreammux,
               "nvbuf-memory-type", 3,
               "batch-size", 1,
               "width", 1280,
               "height", 720,
               "batched-push-timeout", 40000,
               "sync-inputs", false,
               "live-source", false,
               NULL);
  nvinfer = gst_element_factory_make("nvinfer", "nv_detection");
  g_object_set(nvinfer,
               "config-file-path","/src/configs/model/face_detection/detection_face.yml",
               "batch-size", 1,
               "qos", 1,
               NULL);

  nvtracker = gst_element_factory_make("nvtracker", "nv_tracker");
  g_object_set(nvtracker,
               "ll-config-file", "/src/configs/model/tracker.yml",
               "ll-lib-file", "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so",
               "enable-batch-process", 1,
               "tracker-width", 640,
               "tracker-height", 480,
               NULL);
  queue = gst_element_factory_make("queue", "nv_queue");
  fakesink = gst_element_factory_make("fakesink", "sink");
  g_object_set(fakesink, "sync", false, NULL);

  // add elements to the bin
  gst_bin_add_many(GST_BIN(bin), nvstreammux, nvinfer, nvtracker, queue, fakesink, NULL);
  if(!gst_element_link_many(nvstreammux, nvinfer, nvtracker, queue, fakesink, NULL))
    LOG(FATAL) << "Failed to add elements to bin=" << binName;

  // create ghost pad at output for future linking
  for (int i = 0; i < num_src; i++) {
    std::string padName = (std::string) "sink_" + std::to_string(i);
    GstPad *setSrcBinPad = gst_element_get_request_pad(nvstreammux, padName.c_str());
    // Check if the pad was created.
    if (setSrcBinPad == NULL)
      LOG(FATAL) << "Could not get the tee request pad=" << padName;

    std::string ghostPadName = (std::string) "input" + std::to_string(i);
    GstPad *ghostPad = gst_ghost_pad_new(ghostPadName.c_str(), setSrcBinPad);
    if (ghostPad == NULL)
      LOG(FATAL) << "Could not create the ghostPad for bin=" << binName << ", ghostPadName=" << ghostPadName;

    if (!gst_element_add_pad(bin, ghostPad))
      LOG(FATAL) << "Could not add the ghostPad to bin=" << binName << ", ghostPadName=" << ghostPadName;
    gst_object_unref(GST_OBJECT(setSrcBinPad));
    gst_pad_set_active (GST_PAD_CAST (ghostPad), 1);
    LOG(INFO) << "Added ghost pad to bin=" << binName << " with pad=" << ghostPadName;
  }
  return bin;
}

inline GstElement* createInferenceBinToVideoDisplay(std::string binName, std::string inputPadName, int num_src)
{
  // create bin
  GstElement* bin = gst_bin_new(binName.c_str());
  // create elements
  GstElement *nvstreammux, *nvinfer, *nvtracker, *nvconvert, *queue, *sink;
  nvstreammux = gst_element_factory_make("nvstreammux", "nv_mux");
  g_object_set(nvstreammux,
               "nvbuf-memory-type", 3,
               "batch-size", 1,
               "width", 1280,
               "height", 720,
               "batched-push-timeout", 40000,
               "sync-inputs", false,
               "live-source", false,
               NULL);
  nvinfer = gst_element_factory_make("nvinfer", "nv_detection");
  g_object_set(nvinfer,
               "config-file-path","/src/configs/model/face_detection/detection_face.yml",
               "batch-size", 1,
               "qos", 1,
               NULL);

  nvtracker = gst_element_factory_make("nvtracker", "nv_tracker");
  g_object_set(nvtracker,
               "ll-config-file", "/src/configs/model/tracker.yml",
               "ll-lib-file", "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so",
               "enable-batch-process", 1,
               "tracker-width", 640,
               "tracker-height", 480,
               NULL);
  nvconvert = gst_element_factory_make("nvvideoconvert", "nv_convert");
  queue = gst_element_factory_make("queue", "nv_queue");
  sink = gst_element_factory_make("xvimagesink", "sink");
  g_object_set(sink, "sync", true, NULL);

  // add elements to the bin
  gst_bin_add_many(GST_BIN(bin), nvstreammux, nvinfer, nvtracker, nvconvert, queue, sink, NULL);
  if(!gst_element_link_many(nvstreammux, nvinfer, nvtracker, nvconvert, queue, sink, NULL))
    LOG(FATAL) << "Failed to add elements to bin=" << binName;

  // create ghost pad at output for future linking
  for (int i = 0; i < num_src; i++) {
    std::string padName = (std::string) "sink_" + std::to_string(i);
    GstPad *setSrcBinPad = gst_element_get_request_pad(nvstreammux, padName.c_str());
    // Check if the pad was created.
    if (setSrcBinPad == NULL)
      LOG(FATAL) << "Could not get the nvstreammux request pad=" << padName;

    std::string ghostPadName = (std::string) "input" + std::to_string(i);
    GstPad *ghostPad = gst_ghost_pad_new(ghostPadName.c_str(), setSrcBinPad);
    if (ghostPad == NULL)
      LOG(FATAL) << "Could not create the ghostPad for bin=" << binName << ", ghostPadName=" << ghostPadName;

    if (!gst_element_add_pad(bin, ghostPad))
      LOG(FATAL) << "Could not add the ghostPad to bin=" << binName << ", ghostPadName=" << ghostPadName;
    gst_object_unref(GST_OBJECT(setSrcBinPad));
    gst_pad_set_active (GST_PAD_CAST (ghostPad), 1);
    LOG(INFO) << "Added ghost pad to bin=" << binName << " with pad=" << ghostPadName;
  }
  return bin;
}

}  // namespace pipelineUtils