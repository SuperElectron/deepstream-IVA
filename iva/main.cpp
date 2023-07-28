#include "Application.h"
#include "logging.hpp"
#include "SoftwareLicense.h"

#include <iostream>
#include <fstream>

using namespace security;
using namespace core;

std::string BASE_DIR;

int main(int argc, char *argv[])
{

  // check that .cache exists
  BASE_DIR = ".cache";
  std::ifstream f(BASE_DIR);
  if(!f.good())
    LOG(FATAL) << "Could not find main project directory (" << BASE_DIR << "). Check your path";

  SoftwareLicense license;
  bool success = license.start();
  if(!success)
    return EXIT_FAILURE;

  /* Set up logging. Note that MY_LOG_LEVEL is set in CMakeLists.txt */
#ifndef MY_LOG_LEVEL
#define MY_LOG_LEVEL 1 // Default log level (INFO)
#endif
  FLAGS_minloglevel = MY_LOG_LEVEL;
  core::Logging::init(argv);
  google::InstallFailureSignalHandler();

  LOG(ERROR) << "ERROR logs enabled";
  LOG(WARNING) << "WARNING logs enabled";
  LOG(INFO) << "INFO logs enabled";
  VLOG(DEBUG) << "DEBUG logs enabled";
  VLOG(DEEP) << "DEEP logs enabled";
  // start the app
  core::Application app;
  app.start();
}