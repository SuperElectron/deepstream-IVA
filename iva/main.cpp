#include "Application.h"
#include "logging.hpp"

using namespace core;

int main(int argc, char *argv[])
{
  core::Logging::init(argv);
  google::InstallFailureSignalHandler();
  core::Application app;
  app.start();
}