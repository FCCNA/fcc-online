cmake_minimum_required(VERSION 3.0)
project(caenhv_fe)

#default target name
set(targetname caenhv_fe)

# Typical system types are SY1527 and SY4527.
# See CAENHV_SYSTEM_TYPE_t in CAENHVWrapper.h for the full list.
if (NOT DEFINED SYSTEM_TYPE)
  set(SYSTEM_TYPE N1470)
else()
  set(targetname "caenhv_fe_${SYSTEM_TYPE}")
  set(EXPLICIT_SYSTEM_TYPE 1)
endif()

message("Compiling executable ${targetname} for CAEN system ${SYSTEM_TYPE}")

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

add_executable(${targetname}
  caenhv_fe.cxx
  dsproto_sy4527/caenhv_fe_class.cxx
  dsproto_sy4527/caenhvwrapper_cxx.cxx)

install(TARGETS ${targetname} DESTINATION ${INSTALL_DIR})

target_compile_options(${targetname} PRIVATE -DUNIX -DSYSTEM_TYPE=${SYSTEM_TYPE})
if (${EXPLICIT_SYSTEM_TYPE})
  target_compile_options(${targetname} PRIVATE -DEXPLICIT_SYSTEM_TYPE)
endif()


# No CAEN dir
target_include_directories(${targetname} PRIVATE
  ${MIDASSYS}/include
  dsproto_sy4527/
)

target_link_libraries(${targetname} ${MIDASSYS}/lib/libmfe.a ${MIDASSYS}/lib/libmidas.a ${CAEN_LIB} ${LIBS})


