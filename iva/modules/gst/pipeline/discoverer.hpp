#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <iostream>

namespace rtspAnalyzer
{

struct UserData {
  bool& discovered;
  UserData(bool& discovered_) : discovered(discovered_) {}
};

inline void on_discovered(GstDiscoverer *discoverer, GstDiscovererInfo *info, GError *err, gpointer user_data) {
  if (err) {
    g_printerr("[on_discovered] Error: %s\n", err->message);
    g_error_free(err);
    return;
  }
  // unpack the flag
  UserData* userdata = static_cast<UserData*>(user_data);

  // Get the URI from the info
  std::string uri = (std::string) gst_discoverer_info_get_uri(info);

  // You can get various information about the stream here using gst_discoverer_info_get_* functions.
  // For example, to get the duration:
  gint64 duration = gst_discoverer_info_get_duration(info);
  std::cout << "Discovered URI: " << uri << "\tduration[ns]=" << duration << std::endl;
  userdata->discovered = true;
}

//inline bool analyzeRTSPStream(const std::string& rtspUri) {
//  GError *error = NULL;
//  GstDiscoverer *discoverer = gst_discoverer_new(5 * GST_SECOND, &error);
//
//  if (!discoverer) {
//    g_printerr("Error creating GstDiscoverer: %s\n", error->message);
//    g_error_free(error);
//    return false;
//  }
//
//  bool stream_discovered = false;
//  UserData userdata(stream_discovered);
//
//  // Connect the signal to the callback function
//  g_signal_connect(discoverer, "discovered", G_CALLBACK(on_discovered), &userdata);
//
//  // Discover the RTSP stream asynchronously
//  std::cout << "Checking uri:" << rtspUri << std::endl;
//
//  gst_discoverer_discover_uri_async(discoverer, rtspUri.c_str());
//  gst_discoverer_start();
//
//      // Start the GMainLoop to wait for the discovery to finish
//  GMainLoop *loop = g_main_loop_new(NULL, FALSE);
//  g_timeout_add_seconds(10, [](gpointer data) {
//        GMainLoop *loop = static_cast<GMainLoop*>(data);
//        g_main_loop_quit(loop);
//        return G_SOURCE_REMOVE;
//      }, loop);
//
//  g_main_loop_run(loop);
//
//  // Clean up
//  g_main_loop_unref(loop);
//  return stream_discovered;
//}

inline bool analyzeRTSPStream(const std::string& rtspUri) {
  GError *error = NULL;
  GstDiscoverer *discoverer = gst_discoverer_new(5 * GST_SECOND, &error);

  if (!discoverer) {
    g_printerr("Error creating GstDiscoverer: %s\n", error->message);
    g_error_free(error);
    return false;
  }

  bool stream_discovered = false;
  UserData userdata(stream_discovered);

  // Connect the signal to the callback function
  g_signal_connect(discoverer, "discovered", G_CALLBACK(on_discovered), &userdata);

  // Discover the RTSP stream synchronously
  std::cout << "Checking uri:" << rtspUri << std::endl;

  GstDiscovererInfo *info = gst_discoverer_discover_uri(discoverer, rtspUri.c_str(), &error);
  if (info) {
    // The RTSP stream was successfully discovered
    on_discovered(discoverer, info, NULL, &userdata);
    gst_discoverer_info_unref(info);
  } else {
    // Failed to discover the RTSP stream
    g_printerr("Error discovering RTSP stream: %s\n", error->message);
    g_error_free(error);
  }

  // Clean up
  g_object_unref(discoverer);
  return stream_discovered;
}

}
