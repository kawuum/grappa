
# (2.8 has ExternalProject support)
cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

option(TRACING "Sample statistics with VTrace at regular intervals." OFF)
option(GASNET_USE_UDP "Force Gasnet to use UDP (typically set by configure script)" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# if( NOT DEFINED ENV{CXX} )
#   message(FATAL_ERROR "you must set CC & CXX environment variables!")
# endif()

set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.clang.3_4")

project(Grappa)

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")

set(CMAKE_C_FLAGS_DEBUG "-O1 -DDEBUG -g -DGUARD_PAGES_ON_STACK")
set(CMAKE_CXX_FLAGS_DEBUG "-O1 -DDEBUG -g -DGUARD_PAGES_ON_STACK")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
enable_testing()
# set(CMAKE_BUILD_TYPE RelWithDebInfo)
site_name(MACHINENAME)

if(TRACING)
  # TODO:Fix VampirTrace support
  
  set(GOOGLE_PROFILER ON)

  add_definitions(-DVTRACE_SAMPLED -DVTRACE -DGOOGLE_PROFILER)

  include_directories("${THIRD_PARTY_ROOT}/include/vampirtrace")
  
endif()

# global C++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Winline -Wno-inline -mno-red-zone")
add_definitions("-DENABLE_RDMA_AGGREGATOR")

# TODO: use -stdlib=libc++ too?

#########################################
# Determine third party tools to build
if(NOT DEFINED THIRD_PARTY_ROOT)
  set(THIRD_PARTY_ROOT "${CMAKE_BINARY_DIR}/third-party" CACHE TYPE PATH)
endif()

###########################
# Use RUNPATH if available
# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
# use final INSTALL_RPATH even in build tree (this lets us manually add things to CMAKE_INSTALL_RPATH)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
# set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
   set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")

## Not setting runpath because having MPI libs in LD_LIBRARY_PATH was messing up VampirTrace's ability to find its own libs. Maybe there's another way to fix this, but it just seems more robust (for now) to not allow LD_LIBRARY_PATH to affect our libs (after configure uses it to find them).
# set runpath, too
# if(NOT APPLE)
#   set(CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-new-dtags")
# endif()

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:${THIRD_PARTY_ROOT}/lib")

if(TRACING)
  message("-- Tracing enabled.")
  if(NOT THIRD_PARTY_ROOT STREQUAL ${VAMPIR_ROOT})
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:${VAMPIR_ROOT}/lib")
  endif()
endif()

##### </RUNPATH> ########



if(TRACING)
  
  set(GOOGLE_PROFILER ON)
  
  include_directories("${VAMPIR_ROOT}/include/vampirtrace")
  link_directories("${VAMPIR_ROOT}/lib")
  
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finstrument-functions")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finstrument-functions")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -vt:inst manual")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -vt:inst manual")
  
  # list(APPEND GRAPPA_LIBS vt otf z papi dl m)
  
  # set(CMAKE_C_FLAGS "-vt:cc  ${CMAKE_C_FLAGS} -pthread")
  # set(CMAKE_CXX_FLAGS "-vt:cxx ${GCC_BASE}/bin/g++ ${CMAKE_CXX_FLAGS}")
  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --as-needed --whole-archive -lvt --no-whole-archive --no-as-needed -lopen-trace-format -lz -lpapi -ldl -lm")

endif()


if( GOOGLE_PROFILER )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  add_definitions( -DGOOGLE_PROFILER )
endif()

# pre-compiled external dependencies
# TODO: make CMake build these (see GraphLab's CMake)
include_directories("${THIRD_PARTY_ROOT}/include")
link_directories("${THIRD_PARTY_ROOT}/lib")
list(APPEND GRAPPA_LIBS
  gflags
  glog
)

#####################################
# Boost
#####################################
set(BOOST_COMPONENTS filesystem system unit_test_framework)

set(Boost_USE_MULTITHREADED OFF)
find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS} QUIET)

if(NOT Boost_FOUND)
  set(Boost_USE_MULTITHREADED ON)
  find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS} QUIET)
endif()
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIR})
  link_directories(${Boost_LIBRARY_DIRS})
  string(REGEX REPLACE "/include" "" BOOST_PREFIX ${Boost_INCLUDE_DIR} )
  message("-- Boost found:  ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION} -- ${BOOST_PREFIX}")
  # Boost_LIBRARIES set with the libraries to link against
  foreach(lib ${Boost_LIBRARY_DIRS})
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:${lib}")
  endforeach()
  
else()
  message("-- Boost not found.")
  message("   !! Will download and build Boost, which may take a while.")
endif()

######################################
# Gasnet
######################################
if(DEFINED APPLE OR GASNET_USE_UDP)
  set(GASNET_CONDUIT "udp")
else()
  set(GASNET_CONDUIT "ibv") # values: ibv,mpi
endif()

set(GASNET_THREAD "seq")  # values: seq,par,parsync -- seq recommended
list(APPEND GRAPPA_LIBS "gasnet-${GASNET_CONDUIT}-${GASNET_THREAD}")

if( ${GASNET_CONDUIT} MATCHES "ibv")
  list(APPEND GRAPPA_LIBS "ibverbs")
endif()
if( ${GASNET_CONDUIT} MATCHES "mpi")
  list(APPEND GRAPPA_LIBS "ammpi")
endif()
if( ${GASNET_CONDUIT} MATCHES "udp")
  list(APPEND GRAPPA_LIBS "amudp")
endif()

string(TOUPPER ${GASNET_CONDUIT} GASNET_CONDUIT_UP)
string(TOUPPER ${GASNET_THREAD} GASNET_THREAD_UP)

add_definitions(
  "-DGASNET_${GASNET_THREAD_UP}"
  "-DGASNET_${GASNET_CONDUIT_UP}"
  "-DGASNET_CONDUIT_${GASNET_CONDUIT_UP}"
)
include_directories("${THIRD_PARTY_ROOT}/include/${GASNET_CONDUIT}-conduit")

list(APPEND GRAPPA_ENV
  GLOG_logtostderr=1
  GLOG_v=1
)


add_definitions(-DSHMMAX=${SHMMAX})
# add_definitions(-DSHMMAX=12884901888) # 12 GB
# add_definitions(-DSHMMAX=6442450944) # 6 GB

if(NOT APPLE)
  list(APPEND GRAPPA_LIBS pthread rt)
endif()

# set(BOOST_ROOT ...)
# find_package(Boost REQUIRED COMPONENTS filesystem system unit_test_framework )
# include_directories(${Boost_INCLUDE_DIR})
# link_directories(${Boost_LIBRARY_DIRS})

# MPI (mostly for booting, but we also use MPI communication in some places now)
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

macro(add_grappa_exe target exe )
  add_executable(${target} EXCLUDE_FROM_ALL ${ARGN})
  set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_NAME "${exe}")
  target_link_libraries(${target} Grappa)
  target_link_libraries(${target} ${MPI_CXX_LIBRARIES})
  target_link_libraries(${target} ${Boost_LIBRARIES})  
endmacro(add_grappa_exe)

macro(add_grappa_application name)
  add_grappa_exe(${name} ${name} ${ARGN})
  set_property(TARGET ${name} PROPERTY FOLDER "Applications") # For organization in Xcode project
endmacro(add_grappa_application)

set(GRAPPA_TESTS "")

macro(add_grappa_test name nnode ppn)
  get_filename_component(base ${name} NAME_WE) # name without extension
  
  add_grappa_exe(${name} ${name} ${ARGN})
  set_property(TARGET ${name} PROPERTY FOLDER "Tests") # For organization in Xcode project
  set_target_properties(${test} PROPERTIES
    COMPILE_FLAGS "-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=${base}"
  )
  
  # TODO: use "ctest" to make whole regression test suite
  add_custom_target("check-${base}"
    COMMAND ${CMAKE_BINARY_DIR}/bin/grappa_srun --test=${base} --nnode=${nnode} --ppn=${ppn}
    DEPENDS Grappa ${name}
  )
  add_test(NAME "test-${base}" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_BINARY_DIR}/bin/grappa_srun --test=${base} --nnode=${nnode} --ppn=${ppn}
  )
  
  set_property(TARGET "check-${base}" PROPERTY FOLDER "Tests")
  list(APPEND GRAPPA_TESTS "check-${base}")
endmacro(add_grappa_test)

####################
# find igor scripts
file(GLOB_RECURSE igor_scripts
  RELATIVE ${CMAKE_SOURCE_DIR}
  igor_*
)
string(REGEX REPLACE "build/[^;]+;?" "" igor_scripts "${igor_scripts}") # filter out build directory

foreach(script ${igor_scripts})
  get_filename_component(out_dir "${script}" PATH)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${out_dir})
  execute_process(COMMAND ln -sf ${CMAKE_SOURCE_DIR}/${script} ${CMAKE_BINARY_DIR}/${script})
endforeach()
# message("igor scripts: ${igor_scripts}")


# put Grappa system directory on include path for everything following this
include_directories(system)
include_directories(system/tasks)

####################################
# subdirectories
add_subdirectory(third-party)
add_subdirectory(system)
add_subdirectory(applications)
add_subdirectory(bin)
add_subdirectory(scratch)
add_subdirectory(doc)
