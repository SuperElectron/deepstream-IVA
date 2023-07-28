#pragma once

#include <uuid/uuid.h>

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "logging.hpp"
#include "Event.h"

namespace core
{

// forward declare classes so that you can pass them as instantiation args for Mediator(<classes>)
class BaseComponent;
class ApplicationContext;
class KafkaBroker;
class Pipeline;
class Processing;
class EventBase;

/**
 * @class Mediator
 * @brief the main class the enables inter-module communication
 * @var app_context
 * the application context that holds all runtime information
 * @var kafka
 * the messaging service to connect with external clients
 * @var pipeline
 * the gstreamer service to run video pipelines and realtime inference
 * @var _mutex
 * safe access between threads or concurrent calls
 */
class Mediator
{
public:
    Mediator(core::ApplicationContext *app_context = nullptr,
             core::KafkaBroker *kafka = nullptr,
             core::Pipeline *pipeline = nullptr
    );

    void notify(core::BaseComponent* sender, std::string event_string);
    void notify(core::EventBase* event);

    core::ApplicationContext *app_context;
private:
    core::KafkaBroker *kafka;
    core::Pipeline *pipeline;
    std::mutex _mutex;
};
}  // namespace core

