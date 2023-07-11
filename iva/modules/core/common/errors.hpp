#pragma once

#include <exception>
#include <string>


namespace core
{

/**
 * @namespace errors
 * @brief Error structure for handling faults
 *
 */
namespace errors
{
/* common errors */
const int NO_ERROR = 0;  // success

/* specific module and sub-module errors */

// APPLICATION RANGE :: 1001 - 2000
const int NO_CONFIGS_DIRECTORY = 1001;
const int INVALID_CONFIG = 1003;

/**
 * Readable strings for errors
 *
 * @param errnum : The error number to translate to a string
 * @return A stringified version of the error
 */
inline const char *strerror(int errnum)
{
    std::string msg;

    switch (errnum)
    {
        /* common errors */
        case NO_ERROR:
            msg = "OK";
            break;
            /* APPLICATION RANGE :: 1 - 100000 */
        case INVALID_CONFIG:
            msg = "Invalid config.json passed into the application";
            break;

        default:
            msg = "NO MESSAGE ASSIGNED TO ERROR CODE";
            break;
    }
    return msg.c_str();
}

/**
 * @class BaseError
 * @brief Derived from std::exception and responsible for describing all program errors
 * @var _errnum
 * the number of the error
 * @var _user_message
 * the human readable message for this error for logging purposes
 */
class BaseError : public std::exception
{
public:
    /**
     * @brief This constructor simply sets the error value into a local instance variable
     *    that be retrieved with a call to code
     * @param errnum the ERROR_CODE (e.g. INVALID_CONFIG)
     * @param msg the user message to display
     */
    explicit BaseError(int errnum, const std::string &msg = "")
    {
        this->_errnum = errnum;
        this->_user_message = msg;
    }

    /**
     * @brief override operator
     * @param errnum the ERROR_CODE (e.g. INVALID_CONFIG)
     * @param msg the user message to display
     */
    void operator()(const int errnum, const std::string &msg = "")
    {
        this->_errnum = errnum;
        this->_user_message = msg;
    }

    /**
     * @brief Exception message
     * @return a human readable error
     */
    virtual const char *what() const noexcept
    {
        if (!_user_message.empty())
        {
            return this->_user_message.c_str();
        }
        return core::errors::strerror(this->code());
    }

    /* Accessor to the underlying error code */
    int code() const

    noexcept { return this->_errnum; }

private:
    /* The error code from the module/system/library that this exception wraps */
    int _errnum;
    std::string _user_message;
};
}  // namespace errors
}  // namespace core

