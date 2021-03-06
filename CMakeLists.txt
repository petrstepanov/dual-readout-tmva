# ROOT CMake file written by Petr Stepanov (stepanovps@gmail.com)

# Instructions:
# 1. Put this CMakeLists.txt in the program directory containing project files: source files, header files and LinkDef.h
# 2. C++ file containing main() function must be named main.c (or .cc, .cpp etc...)
# 3. Specify the name of your executable below (stored it CMake PROJECT_NAME variable):
  project(dual-readout-tmva)

# Set the minimum required version of cmake for a project
  cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Locate the ROOT package and define a number of useful targets and variables (need RIO Net?)
  find_package(ROOT REQUIRED COMPONENTS)
# message(STATUS "Found ROOT libraries:")
# message(STATUS "${ROOT_LIBRARIES}")

# Compose list of ROOT libraries with "ROOT::" prefix
  set(LIB_NAMES "")
  FOREACH(X IN LISTS ROOT_LIBRARIES)
    get_filename_component(FILENAME ${X} NAME)
    string(REGEX REPLACE "lib([a-zA-Z0-9]+).so" "ROOT::\\1" FILENAME2 ${FILENAME})
    list(APPEND LIB_NAMES "${FILENAME2}")
  ENDFOREACH()

# Manually append extra ROOT libraries (why missing?)
  list(APPEND LIB_NAMES "ROOT::Gui")
#  list(APPEND LIB_NAMES "ROOT::RooFit")
#  list(APPEND LIB_NAMES "ROOT::RooFitCore")
  list(APPEND LIB_NAMES "ROOT::Html")
  list(APPEND LIB_NAMES "ROOT::Minuit")
  list(APPEND LIB_NAMES "ROOT::Fumili")
  list(APPEND LIB_NAMES "ROOT::TMVA")
  list(APPEND LIB_NAMES "ROOT::TMVAGui")

# message(STATUS "Modified ROOT libraries:")
# message(STATUS "${LIB_NAMES}")

#----------------------------------------------------------------------------
# Build list of header files and exclude LinkDef from headers
  file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hxx ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hh ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
  list(FILTER HEADERS EXCLUDE REGEX "[lL]ink[dD]ef")
  message(STATUS "Found list of headers:")
  message(STATUS "${HEADERS}")

#----------------------------------------------------------------------------
# Find LinkDef.h file
  file(GLOB_RECURSE LINKDEFH ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hxx ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hh ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
  list(FILTER LINKDEFH INCLUDE REGEX "[lL]ink[dD]ef")
  message(STATUS "Found LinkDef file: ${LINKDEFH}")

#----------------------------------------------------------------------------
# Generate dictionary
  ROOT_GENERATE_DICTIONARY(D_${PROJECT_NAME} ${HEADERS} LINKDEF ${LINKDEFH})

#----------------------------------------------------------------------------
# Build list of source files
  file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
  message(STATUS "Found list of sources:")
  message(STATUS "${SOURCES}")

#----------------------------------------------------------------------------
# Create shared library with a generated dictionary.
  add_library(SO_${PROJECT_NAME} ${SOURCES} D_${PROJECT_NAME}.cxx)

#----------------------------------------------------------------------------
# Link against shared library and list of ROOT libraries
  target_link_libraries(SO_${PROJECT_NAME} PUBLIC ${LIB_NAMES})

#----------------------------------------------------------------------------
# Find location of the enrty point file (main.c*)
  file(GLOB_RECURSE MAIN ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
  list(FILTER MAIN INCLUDE REGEX "main\\.c")
# message(STATUS "Found entry point file: ${MAIN}")

#----------------------------------------------------------------------------
# Create the main program using the library.
  add_executable(${PROJECT_NAME} ${MAIN})
  target_link_libraries(${PROJECT_NAME} SO_${PROJECT_NAME})

#----------------------------------------------------------------------------
# Compose the install target
install(TARGETS ${PROJECT_NAME} SO_${PROJECT_NAME} 
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION $ENV{ROOTSYS}/lib
        ARCHIVE DESTINATION $ENV{ROOTSYS}/lib)
install(FILES ${PROJECT_BINARY_DIR}/libD_${PROJECT_NAME}.rootmap
        DESTINATION $ENV{ROOTSYS}/lib)
install(FILES ${PROJECT_BINARY_DIR}/libD_${PROJECT_NAME}_rdict.pcm
        DESTINATION /usr/local/bin)

#----------------------------------------------------------------------------
# Compose the install launcher phony target
# https://stackoverflow.com/questions/38079251/get-cmake-to-declare-a-target-phony
add_custom_target(install-launcher
  # Install icon (SVG support dropped due to slow rendering speeds of some icons)
  COMMAND bash -c "xdg-icon-resource install --novendor --context apps --size 256 ${CMAKE_CURRENT_SOURCE_DIR}/resources/${CMAKE_PROJECT_NAME}.png ${CMAKE_PROJECT_NAME}"
  COMMAND bash -c "gtk-update-icon-cache"
  # Install launcher ("forceupdate" command performed automatically unless "--noupdate" is specified)
  COMMAND bash -c "xdg-desktop-menu install ${CMAKE_CURRENT_SOURCE_DIR}/resources/${CMAKE_PROJECT_NAME}.desktop"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Installing application icon and launcher (for linux)"
)
