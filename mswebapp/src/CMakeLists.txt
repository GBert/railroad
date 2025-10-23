cmake_minimum_required(VERSION 3.10)
project(mswebapp_cpp LANGUAGES CXX)

# Ensure a modern macOS SDK target for libc++ filesystem on AppleClang
if (APPLE)
    # 10.15+ recommended for full std::filesystem support in libc++
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment target")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(MSWEBAPP_WITH_OPENSSL "Enable HTTPS (requires OpenSSL)" OFF)

add_executable(mswebapp_cpp
    main.cpp
)

# Link stdc++fs only for older GCC libstdc++ (not needed on AppleClang/libc++ or modern GCC)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(mswebapp_cpp PRIVATE stdc++fs)
    endif()
endif()

target_include_directories(mswebapp_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

if (MSWEBAPP_WITH_OPENSSL)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(mswebapp_cpp PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(mswebapp_cpp PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()

# Installation
install(TARGETS mswebapp_cpp RUNTIME DESTINATION bin)

# Developer convenience: copy compile_commands.json to repository root if possible
if (EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
    file(COPY "${CMAKE_BINARY_DIR}/compile_commands.json" DESTINATION "${CMAKE_SOURCE_DIR}/..")
endif()
