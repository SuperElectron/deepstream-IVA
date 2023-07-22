#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <fstream>
#include <filesystem>

#include <nlohmann/json.hpp>

#include "LexActivator.h"

using njson = nlohmann::json;

#ifndef SECURITY_FOLDER
#define SECURITY_FOLDER "/tmp/.cache/licenses"
#endif

#define CONTACT_MSG "Please contact info@alphaiwsesolutions.com"

namespace security
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
class SoftwareLicense
{
public:
	SoftwareLicense();

	bool start();
	~SoftwareLicense();

private:
	bool init();
	bool setReleaseParams();
	bool activate();
	bool activateTrial();
    bool _parse_configs();

	// general product information
	std::string RELEASE_VERSION;
	std::string RELEASE_CHANNEL;
	std::string RELEASE_PLATFORM;
	// product validation info
	std::string PRODUCT_ID;
	std::string LICENSE_KEY;
	std::string PRODUCT_DAT_PATH;
};

} // namespace security
