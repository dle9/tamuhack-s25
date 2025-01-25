# Main CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# # Set Bluetooth configurations before including project.cmake
# set(BLUETOOTH_ENABLED 1)
# set(BT_ENABLED 1)
# set(BLE_ENABLED 1)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_security_trainer)