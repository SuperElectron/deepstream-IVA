#pragma once

#include <gst/gst.h>

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <fstream>

#include "BaseComponent.h"
#include "Mediator.h"
#include "logging.hpp"

using njson = nlohmann::json;

namespace core
{

class Mediator;
class BaseComponent;

/**
 * @enum APP_STATE
 * @brief describes the application states that must be managed with logic
 */
enum APP_STATE
{
  NO_APP_STATE = 0,
  CONFIGURATION = 1,
  START = 2,
  LIVE = 3,
  STOPPED = 4,
  ERROR = 5,
  SHUT_DOWN = 1000,
};

/**
 * @class ApplicationContext
 * @brief Derived from BaseComponent and responsible to keep updated application parameters for the lifetime of the app
 *
 * @details AppContext will NOT chase down other modules for information.
 * All modules are responsible for notifying and sending updated data to AppContext be accessed through the supervisor (id or name).
 *
 * @var run_state
 * if false, then the application will automatically die. This is a last resort for fatal errors where the application cannot run.
 * @var app_state
 * describes the state machine's state
 * @var _lock
 * safe access to attribute updates
 * @var _configs
 * the application module configurations loaded in Application.cpp
 */
class ApplicationContext : public BaseComponent
{
 public:
  ApplicationContext();
  void kill_app() { this->app_state = APP_STATE::SHUT_DOWN; }
  bool _load_module_configs();
  njson get_configs(core::events::Type module_type);

  /**************
   * App Details *
   ***************/
  bool run_state = false;
  int app_state = APP_STATE::NO_APP_STATE;

 private:
  std::mutex *_lock;
  njson _configs;

};
}  // namespace core
