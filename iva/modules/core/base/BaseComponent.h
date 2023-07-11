#pragma once

#include <glib.h>

#include <nlohmann/json.hpp>
#include <string>

#include "logging.hpp"
#include "Mediator.h"

using njson = nlohmann::json;

/**
 * @brief this is the main namespace for all all code
 *
 */
namespace core
{

// forward declaration for Mediator
class Mediator;

/**
 * @enum Module
 * @brief a list of modules for this program
 */
enum Module
{
    MODULE_NONE = 0,
    APPLICATION_CONTEXT,
    GST_PIPELINE,
    GST_PROCESSOR,
    KAFKA,
    MODULE_ERROR = 9999
};

static std::map <Module, std::string> module_to_str{{core::Module::MODULE_NONE,         "MODULE_NONE"},
                                                    {core::Module::APPLICATION_CONTEXT, "APPLICATION_CONTEXT"},
                                                    {core::Module::GST_PIPELINE,        "GST_PIPELINE"},
                                                    {core::Module::GST_PROCESSOR,       "GST_PROCESSOR"},
                                                    {core::Module::KAFKA,               "KAFKA"},
};

/**
 * @class BaseComponent
 * @brief the base class used for all modules so that the have access to mediator and mediator has access to them
 * @var _mediator
 * the mediator which is shared between modules
 * @var _module_id
 * the id of each module
 */
class BaseComponent
{
public:
    BaseComponent() = default;

    explicit BaseComponent(core::Mediator *mediator);

    inline void set_mediator(core::Mediator *mediator) { this->_mediator = mediator; }


    /**
     * @brief operator override ("<<") for this use:  LOG(INFO) << *this
     * @note terminal log will convert integer "_module_id" to  core::Module string with module_to_str()
     * @copydoc kakfa module: (LOG(INFO) << *this) will output "Modules (KAFKA)"
     *
     * @param os string stream that can be passed into LOG(INFO) via the "<<" operator
     * @param rhs the value, or right-hand-side, or key:value pair from module_to_str()
     * @return std::ostream - i.e. a string value that represents the module's name
     */
    friend std::ostream &operator<<(std::ostream &os, BaseComponent &rhs)  // const StateMachine cannot be declared here
    {
        os << "Module: (" << core::module_to_str[static_cast<core::Module>(rhs._module_id)] << ")";
        return os;
    }

    friend std::ostream &operator<<(std::ostream &os, core::Module module)
    {
        os << module_to_str.at(module);
        return os;
    }

protected:
    Mediator *_mediator;
    gint _module_id;
};
}  // namespace core

