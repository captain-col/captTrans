# project name
project(datatypes)

# ----------------------------------------------------------------------
# Build this project's library:

set( LIB_NAME daq_datatypes)

set(LIB_SOURCES
write_read.cpp
evttypes.cpp
globalHeader.cpp
gps.cpp
triggerData.cpp
crateData.cpp
crateHeader.cpp
cardData.cpp
cardHeader.cpp
eventRecord.cpp
beamData.cpp
beamHeader.cpp
)

add_library( ${LIB_NAME} SHARED ${LIB_SOURCES}  )

add_executable(write_read write_read.cpp)

target_link_libraries(write_read
    daq_share 
    ${LIB_NAME}
    ${COMMON_PROJECT_LIBS}
    ${WINDRIVERLIB}
    ${BOOST_LIB}
    -ldl
    )

# ----------------------------------------------------------------------
# Install this project's headers, sources, library, and executables:

install_headers()
install_source()

install( TARGETS ${LIB_NAME}
         DESTINATION ${flavorqual_dir}/lib )

install( TARGETS write_read
         DESTINATION ${flavorqual_dir}/bin )

