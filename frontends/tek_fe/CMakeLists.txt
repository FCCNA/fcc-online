cmake_minimum_required(VERSION 3.0)
project(tek_fe)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

# Check for environment variables
if (NOT DEFINED ENV{MIDASSYS})
    message(SEND_ERROR "MIDASSYS environment variable not defined.")
endif()

set(CMAKE_CXX_STANDARD 11)
set(MIDASSYS $ENV{MIDASSYS})

if (${CMAKE_SYSTEM_NAME} MATCHES Linux)
   set(LIBS ${LIBS} -lpthread -lutil -lrt -ldl)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES Darwin)
   set(LIBS ${LIBS} -lutil)
endif()

add_compile_definitions(UNIX)


add_executable(tek_cl
   src/tek_cl.cpp
   src/tek.cpp
)

target_include_directories(tek_cl PRIVATE
   ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(tek_cl ${LIBS})

install(TARGETS tek_cl DESTINATION bin)

add_executable(tek_fe
	src/tek_fe.cpp
	src/tek.cpp
	${MIDASSYS}/src/odb.cxx
	${MIDASSYS}/src/odbxx.cxx
	${MIDASSYS}/src/mfe.cxx
	)

target_include_directories(tek_fe PRIVATE
	${CMAKE_SOURCE_DIR}/include
	${MIDASSYS}/include
	${MIDASSYS}/mxml
	${MIDASSYS}/mscb/include
	${MIDASSYS}/mjson
	)

target_link_libraries(tek_fe
	${MIDASSYS}/lib/libmfe.a
	${MIDASSYS}/lib/libmidas.a
	${LIBS}
	)

install(TARGETS tek_fe DESTINATION bin)
