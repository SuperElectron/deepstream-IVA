#include "Application.h"
#include "logging.hpp"
#include "SoftwareLicense.h"
#include <iostream>

using namespace security;
using namespace core;

std::string BASE_DIR;

int main(int argc, char *argv[])
{

  if (argc < 2) {
    std::cerr << "Usage    | $ " << argv[0] << " /path/to/.cache\n";
    std::cerr << "Try this | $ " << argv[0] << " /tmp/.cache\n";
    return 1;
  }

  BASE_DIR = argv[1];

  SoftwareLicense license;
  bool success = license.start();
  if(!success)
    return EXIT_FAILURE;

#ifndef MY_LOG_LEVEL
#define MY_LOG_LEVEL 1 // Default log level (INFO)
#endif

  FLAGS_minloglevel = MY_LOG_LEVEL;
  /* Set up logging. Note that GOOGLE_LOG_LEVEL is set in CMakeLists.txt */
  std::cout << "FLAGS_minloglevel=" << FLAGS_minloglevel << std::endl;
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