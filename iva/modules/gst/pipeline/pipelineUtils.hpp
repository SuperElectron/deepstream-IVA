#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <unordered_set>

//#include "Application.h"
#include "date/tz.h"
#include "logging.hpp"

// Declare the global variable from argv[1] in main.cpp
extern std::string BASE_DIR;

namespace fs = std::filesystem;
using njson = nlohmann::json;

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
      LOG(WARNING) << log_prefix << "EOS (end of stream) ... terminating pipeline";
      g_main_loop_quit(loop);
      break;
    }
    case GST_MESSAGE_ERROR: {
      gchar *debug;
      GError *error;

      gst_message_parse_error(msg, &error, &debug);
      g_free(debug);

      std::string errMsg = (std::string) error->message;

      if (errMsg.find("Could not open resource for reading and writing") != std::string::npos
          ||
          errMsg.find("Resource not found") != std::string::npos
          )
      {
        LOG(ERROR) << log_prefix << "Error: " << error->message;
        LOG(WARNING) << "\n\t*************************************************************************"
                        "\n\tCheck that pipeline['sources'] and pipeline['sinks'] in config.json exist"
                        "\n\t*************************************************************************\n\t"
                        "file:     $ ls /path/to/video.mp4 \n\t"
                        "rtsp:     $ gst-discoverer-1.0 rtsp://IP:PORT/stream \n\t"
                        "rtmp:     $ gst-discoverer-1.0 rtmp://IP:PORT/stream \n\t";
      }
//      else if (errMsg.find("Could not open device") != std::string::npos)
//      {
//        LOG(ERROR) << log_prefix << "Error: " << error->message;
//        LOG(WARNING) << "\n\t*************************************************************************"
//                        "\n\tCheck that pipeline['sources'] config.json exist"
//                        "\n\t*************************************************************************\n\t"
//                        "v4l2src:  $ v4l2-ctl --all \n\t";
//      }
      else
      {
        LOG(ERROR) << log_prefix << "Error: " << error->message;
      }
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
  if (pipeline_name.empty())
    LOG(FATAL) << log_prefix << "Pipeline must have a name! pipeline_name=(" << pipeline_name;

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
    LOG(FATAL) << log_prefix << "Could not create dot file\n\t -- log_file=(" << log_dot << ")\n\t -- ERROR \n\t" << e.what();
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

  // checks to ensure the right pad is being linked
  if (g_str_has_prefix(GST_PAD_NAME(src_pad), "video")) {
    LOG(INFO) << "[on_pad_added]\n -- dynamic linking (video connected) (bin=" << bin_name << ", src_element=" << src_element_name << ")";
    sink_element = gst_bin_get_by_name(GST_BIN(bin), "src_parser");
  } else if (g_str_has_prefix(GST_PAD_NAME(src_pad), "recv_rtp_src"))
  {
    LOG(INFO) << "[on_pad_added]\n -- dynamic linking (rtsp connected) (bin=" << bin_name << ", src_element=" << src_element_name << ")";
    sink_element = gst_bin_get_by_name(GST_BIN(bin), "source_depay");
  } else {
    LOG(INFO) << "[on_pad_added]\n -- [Ignore] Unexpected pad type=(" << GST_PAD_NAME(src_pad)
              << ") called by bin=(" << bin_name << ", src_element=" << src_element_name << ") ** [ignore link] ** ";
    return;
  }

  // get element details and its pad for linking
  pipelineUtils::get_element_name(sink_element, sink_element_name);
  GstPad *sink_pad;
  sink_pad = gst_element_get_static_pad(sink_element, "sink");
  if (gst_pad_is_linked(sink_pad)) {
    LOG(WARNING) << "[on_pad_added]\n -- sinkpad already linked. called by bin=(" << bin_name << ", src_element=" << src_element_name << ")** [ignore link] ** ";
    gst_object_unref(sink_pad);
    return;
  }

  // attempt to link the pads
  GstPadLinkReturn ret;
  ret = gst_pad_link(src_pad, sink_pad);
  if (GST_PAD_LINK_FAILED(ret))
    LOG(FATAL) << "[on_pad_added]\n -- [FAILURE] dynamic link for bin=(" << bin_name << " on src=(" << src_element_name << ") and sink(" << sink_element_name
               << "). ERROR: " << pipelineUtils::get_link_status(ret) << "\n ** [ignore link] ** ";

  LOG(INFO) << "[on_pad_added]\n -- [success] Dynamic link called by bin=("
            << bin_name << ") on elements=(src:" << src_element_name << ", sink:" << sink_element_name << ")";
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
	GstElement *filesrc, *qtdemux, *h264parse, *nvv4l2decoder, *queue;
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

	// Add ghost pads to access src in bin for future linking
	std::string ghostPadName = "output0";
	std::string padName = "src";
	GstPad* binPad0 = gst_element_get_static_pad(queue, padName.c_str());
	if(!gst_element_add_pad(bin, gst_ghost_pad_new(ghostPadName.c_str(), binPad0)))
		LOG(FATAL) << "Could not add the ghostPad to the bin=" << binName << ", pad=" << ghostPadName;
	gst_pad_set_active (GST_PAD_CAST (binPad0), 1);
	gst_object_unref(GST_OBJECT(binPad0));
	VLOG(DEBUG) << "Added ghost pad to bin=" << binName << " with pad=" << ghostPadName;
	return bin;
}

inline GstElement* createV4l2SrcBin(std::string binName, std::string deviceId)
{
	// create bin
	GstElement* bin = gst_bin_new(binName.c_str());
	// create elements
	GstElement *source, *source_queue, *src_rate, *src_scale, *src_conv, *src_caps, *src_decoder, *src_queue;
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

	// create ghost pad at output for future linking on queue pad (src)
	int num_pads = 0;
	// Add ghost pads to access src_0 in bin
	std::string ghostPadName = (std::string) "output" + std::to_string(num_pads);
	std::string padName = (std::string) "src";
	GstPad* binPad0 = gst_element_get_static_pad(src_queue, padName.c_str());
	if(!gst_element_add_pad(bin, gst_ghost_pad_new(ghostPadName.c_str(), binPad0)))
		LOG(FATAL) << "Could not add the ghostPad to the bin=" << binName << ", pad=" << ghostPadName;
	gst_pad_set_active (GST_PAD_CAST (binPad0), 1);
	gst_object_unref(GST_OBJECT(binPad0));
	VLOG(DEBUG) << "Added ghost pad to bin=" << binName << " with pad=" << ghostPadName;
	return bin;
}

inline GstElement* createRtspSrcBin(std::string binName, std::string rtsp_url)
{
    // create bin
    GstElement* bin = gst_bin_new(binName.c_str());
    // create elements
    GstElement *source, *source_depay, *src_parse, *src_decoder, *src_queue;
    source = gst_element_factory_make("rtspsrc", "source");
    g_object_set(G_OBJECT(source),"location", rtsp_url.c_str(), NULL);
    g_signal_connect(source, "pad-added", G_CALLBACK(pipelineUtils::on_pad_added), (gpointer)bin);

    source_depay = gst_element_factory_make("rtph264depay", "source_depay");
    src_parse = gst_element_factory_make("h264parse", "src_parse");
    src_decoder = gst_element_factory_make("nvv4l2decoder", "src_decoder");
    src_queue = gst_element_factory_make("queue", "src_queue");

    // add elements to the bin
    gst_bin_add_many(GST_BIN(bin), source, source_depay, src_parse, src_decoder, src_queue, NULL);

    // link elements
//    if(gst_element_link_many(source_depay, src_parse, src_decoder, src_queue, NULL) != TRUE)
//        LOG(FATAL) << "Failed to link many elements in bin=" << binName << ": Elements=(source_depay, src_parse, src_decoder, src_queue)";
    if (!gst_element_link(source_depay, src_parse)) {
        LOG(FATAL) << "Failed to link many elements in bin=" << binName << ": Elements=(source_depay, src_parse)";
    }
    if (!gst_element_link(src_parse, src_decoder)) {
        LOG(FATAL) << "Failed to link many elements in bin=" << binName << ": Elements=(src_parse, src_decoder)";
    }
    if (!gst_element_link(src_decoder, src_queue)) {
        LOG(FATAL) << "Failed to link many elements in bin=" << binName << ": Elements=(src_decoder, src_queue)";
    }

    // create ghost pad at output for future linking on queue pad (src)
    int num_pads = 0;
    // Add ghost pads to access src_0 in bin
    std::string ghostPadName = (std::string) "output" + std::to_string(num_pads);
    std::string padName = (std::string) "src";
    GstPad* binPad0 = gst_element_get_static_pad(src_queue, padName.c_str());
    if(!gst_element_add_pad(bin, gst_ghost_pad_new(ghostPadName.c_str(), binPad0)))
        LOG(FATAL) << "Could not add the ghostPad to the bin=" << binName << ", pad=" << ghostPadName;
    gst_pad_set_active (GST_PAD_CAST (binPad0), 1);
    gst_object_unref(GST_OBJECT(binPad0));
    VLOG(DEBUG) << "Added ghost pad to bin=" << binName << " with pad=" << ghostPadName;
    return bin;
}

inline GstElement* createInferenceBinToStreamDemux(std::string binName, int num_src, int width, int height, bool live_source)
{
  std::string detection_file = BASE_DIR + "/model/detection.yml";
  std::string tracker_file = BASE_DIR + "/model/tracker.yml";

  // create bin
  GstElement* bin = gst_bin_new(binName.c_str());
  // create elements
  GstElement *nv_mux, *nv_infer, *nv_tracker, *nv_convert, *nv_demux;
  nv_mux = gst_element_factory_make("nvstreammux", "nv_mux");

  // set buffer memory type, and if on JETSON, then set to 0!
  int mem_type = 3;
#ifdef PLATFORM_TEGRA
  mem_type = 0;
#endif

  g_object_set(nv_mux,
               "nvbuf-memory-type", mem_type,
               "batch-size", num_src,
               "width", width,
               "height", height,
               "batched-push-timeout", 40000,
               "sync-inputs", false,
               "live-source", live_source,
               NULL);
  nv_infer = gst_element_factory_make("nvinfer", "nv_detection");
  g_object_set(nv_infer,
               "config-file-path",detection_file.c_str(),
//               "batch-size", 1,
               "qos", 1,
               NULL);

  nv_tracker = gst_element_factory_make("nvtracker", "nv_tracker");
  g_object_set(nv_tracker,
               "ll-config-file", tracker_file.c_str(),
               "ll-lib-file", "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so",
               "enable-batch-process", 1,
               "tracker-width", 640,
               "tracker-height", 480,
               NULL);
  nv_convert = gst_element_factory_make("nvvideoconvert", "nv_convert");
  nv_demux = gst_element_factory_make("nvstreamdemux", "nv_demux");

  // add elements to the bin
  gst_bin_add_many(GST_BIN(bin), nv_mux, nv_infer, nv_tracker, nv_convert, nv_demux, NULL);
  if(!gst_element_link_many(nv_mux, nv_infer, nv_tracker, nv_convert, nv_demux, NULL))
    LOG(FATAL) << "Failed to add elements to bin=" << binName;

  // create ghost pad at output for future linking
  for (int i = 0; i < num_src; i++) {
    // create ghost pad for each input pad (sink)
    std::string inputPadName = (std::string) "sink_" + std::to_string(i);
    GstPad *inputBinPad = gst_element_get_request_pad(nv_mux, inputPadName.c_str());
    // Check if the pad was created.
    if (inputBinPad == NULL)
      LOG(FATAL) << "Could not get the nvstreammux request pad=" << inputPadName;

    std::string inputGhostPadName = (std::string) "input" + std::to_string(i);
    GstPad *inputGhostPad = gst_ghost_pad_new(inputGhostPadName.c_str(), inputBinPad);
    if (inputGhostPad == NULL)
      LOG(FATAL) << "Could not create the ghostPad for bin=" << binName << ", ghostPadName=" << inputGhostPadName;

    if (!gst_element_add_pad(bin, inputGhostPad))
      LOG(FATAL) << "Could not add the ghostPad to bin=" << binName << ", ghostPadName=" << inputGhostPadName;
    gst_object_unref(GST_OBJECT(inputBinPad));
    gst_pad_set_active (GST_PAD_CAST (inputGhostPad), 1);
    VLOG(DEBUG) << "Added ghost pad to bin=" << binName << " with pad=" << inputGhostPadName;

    // create ghost pad for each output pad (src)
    std::string outputPadName = (std::string) "src_" + std::to_string(i);
    GstPad *outputBinPad = gst_element_get_request_pad(nv_demux, outputPadName.c_str());
    // Check if the pad was created.
    if (outputBinPad == NULL)
      LOG(FATAL) << "Could not get the nvstreammux request pad=" << inputPadName;

    std::string outputGhostPadName = (std::string) "output" + std::to_string(i);
    GstPad *outputGhostPad = gst_ghost_pad_new(outputGhostPadName.c_str(), outputBinPad);
    if (outputGhostPad == NULL)
      LOG(FATAL) << "Could not create the ghostPad for bin=" << binName << ", ghostPadName=" << outputGhostPadName;

    if (!gst_element_add_pad(bin, outputGhostPad))
      LOG(FATAL) << "Could not add the ghostPad to bin=" << binName << ", ghostPadName=" << outputGhostPadName;
    gst_object_unref(GST_OBJECT(outputBinPad));
    gst_pad_set_active (GST_PAD_CAST (outputGhostPad), 1);
    VLOG(DEBUG) << "Added ghost pad to bin=" << binName << " with pad=" << outputGhostPadName;
  }
  return bin;
}

inline GstElement* createSinkBinToDisplay(std::string binName, bool sync) {
  // create bin
  GstElement* bin = gst_bin_new(binName.c_str());
  // create elements
  GstElement *sink_nvconvert, *sink_convert, *sink_caps, *sink_queue, *sink;
  sink_nvconvert = gst_element_factory_make("nvvideoconvert", "sink_nvconvert");
  g_object_set(sink_nvconvert,
               "compute-hw", 1,
               NULL);
  sink_convert = gst_element_factory_make("videoconvert", "sink_convert");
  sink_caps = gst_element_factory_make("capsfilter", "sink_caps");
  g_object_set(sink_caps,
               "caps", gst_caps_from_string("video/x-raw,format=(string)YV12"),
               NULL);
  sink_queue = gst_element_factory_make("queue", "sink_queue");
  sink = gst_element_factory_make("xvimagesink", "sink");
  g_object_set(sink, "sync", sync, NULL);
  g_object_set(sink, "async", true, NULL);

  // add elements to the bin
  gst_bin_add_many(GST_BIN(bin), sink_nvconvert, sink_convert, sink_caps, sink_queue, sink, NULL);
  if(!gst_element_link_many(sink_nvconvert, sink_convert, sink_caps, sink_queue, sink, NULL))
    LOG(FATAL) << "Failed to add elements to bin=" << binName;

  // create ghost pad at output for future linking
  std::string inputPadName = "sink";
  GstPad *inputBinPad = gst_element_get_static_pad(sink_nvconvert, inputPadName.c_str());
  // Check if the pad was created.
  if (inputBinPad == NULL)
    LOG(FATAL) << "Could not get the sink_convert static pad=" << inputPadName;

  std::string inputGhostPadName = "input0";
  GstPad *inputGhostPad = gst_ghost_pad_new(inputGhostPadName.c_str(), inputBinPad);
  if (inputGhostPad == NULL)
    LOG(FATAL) << "Could not create the ghostPad for bin=" << binName << ", ghostPadName=" << inputGhostPadName;

  if (!gst_element_add_pad(bin, inputGhostPad))
    LOG(FATAL) << "Could not add the ghostPad to bin=" << binName << ", ghostPadName=" << inputGhostPadName;
  gst_object_unref(GST_OBJECT(inputBinPad));
  gst_pad_set_active (GST_PAD_CAST (inputGhostPad), 1);
  VLOG(DEBUG) << "Added ghost pad to bin=" << binName << " with pad=" << inputGhostPadName;
  return bin;
}

inline GstElement* createSinkBinToRTMP(std::string binName, std::string uri, bool sync) {
  // create bin
  GstElement* bin = gst_bin_new(binName.c_str());
  // create elements
  GstElement *sink_nvconvert, *sink_convert, *sink_caps, *sink_encode, *sink_mux, *sink_queue, *sink;
  sink_nvconvert = gst_element_factory_make("nvvideoconvert", "sink_nvconvert");
  g_object_set(sink_nvconvert,
               "compute-hw", 1,
               NULL);
  sink_convert = gst_element_factory_make("videoconvert", "sink_convert");
  sink_caps = gst_element_factory_make("capsfilter", "sink_caps");
  g_object_set(sink_caps,
               "caps", gst_caps_from_string("video/x-raw,format=(string)YV12"),
               NULL);
  sink_encode = gst_element_factory_make("x264enc", "sink_encode");
  sink_mux = gst_element_factory_make("flvmux", "sink_mux");

  sink_queue = gst_element_factory_make("queue", "sink_queue");
  sink = gst_element_factory_make("rtmpsink", "sink");
  g_object_set(sink,
               "sync", sync,
               "async", true,
               "location", uri.c_str(),
               NULL);

  // add elements to the bin
  gst_bin_add_many(GST_BIN(bin), sink_nvconvert, sink_convert, sink_caps, sink_encode, sink_mux, sink_queue, sink, NULL);
  if(!gst_element_link_many(sink_nvconvert, sink_convert, sink_caps, sink_encode, sink_mux, sink_queue, sink, NULL))
    LOG(FATAL) << "Failed to add elements to bin=" << binName;

  // create ghost pad at output for future linking
  std::string inputPadName = "sink";
  GstPad *inputBinPad = gst_element_get_static_pad(sink_nvconvert, inputPadName.c_str());
  // Check if the pad was created.
  if (inputBinPad == NULL)
    LOG(FATAL) << "Could not get the sink_convert static pad=" << inputPadName;

  std::string inputGhostPadName = "input0";
  GstPad *inputGhostPad = gst_ghost_pad_new(inputGhostPadName.c_str(), inputBinPad);
  if (inputGhostPad == NULL)
    LOG(FATAL) << "Could not create the ghostPad for bin=" << binName << ", ghostPadName=" << inputGhostPadName;

  if (!gst_element_add_pad(bin, inputGhostPad))
    LOG(FATAL) << "Could not add the ghostPad to bin=" << binName << ", ghostPadName=" << inputGhostPadName;
  gst_object_unref(GST_OBJECT(inputBinPad));
  gst_pad_set_active (GST_PAD_CAST (inputGhostPad), 1);
  VLOG(DEBUG) << "Added ghost pad to bin=" << binName << " with pad=" << inputGhostPadName;
  return bin;
}


inline GstElement* createSinkBinToFile(std::string binName, std::string fileName, bool sync) {
  // create bin
  GstElement* bin = gst_bin_new(binName.c_str());
  // create elements
  GstElement *sink_nvconvert, *sink_convert, *sink_caps, *sink_encode, *sink_mux, *sink_queue, *sink;
  sink_nvconvert = gst_element_factory_make("nvvideoconvert", "sink_nvconvert");
  g_object_set(sink_nvconvert,
               "compute-hw", 1,
               NULL);
  sink_convert = gst_element_factory_make("videoconvert", "sink_convert");
  sink_caps = gst_element_factory_make("capsfilter", "sink_caps");
  g_object_set(sink_caps,
               "caps", gst_caps_from_string("video/x-raw,format=(string)YV12"),
               NULL);
  sink_encode = gst_element_factory_make("x264enc", "sink_encode");
  sink_mux = gst_element_factory_make("flvmux", "sink_mux");

  sink_queue = gst_element_factory_make("queue", "sink_queue");
  sink = gst_element_factory_make("filesink", "sink");
  g_object_set(sink,
               "sync", sync,
               "async", true,
               "location", fileName.c_str(),
               NULL);

  // add elements to the bin
  gst_bin_add_many(GST_BIN(bin), sink_nvconvert, sink_convert, sink_caps, sink_encode, sink_mux, sink_queue, sink, NULL);
  if(!gst_element_link_many(sink_nvconvert, sink_convert, sink_caps, sink_encode, sink_mux, sink_queue, sink, NULL))
    LOG(FATAL) << "Failed to add elements to bin=" << binName;

  // create ghost pad at output for future linking
  std::string inputPadName = "sink";
  GstPad *inputBinPad = gst_element_get_static_pad(sink_nvconvert, inputPadName.c_str());
  // Check if the pad was created.
  if (inputBinPad == NULL)
    LOG(FATAL) << "Could not get the sink_convert static pad=" << inputPadName;

  std::string inputGhostPadName = "input0";
  GstPad *inputGhostPad = gst_ghost_pad_new(inputGhostPadName.c_str(), inputBinPad);
  if (inputGhostPad == NULL)
    LOG(FATAL) << "Could not create the ghostPad for bin=" << binName << ", ghostPadName=" << inputGhostPadName;

  if (!gst_element_add_pad(bin, inputGhostPad))
    LOG(FATAL) << "Could not add the ghostPad to bin=" << binName << ", ghostPadName=" << inputGhostPadName;
  gst_object_unref(GST_OBJECT(inputBinPad));
  gst_pad_set_active (GST_PAD_CAST (inputGhostPad), 1);
  VLOG(DEBUG) << "Added ghost pad to bin=" << binName << " with pad=" << inputGhostPadName;
  return bin;
}

// todo: add this logic to a configuration sanitizer
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/// CONFIG PARSER

inline bool checkStringStartsWith(const std::string& str, const std::string& prefix) {
  return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

inline bool checkStringEndsWith(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
}

inline bool areAllElementsUniqueStrings(const njson& jsonArray) {
  std::unordered_set<std::string> uniqueElements;

  for (const auto& element : jsonArray) {
    if (!element.is_string()) {
      LOG(WARNING) << "Expect string elements only";
      return false; // Element is not a string
    }

    // Check if the element is already in the set (duplicate)
    auto result = uniqueElements.insert(element.get<std::string>());
    if (!result.second) {
      LOG(WARNING) << "Found a duplicate entry: " << element.get<std::string>();
      return false; // Duplicate found
    }
  }
  return true;
}

inline bool areAllElementsStrings(const njson& jsonArray) {
  for (const auto& element : jsonArray) {
    if (!element.is_string()) {
      return false; // Element is not a string
    }
  }
  return true; // All elements are strings
}

inline void displayFilesSaved(const njson& jsonArray) {

  for (size_t i = 0; i < jsonArray.size(); ++i) {
    std::cout << "Saved file: " << jsonArray[i] << std::endl;
  }
}

}  // namespace pipelineUtils