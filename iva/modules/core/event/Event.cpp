#include "Event.h"

using namespace core;
using namespace core::events;

/**
 * @brief used to print the module with LOG(<>)
 * @param os stringstream
 * @param module module_id of the module as defined in Event.h
 * @return a human readable string of the module for logging
 */
std::ostream& core::events::operator<<(std::ostream& os, core::events::Module module)
{
  os << core::events::module_to_str.at(module);
  return os;
}

/**
 * @brief used to print the module with LOG(<>)
 * @param os stringstream
 * @param action action type as defined in Event.h
 * @return a human readable string of the module for logging
 */
std::ostream& core::events::operator<<(std::ostream& os, core::events::Actions action)
{
  os << core::events::action_to_str.at(action);
  return os;
}

EventBase::EventBase() {}

EventBase::EventBase(Type type) { this->t = events::Type(type); }

EventBase::~EventBase() {}

/****************
 * APPCONTEXT
 ****************/

/**
 * @brief process safe signal to ensure parallel threads don't collide. Refer to Mediator.cpp poly_event->own() ... poly_event->end()
 */
void EventBase::end()
{
  this->free();
  this->_completed = true;
}

/**
 * @brief Construct a new EventBase::EventBase object.
 *
 * @details Type must be passed if creating a base Event Object and not one of its derived classes.
 *
 * @param action  what the event is supposed to do
 * @param type    type of the event, in most cases this is the targeted module, same as dst
 * @param src     (optional) id of the source module (in most cases, you can use this->_module_id)
 * @param dst     (optional) id of the target module
 */
EventBase::EventBase(const int action, const int type, const int src, const int dst)
{
  this->_action = action;
  this->t = Type(type);
  this->_source = src;
  this->_target = dst;
}


/*********************
 * APPLICATION EVENT *
 *********************/

/**
 * @brief Events from the application module
 *
 * @param action    type of the event, in most cases this is the targeted module, same as dst (refer to Event.h)
 * @param src       the sender module of the event
 */
core::ApplicationEvent::ApplicationEvent(const gint action, const guint src)
{
    this->t = core::events::Type::APPLICATION;
    this->_action = action;

    if (src > 0)
        this->_source = src;
}

/***************
 * KafkaEvent *
 ***************/

/**
 * @brief KafkaEvent structure for sending data to the PRODUCER
 *
 * @param action    type of the event, in most cases this is the targeted module, same as dst (refer to Event.h)
 * @param uuid      id of the Processing
 * @param src       id of the Processing src (stream_id)
 */
core::KafkaEvent::KafkaEvent(const int action, const std::string uuid, const guint src)
{
  std::string log_prefix = "[KafkaEvent]\n -- ";
  this->_action = action;
  this->t = core::events::Type::KAFKA;
  this->_target = core::events::Module::MODULE_KAFKA;
  this->_requested_uuid = uuid;
  this->_source = src;
}

/**
 * @brief KafkaEvent structure for receiving data from the CONSUMER
 * @param action    type of the event, in most cases this is the targeted module, same as dst (refer to Event.h)
 */
core::KafkaEvent::KafkaEvent(const int action)
{
    this->_action = action;

    this->t = core::events::Type::KAFKA;
    this->_target = core::events::Module::MODULE_PIPELINE;
}
/*********************
 * PipelineEvent *
 *********************/

/**
 * @brief Construct a new PipelineEvent::PipelineEvent object
 *
 * @param action    type of the event, in most cases this is the targeted module, same as dst (refer to Event.h)
 * @param uuid      uuid of the Pipeline to be accessed by the event
 * @param src       id of the Processing src (stream_id)
 *
 */
PipelineEvent::PipelineEvent(const int action, const std::string uuid, const gint src)
{
    this->_action = action;
    this->t = core::events::Type::PIPELINE;
    this->_target = core::events::Module::MODULE_PIPELINE;
}
