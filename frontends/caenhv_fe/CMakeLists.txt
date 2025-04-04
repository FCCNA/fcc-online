cmake_minimum_required(VERSION 3.0)
project(camehv_fe)

# Typical system types are SY1527 and SY4527.
# See CAENHV_SYSTEM_TYPE_t in CAENHVWrapper.h for the full list.
if (NOT DEFINED SYSTEM_TYPE)
  set(SYSTEM_TYPE N1470)
endif()

# Check for MIDASSYS environment variable
if (NOT DEFINED ENV{MIDASSYS})
   message(SEND_ERROR "MIDASSYS environment variable not defined.")
endif()

set(CMAKE_CXX_STANDARD 11)
set(MIDASSYS $ENV{MIDASSYS})

if (NOT DEFINED CAEN_INC_DIR)
	set(CAEN_INC_DIR /home/fcc/online/frontends/caenhv_fe/CAENlibs/CAENHV/include)
endif()

if (NOT DEFINED CAEN_LIB)
	set(CAEN_LIB /home/fcc/online/frontends/caenhv_fe/CAENlibs/CAENHV/lib/Unix/x86_64/libcaenhvwrapper.so)
endif()

if (NOT DEFINED INSTALL_DIR)
  set(INSTALL_DIR ${CMAKE_SOURCE_DIR}/bin)
endif()

add_executable(caenhv_fe
  caenhv_fe.cxx
  dsproto_sy4527/caenhv_fe_class.cxx
  dsproto_sy4527/caenhvwrapper_cxx.cxx)

install(TARGETS caenhv_fe DESTINATION ${INSTALL_DIR})

target_compile_options(caenhv_fe PRIVATE -DUNIX -DSYSTEM_TYPE=${SYSTEM_TYPE})

# No CAEN dir
target_include_directories(caenhv_fe PRIVATE
  ${MIDASSYS}/include
  dsproto_sy4527/
)

target_link_libraries(caenhv_fe ${MIDASSYS}/lib/libmfe.a ${MIDASSYS}/lib/libmidas.a ${CAEN_LIB} ${LIBS})


