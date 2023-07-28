#pragma once

#include <glog/logging.h>

#include <filesystem>

// Declare the global variable from argv[1] in main.cpp
extern std::string BASE_DIR;

namespace fs = std::filesystem;

/**
 * @brief Custom defined log levels
 *
 *      VLOG(DEBUG), VLOG(EVENT), VLOG(GST_TRACE), VLOG(DEEP)
 *
 *  @note Normal log levels are
 *      LOG(INFO), LOG(WARNING), LOG(FATAL)
 */

static const int DEBUG = 5;
static const int EVENT = 6;
static const int GST_TRACE = 10;
static const int DEEP = 15;


namespace core
{


/**
 * @class Logging
 * @brief set up GLogs (google logging)
 */
class Logging
{
public:
    inline static void init(char *argv[], int verbose = 0)
    {
        // logging setup
        google::InitGoogleLogging(argv[0]);

        FLAGS_logbuflevel = -1;
        FLAGS_colorlogtostderr = true;
        FLAGS_alsologtostderr = true;

        std::string current_path = ".";
        auto logs_dir = current_path + "/logs";
        auto logs_dir_info = logs_dir + "/info_logs/";
        auto logs_dir_warning = logs_dir + "/warning_logs/";
        auto logs_dir_error = logs_dir + "/error_logs/";
        auto logs_dir_fatal = logs_dir + "/fatal_logs/";

#ifdef ENABLE_DOT
        auto gst_dot = logs_dir + "/gst_debug_dot/";
#endif
        auto media_dir = current_path + (std::string) "/saved_media";
        auto images = media_dir + "/image/";
        auto payloads = media_dir + "/payload/";

        if (!fs::exists(logs_dir)) {
          // general directory where everything goes
          fs::create_directory(logs_dir);
          fs::permissions(logs_dir, fs::perms::all);
        }
        if (!fs::exists(logs_dir_info)) {
          // sub-folders for LOG(<level>)
          fs::create_directory(logs_dir_info);
          fs::permissions(logs_dir_info, fs::perms::all);
        }
        if (!fs::exists(logs_dir_warning)) {
          fs::create_directory(logs_dir_warning);
          fs::permissions(logs_dir_warning, fs::perms::all);
        }
        if (!fs::exists(logs_dir_error)) {
          fs::create_directory(logs_dir_error);
          fs::permissions(logs_dir_error, fs::perms::all);
        }
        if (!fs::exists(logs_dir_fatal)) {
          fs::create_directory(logs_dir_fatal);
          fs::permissions(logs_dir_fatal, fs::perms::all);
        }

#ifdef ENABLE_DOT
        if (!fs::exists(gst_dot)) {
            // store GST_DEBUG_DUMP_DOT_DIR diagrams for gstreamer pipeline
            fs::create_directory(gst_dot);
            fs::permissions(gst_dot, fs::perms::all);
        }
#endif

        if (!fs::exists(media_dir)) {
            // store generated outputs (json, video, images)
            fs::create_directory(media_dir);
            fs::permissions(media_dir, fs::perms::all);
        }
        if (!fs::exists(images)) {
            fs::create_directory(images);
            fs::permissions(images, fs::perms::all);
        }
        if (!fs::exists(payloads)) {
            fs::create_directory(payloads);
            fs::permissions(payloads, fs::perms::all);
        }

        google::SetLogDestination(google::GLOG_INFO, logs_dir_info.c_str());
        google::SetLogDestination(google::GLOG_WARNING, logs_dir_warning.c_str());
        google::SetLogDestination(google::GLOG_ERROR, logs_dir_error.c_str());
        google::SetLogDestination(google::GLOG_FATAL, logs_dir_fatal.c_str());
    }
};

}  // namespace core
