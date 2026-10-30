cmake_minimum_required(VERSION 3.21)
project(UnifiedEmu VERSION 0.1.0 LANGUAGES CXX)

# C++17: path::u8string() devuelve std::string, que es lo que usa el detector.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(UNIFIEDEMU_BUILD_GUI "Compilar la interfaz Qt" ON)
option(UNIFIEDEMU_BUILD_TESTS "Compilar las pruebas del detector" ON)

if(MSVC)
  # El código fuente está en UTF-8 (textos en español).
  add_compile_options(/utf-8 /W4 /permissive-)
  add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
else()
  add_compile_options(-Wall -Wextra)
endif()

# Núcleo sin Qt: detección de juegos.
add_library(unifiedemu_detector STATIC src/core/GameDetector.cpp src/core/GameDetector.h)
target_include_directories(unifiedemu_detector PUBLIC src)

if(UNIFIEDEMU_BUILD_GUI)
  find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Concurrent)
  set(CMAKE_AUTOMOC ON)

  add_executable(UnifiedEmu WIN32
    src/main.cpp
    src/core/AppSettings.cpp
    src/core/AppSettings.h
    src/core/EmulatorLauncher.cpp
    src/core/EmulatorLauncher.h
    src/core/Paths.h
    src/ui/GameListModel.cpp
    src/ui/GameListModel.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/SettingsDialog.cpp
    src/ui/SettingsDialog.h
  )
  target_compile_definitions(UnifiedEmu PRIVATE UNIFIEDEMU_VERSION="${PROJECT_VERSION}")
  target_link_libraries(UnifiedEmu PRIVATE unifiedemu_detector Qt6::Widgets Qt6::Concurrent)

  install(TARGETS UnifiedEmu RUNTIME DESTINATION .)
endif()

if(UNIFIEDEMU_BUILD_TESTS)
  enable_testing()
  add_executable(GameDetectorTests tests/GameDetectorTests.cpp)
  target_link_libraries(GameDetectorTests PRIVATE unifiedemu_detector)
  add_test(NAME GameDetectorTests COMMAND GameDetectorTests)
endif()
