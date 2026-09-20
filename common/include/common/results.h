// Processing success, error codes and failures 
#pragma once
#include <string>

enum class ResultCode {
    SUCCESS = 0, // Operation was successful
    ERROR = 1, // An error occurred during the operation
    FAILURE = 2 // The operation failed to complete successfully
};
