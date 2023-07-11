#pragma once

#include <BS_thread_pool.hpp>
#include <nlohmann/json.hpp>
#include <string>

#include "Mediator.h"
#include "ApplicationContext.h"
#include "KafkaBroker.h"
#include "Pipeline.h"


using njson = nlohmann::json;

namespace core
{

/**
 * @class Application
 * @brief the main module that starts and runs all code
 * @var _mediator
 * the mediator used across modules
 * @var _app_context
 * the application context that manages application state
 * @var _kafka
 * the messaging service that enables communication with external clients
 * @var _pipeline
 * the gstreamer pipeline that runs all video and AI inference
 */
class Application
{
public:
    Application();

    void _distribute_module_configs();
    void start();
    ~Application();

private:
    core::Mediator *_mediator;
    core::ApplicationContext *_app_context;
    core::KafkaBroker *_kafka;
    core::Pipeline *_pipeline;

    void _set_up();
    void _stop_modules();
    void _start_modules();

};
} // namespace core
