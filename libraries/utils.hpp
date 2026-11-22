#pragma once

#include <cstring>
#include <stdexcept>
#include <iostream>
using namespace std;

namespace utils{
    inline runtime_error custom_runtime_error(int error_no, string msg = ""){
        return runtime_error("Error: " + msg +" "+ string(strerror(error_no)) +" [Error no : "+ to_string(error_no) + "]");
    }
}