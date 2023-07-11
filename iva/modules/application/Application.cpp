#include "Application.h"

using namespace core;

/**
 * @brief class instantiation which MUST be empty for BaseClass set inherit mediator
 */
Application::Application(){};

/**
 * @brief do clean up for the class in this destructor (when Application leaves memory scope)
 */
Application::~Application(){};

/**
 * @brief set up the Application module with its gstreamer pipeline
 *
 * @param argc number of command line args from main()
 * @param argv list of command line args from main()
 * @return
 */
void Application::_set_up()
{
  /* setup our module components */
  ApplicationContext *app_context = new ApplicationContext();
  KafkaBroker *kafka = new KafkaBroker();
  Pipeline *pipeline = new Pipeline();
  Mediator *mediator = new Mediator(app_context, kafka, pipeline);

  // save modules in application memory
  LOG(INFO) << "Saving module references to application ";
  this->_mediator = mediator;
  this->_app_context = app_context;
  this->_kafka = kafka;
  this->_pipeline = pipeline;

  // set up modules with their configurations
  this->_app_context->_load_module_configs();
  this->_distribute_module_configs();
}

/**
 * @brief callable to start the main gstreamer pipeline thread
 */
void Application::start()
{
  this->_set_up();
  this->_start_modules();
  if(this->_app_context->run_state)
  {
    this->_app_context->app_state = core::APP_STATE::LIVE;
  }
  while (this->_app_context->app_state == core::APP_STATE::LIVE) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

/// EVENTS

/**
 * @brief creates mediator event to set each module up with its configuration settings from config.json
 */
void Application::_distribute_module_configs()
{
  LOG(INFO) << "Application setting up modules with their configs";
  ApplicationEvent *event = new ApplicationEvent(core::events::Actions::CONFIGURE_MODULES, core::events::Module::MODULE_APPLICATION);
  this->_mediator->notify(event);
}

void Application::_stop_modules()
{
  LOG(INFO) << "Application stopping modules";
  ApplicationEvent *event = new ApplicationEvent(core::events::Actions::STOP_MODULES, core::events::Module::MODULE_APPLICATION);
  this->_mediator->notify(event);

}

void Application::_start_modules()
{
  LOG(INFO) << "Application starting modules";
  ApplicationEvent *event = new ApplicationEvent(core::events::Actions::START_MODULES, core::events::Module::MODULE_APPLICATION);
  this->_mediator->notify(event);

}