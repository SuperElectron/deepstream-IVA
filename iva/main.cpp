#include "Application.h"
#include "logging.hpp"
#include "SoftwareLicense.h"
#include <gflags/gflags.h>

#include <iostream>
#include <fstream>
#define kMaxLogSizeInMB 25
#define kMaxLogFileCount 10

using namespace security;
using namespace core;

std::string BASE_DIR = "";

std::string getHomeDirectory() {
  const char* homeDir = getenv("HOME");
  if (homeDir == nullptr) {
    // Handle the case where the HOME environment variable is not set
    return "";
  }
  return std::string(homeDir);
}

int main(int argc, char *argv[])
{
  std::string homeDir = getHomeDirectory();
  BASE_DIR = homeDir + "/.iva";

  // check that .cache exists
  std::ifstream f(BASE_DIR);
  if(!f.good())
    LOG(FATAL) << "Could not find main project directory (" << BASE_DIR << "). Check your path.  If you run with sudo this changes permissions!";

  SoftwareLicense license;
  bool success = license.start();
  if(!success)
    return EXIT_FAILURE;

  /* Set up logging. Note that MY_LOG_LEVEL is set in CMakeLists.txt */
#ifndef MY_LOG_LEVEL
#define MY_LOG_LEVEL 1 // Default log level (INFO)
#endif
  FLAGS_minloglevel = MY_LOG_LEVEL;
  FLAGS_max_log_size = kMaxLogSizeInMB;
  gflags::SetCommandLineOption("log_file_count", "5");

  core::Logging::init(argv);
  google::InstallFailureSignalHandler();


  // start the app
  core::Application app;
  app.start();
}