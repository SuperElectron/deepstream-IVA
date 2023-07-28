#include "SoftwareLicense.h"

using namespace security;


inline bool checkIfFileExists(std::string filePath)
{
	std::ifstream file(filePath);
	return file.good();
}

// License callback is invoked when IsLicenseGenuine() completes a server sync
inline void LA_CC LicenseCallback(uint32_t status)
{
	// NOTE: Don't invoke IsLicenseGenuine(), ActivateLicense() or ActivateTrial() API functions in this callback
	printf("\nLicense status: %d\n", status);
}

// Software release update callback is invoked when CheckReleaseUpdate() gets a response from the server
inline void LA_CC SoftwareReleaseUpdateCallback(int status, Release* release, void* custom_data)
{
	switch (status)
	{
		case LA_RELEASE_UPDATE_AVAILABLE:
			printf("A new update is available for the app.\n");
			printf("Release notes: %s\n", release->notes);
			break;

		case LA_RELEASE_UPDATE_AVAILABLE_NOT_ALLOWED:
			printf("A new update is available for the app but it's not allowed.\n");
			printf("Release notes: %s\n", release->notes);
			break;

		case LA_RELEASE_UPDATE_NOT_AVAILABLE:
			printf("Current version is already latest.\n");
			break;

		default:
			printf("Error code: %d\n", status);
	}
}

SoftwareLicense::SoftwareLicense() {};

SoftwareLicense::~SoftwareLicense() {};

bool SoftwareLicense::start()
{
	bool configs_parsed = this->_parse_configs();
    if(!configs_parsed){
        return false;
    }

	bool init_success = this->init();
	if(!init_success){
		return false;
	}

	bool release_params_success = this->setReleaseParams();
	if(!release_params_success){
		return false;
	}
	bool is_activated = activate();
	return is_activated;
}

bool SoftwareLicense::_parse_configs()
{
	njson jsonData;

	// check if file exists
    std::string security_folder = (std::string) BASE_DIR + (std::string) "/licenses";
	std::string conf = security_folder + (std::string) "/security.json";
	if(!checkIfFileExists(conf))
	{
		printf("Cannot find security.json in path: %s \n", security_folder.c_str());
		return false;
	}

	try{
		std::ifstream file(conf);
		file >> jsonData;
	} catch (const std::exception &e) {
        printf("Error parsing security.json: \n\tERROR\n\t %s \n", e.what());
		return false;
	}

	try {
		this->RELEASE_VERSION = jsonData["RELEASE_VERSION"].get<std::string>();
		this->RELEASE_CHANNEL = jsonData["RELEASE_CHANNEL"].get<std::string>();
		this->RELEASE_PLATFORM = jsonData["RELEASE_PLATFORM"].get<std::string>();
		this->PRODUCT_ID = jsonData["PRODUCT_ID"].get<std::string>();
		this->LICENSE_KEY = jsonData["LICENSE_KEY"].get<std::string>();
		this->PRODUCT_DAT_PATH = jsonData["PRODUCT_DAT_PATH"].get<std::string>();
	} catch (const std::exception &e) {
        printf("Invalid file, security.json: %s\n", e.what());
		return false;
	}

	// validate that license.dat exists
	std::string dt_path = (std::string) security_folder + (std::string) "/" + this->PRODUCT_DAT_PATH;
	if(!checkIfFileExists(dt_path)) {
		printf("Could not find license.dat in path: %s\n", dt_path.c_str());
		return false;
	}

	this->PRODUCT_DAT_PATH = dt_path;
	// printf("Found license file: %s\n", this->PRODUCT_DAT_PATH.c_str());
    return true;
}

bool SoftwareLicense::init()
{
	int status;
	status = SetProductFile(PRODUCT_DAT_PATH.c_str());
	if (LA_OK != status)
	{
		printf("[SetProductFile] Error (status_code=%d). %s\n", status, CONTACT_MSG);
		return false;
	}

	status = SetProductId(PRODUCT_ID.c_str(), LA_USER);
	if (LA_OK != status)
	{
		printf("[SetProductId] Error (status_code=%d). %s\n", status, CONTACT_MSG);
		return false;
	}
	return true;
}


bool SoftwareLicense::setReleaseParams()
{
	int status;
	// Ensure that platform, channel and release actually exist for the release.
	status = SetReleasePlatform(RELEASE_PLATFORM.c_str()); // set the actual platform of the release e.g windows, macos, linux
	if (LA_OK != status)
	{
		printf("[SetReleasePlatform] Error (status_code=%d). %s\n", status, CONTACT_MSG);
		return false;
	}

	status = SetReleaseChannel(RELEASE_CHANNEL.c_str()); // set the actual channel of the release e.g stable
	if (LA_OK != status)
	{
		printf("[SetReleaseChannel] Error (status_code=%d). %s\n", status, CONTACT_MSG);
		return false;
	}

	status = SetReleaseVersion(RELEASE_VERSION.c_str());
	if (LA_OK != status)
	{
		printf("[SetReleaseVersion] Error (status_code=%d). %s\n", status, CONTACT_MSG);
		return false;
	}
	return true;
}


// Ideally on a button click inside a dialog
bool SoftwareLicense::activate()
{
	int status;

	status = SetLicenseKey(LICENSE_KEY.c_str());
	if (LA_OK != status)
	{
		printf("Your product license has failed. (error_code=%d). %s\n", status, CONTACT_MSG);
		return false;
	}

	status = SetActivationMetadata("key1", "value1");
	if (LA_OK != status)
	{
		printf("Error activating license metadata (error_code=%d).  %s\n", status, CONTACT_MSG);
		return false;
	}

	status = ActivateLicense();
	if (LA_OK == status)
	{
		printf("Product license activated successfully\n");
		return true;
	}
	else if (LA_EXPIRED == status)
	{
		printf("Product license has expired! %s \n", CONTACT_MSG);
		return false;
	}
	else if (LA_SUSPENDED == status)
	{
		printf("Product license has been suspended. %s \n", CONTACT_MSG);
		return false;
	}
	else
	{
		printf("License activation failed (error_code=%d). %s\n", status, CONTACT_MSG);
		return false;
	}
}


// Ideally on a button click inside a dialog
bool SoftwareLicense::activateTrial()
{
	int status;

	status = SetTrialActivationMetadata("key1", "value1");
	if (LA_OK != status)
	{
		printf("Error activating trial license metadata (error_code=%d).  %s\n", status, CONTACT_MSG);
		return false;
	}

	status = ActivateTrial();
	if (LA_OK == status)
	{
		printf("Product trial activated successfully!\n");
		return true;
	}
	else if (LA_TRIAL_EXPIRED == status)
	{
		printf("Product trial has expired! %s\n", CONTACT_MSG);
		return false;
	}
	else
	{
		printf("Product trial activation failed (status_code=%d). %s\n", status, CONTACT_MSG);
		return false;
	}
}
