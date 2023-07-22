#include "Application.h"
#include "logging.hpp"
#include "SoftwareLicense.h"

using namespace security;
using namespace core;

int main(int argc, char *argv[])
{
  SoftwareLicense license;
  bool success = license.start();
  if(!success)
    return EXIT_FAILURE;

  /* Set up logging. Note that GOOGLE_LOG_LEVEL is set in CMakeLists.txt */
  FLAGS_minloglevel = GOOGLE_LOG_LEVEL;
  core::Logging::init(argv);
  google::InstallFailureSignalHandler();

  // start the app
  core::Application app;
  app.start();
}