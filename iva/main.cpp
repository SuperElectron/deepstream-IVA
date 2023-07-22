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

  VLOG_IS_ON(GOOGLE_LOG_LEVEL);

  core::Logging::init(argv);
  google::InstallFailureSignalHandler();
  core::Application app;
  app.start();
}