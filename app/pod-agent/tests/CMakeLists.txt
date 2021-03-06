#************************************************************************
#
# CMakeLists.txt
# 
# Anar Manafov A.Manafov@gsi.de
# 
#
#        version number:    $LastChangedRevision$
#        created by:        Anar Manafov
#                           2009-12-25
#        last changed by:   $LastChangedBy$ $LastChangedDate$
#
#        Copyright (c) 2009-2010 GSI GridTeam. All rights reserved.
#*************************************************************************
project( pod-aget-tests )

include_directories(${pod-agent_SOURCE_DIR}/src)
#=============================================================================
add_executable(pod-agent_test_ProtocolCommands Test_ProtocolCommands.cpp)

target_link_libraries (
    pod-agent_test_ProtocolCommands
    pod_protocol
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

install(TARGETS pod-agent_test_ProtocolCommands DESTINATION tests)

#=============================================================================
set( SRC_PROTOCOL
    Test_Protocol.cpp
)

add_executable(pod-agent_test_Protocol ${SRC_PROTOCOL})

target_link_libraries (
    pod-agent_test_Protocol 
    pod_protocol
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

install(TARGETS pod-agent_test_Protocol DESTINATION tests)

#=============================================================================
#
# configure files
# 
configure_file( ${PROJECT_SOURCE_DIR}/xpd.cf.in ${PROJECT_BINARY_DIR}/xpd.cf @ONLY )

set( SRC_PROTOCOL
    Test_ProofStatusFile.cpp
)

add_executable(pod-agent_test_ProofStatusFile ${SRC_PROTOCOL})

target_link_libraries (
    pod-agent_test_ProofStatusFile
    proof_status_file
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_filesystem_libs} 
)

install(TARGETS pod-agent_test_ProofStatusFile DESTINATION tests)
install(FILES ${PROJECT_BINARY_DIR}/xpd.cf DESTINATION tests)

