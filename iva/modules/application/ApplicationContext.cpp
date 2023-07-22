#include "ApplicationContext.h"

using namespace core;

class Basecomponent;

ApplicationContext::ApplicationContext()
{
    this->_module_id = core::Module::APPLICATION_CONTEXT;
    LOG(INFO) << "CREATED: " << *this;
}


/**
 * @brief loads module settings from config.json
 * @return `bool` true if successful
 */
bool ApplicationContext::_load_module_configs()
{
	/* load in module configurations */
	std::ifstream f("/tmp/.cache/configs/config.json");
    if(!f.good())
      LOG(FATAL) << "Could not find /tmp/.cache/configs/config.json.  Ensure that you have mounted the configs directory to your project!";

	njson conf = njson::parse(f);
	if (conf.empty())
	{
		LOG(ERROR) << "config.json doesn't have any contents!";
		return false;
	}
    // check that config has appropriate fields
    if(!conf.contains("messaging"))
        return false;
    if(!conf.contains("pipeline"))
        return false;
    if(!conf.contains("processing"))
        return false;
	this->_configs = conf;
	VLOG(DEBUG) << "AppContext loaded in configs: \n" << this->_configs.dump(4);
	return true;
}

/**
 * @brief returns the key value of this->_configs (the module's configurations)
 *
 * @param module_type the module type as defined in Event.h
 * @return `njson` with module configs
 */
njson ApplicationContext::get_configs(core::events::Type module_type)
{
	njson module_settings;
	if(module_type == core::events::Type::PIPELINE)
	{
		module_settings = this->_configs["pipeline"];
	}
	else if (module_type == core::events::Type::PROCESSING)
	{
		module_settings = this->_configs["processing"];
	}
	else if (module_type == core::events::Type::KAFKA)
	{
		module_settings = this->_configs["messaging"];
	}
	else
	{
		LOG(ERROR) << "Invalid call, module_type arg is not known in logic setup.";
		throw "Invalid module_type";
	}
	return module_settings;
}