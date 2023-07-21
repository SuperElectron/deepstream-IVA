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
#define SECURITY_FOLDER "/src/modules/softwareLicense/licenses"
#endif

//// general product information
//#define RELEASE_VERSION "1.0"
//#define RELEASE_CHANNEL "stable"
//#define RELEASE_PLATFORM "linux-x86_64"
//
////// iva-test
////#define PRODUCT_ID "bbe14017-b8d6-4839-8447-fc7e501be05c"
////#define LICENSE_KEY "655A47-D53C23-439488-377F39-30006D-4E295E"
////#define PRODUCT_DAT_PATH "/home/mat/code/github/alphawise/security/license.dat"
//
//// single-seat
//#define PRODUCT_ID "a530aaa8-8e27-495c-b08b-fb71e6d473ce"
//#define LICENSE_KEY "1B76E3-B95866-406CB6-49F32C-EE8C75-ECB765"
//#define PRODUCT_DAT_PATH "/home/mat/code/github/alphawise/security/single-seat.dat"

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
