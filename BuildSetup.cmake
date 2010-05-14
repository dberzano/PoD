#
# to build vsual-netstat:
#  1) mkdir build
#  2) cd build
#  3) cmake -C ../BuildSetup.cmake ..
#  4) gmake install
#

# set cmake build type, default value is: RelWithDebInfo
# possible options are: None Debug Release RelWithDebInfo MinSizeRel
set( CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE )

#set(CMAKE_VERBOSE_MAKEFILE TRUE CACHE BOOL "This is useful for debugging only." FORCE)

# This is needed if you want to use gLite plug-in and have several version of BOOST installed
#set( Boost_USE_MULTITHREADED OFF CACHE BOOL "BOOST" FORCE )

# Documentation
set( BUILD_DOCUMENTATION ON CACHE BOOL "Build source code documentation" FORCE )

# Build TESTS
set( BUILD_TESTS ON CACHE BOOL "Build pod-console unit tests" FORCE )

# gLite plug-in
#set( BUILD_GLITE_PLUGIN OFF CACHE BOOL "Build source code documentation" FORCE )

# LSF plug-in
#set( BUILD_LSF_PLUGIN OFF CACHE BOOL "Build source code documentation" FORCE )
#set( LSF_PREFIX "/home/anar/LSF" CACHE STRING "LSF prefix" FORCE)

# PBS plug-in
#set( BUILD_PBS_PLUGIN OFF CACHE BOOL "Build source code documentation" FORCE )
