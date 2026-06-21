#include "Config.h"

// Define the VERSION struct that the library expects
VERSION version = {
    .major = atoi(VERSION_MAJOR),
    .minor = atoi(VERSION_MINOR),
    .patch = atoi(VERSION_PATCH)
};

device_type THIS_DEVICE_TYPE = DEVICE_TYPE;