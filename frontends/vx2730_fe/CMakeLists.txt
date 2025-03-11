cmake_minimum_required(VERSION 3.0)
project(vx2740 C CXX)

#define Dig2 library from git submodule
file(GLOB Dig2_SRC
     "caen-dig2/src/*.cpp"
     "caen-dig2/src/endpoints/*.cpp"
)

add_library (CAEN_Dig2 SHARED ${Dig2_SRC})
target_include_directories(CAEN_Dig2 PUBLIC caen-dig2/include caen-dig2/backend-server/include caen-felib/include caen-dig2/json/include/ caen-dig2/spdlog/include)
set_property(TARGET CAEN_Dig2 PROPERTY CXX_STANDARD 20)
add_compile_definitions(_DARWIN_C_SOURCE)
set(LIBS -ldl -lpthread -lutil)
target_link_libraries(CAEN_Dig2 ${LIBS})


find_package(Boost 1.75.0)
if(Boost_FOUND)
   target_include_directories(CAEN_Dig2 PUBLIC ${Boost_INCLUDE_DIRS})
  target_link_libraries(CAEN_Dig2 ${Boost_LIBRARIES})
else()
   message( FATAL_ERRO, "Boost not found, please use BOOST_ROOT to point to it" )
endif()

#define FELib library from git submodule
file(GLOB Felib_SRC
     "caen-felib/src/*.c"
)
add_library (CAEN_FELib SHARED ${Felib_SRC})
target_include_directories(CAEN_FELib PUBLIC caen-felib/include caen-felib/src)

#define digitizerlib, including only non-midas code
set(LIBDIGITIZER_SRC
  src/CaenDigitizer.cxx
  src/CaenException.cxx
  src/CaenParameter.cxx
)

set(INCDIRS
  include
)

add_library(digitizer STATIC ${LIBDIGITIZER_SRC})
target_include_directories(digitizer PUBLIC ${INCDIRS})
target_link_libraries(digitizer CAEN_FELib)

#test executable for libdigitizer
add_executable(test src/main.cxx)
target_link_libraries(test digitizer)

#define midasdigitizerlib, including midas-dependant code
# Check for MIDASSYS environment variable
if (NOT DEFINED ENV{MIDASSYS})
   message(SEND_ERROR "MIDASSYS environment variable not defined.")
endif()
set(MIDASSYS $ENV{MIDASSYS})

set(LIBMIDASDIGITIZER_SRC
  src/CaenDigitizerMidas.cxx
	${MIDASSYS}/src/odb.cxx
	${MIDASSYS}/src/odbxx.cxx
)

set(MIDASINCDIRS
  include
  ${MIDASSYS}/include
  ${MIDASSYS}/mscb/include/
	${MIDASSYS}/mxml
	${MIDASSYS}/mjson
)

add_library(midasdigitizer STATIC ${LIBMIDASDIGITIZER_SRC})
target_include_directories(midasdigitizer PUBLIC ${MIDASINCDIRS})
target_link_libraries(midasdigitizer digitizer)

#actual frontend starts here
add_executable(vx2730_fe
  src/vx2730_fe.cxx
	${MIDASSYS}/src/mfe.cxx
	)

target_include_directories(vx2730_fe PRIVATE
	${CMAKE_SOURCE_DIR}/include
	${MIDASSYS}/include
	${MIDASSYS}/mxml
)
target_link_libraries(vx2730_fe 
  midasdigitizer
	${MIDASSYS}/lib/libmfe.a
	${MIDASSYS}/lib/libmidas.a
)

install(TARGETS vx2730_fe test DESTINATION bin)

