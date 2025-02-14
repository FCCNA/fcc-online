cmake_minimum_required(VERSION 3.0)
project(vx2740 C CXX)

file(GLOB Dig2_SRC
     "caen-dig2/src/*.cpp"
     "caen-dig2/src/endpoints/*.cpp"
)

add_library (CAEN_Dig2 SHARED ${Dig2_SRC})
target_include_directories(CAEN_Dig2 PUBLIC caen-dig2/include caen-dig2/backend-server/include caen-felib/include caen-dig2/json/include/ caen-dig2/spdlog/include)
set_property(TARGET CAEN_Dig2 PROPERTY CXX_STANDARD 14)
add_compile_definitions(_DARWIN_C_SOURCE)
set(LIBS -ldl -lpthread -lutil)
target_link_libraries(CAEN_Dig2 ${LIBS})


find_package(Boost 1.75.0 COMPONENTS ALL)
if(Boost_FOUND)
   target_include_directories(CAEN_Dig2 PUBLIC ${Boost_INCLUDE_DIRS})
  target_link_libraries(CAEN_Dig2 ${Boost_LIBRARIES})
else()
   message( FATAL_ERRO, "Boost not found, please use BOOST_ROOT to point to it" )
endif()
#target_include_directories(CAEN_Dig2 PUBLIC /opt/homebrew/opt/boost@1.85/include)

file(GLOB Felib_SRC
     "caen-felib/src/*.c"
)
add_library (CAEN_FELib SHARED ${Felib_SRC})
target_include_directories(CAEN_FELib PUBLIC caen-felib/include caen-felib/src)

add_executable(main src/main.c)
target_link_libraries(main CAEN_FELib)


