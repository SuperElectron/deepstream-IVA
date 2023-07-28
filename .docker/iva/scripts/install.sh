#!/bin/bash

# Example usage: ./install.sh

export WORKDIR="/start/scripts"
export NAME="[install.sh]: "
echo "${NAME} STARTING "

# Bash failure reporting for the script
set -eE -o functrace
failure() {
  local lineno=$1
  local msg=$2
  echo "${NAME} Failed at $lineno: $msg"
}
trap '${NAME} failure ${LINENO} "$BASH_COMMAND"' ERR

echo "${NAME} pre-flight check for exiting folder $WORKDIR from docker COPY "
INSTALL_DIR_EXISTS="$(test -d $WORKDIR && echo 'yes' || echo 'no')"
if [[ "$INSTALL_DIR_EXISTS" == "no" ]]; then
  echo "--(fail)-- install directory exists: ${WORKDIR}"
  exit 1;
else
  echo "--(pass)-- install directory exists: ${WORKDIR}"
fi

echo "${NAME} install vcpkg "
VCPKG_VER=2022.10.19
cd /tmp && wget "https://github.com/microsoft/vcpkg/archive/refs/tags/${VCPKG_VER}.zip" && \
  cd /tmp && unzip "${VCPKG_VER}.zip" && \
  cd /tmp && rm -rf "${VCPKG_VER}.zip" && \
  mv /tmp/vcpkg-"${VCPKG_VER}" /start/vcpkg && \
  rm -rf /tmp/vcpk*

export VCPKG_FORCE_SYSTEM_BINARIES=1
cd "/start/vcpkg"; ./bootstrap-vcpkg.sh;
cd "/start/vcpkg"; ./vcpkg integrate install;
echo "${NAME} Install 'Google Tests' with vcpkg "
cd "/start/vcpkg"; ./vcpkg install gtest;
echo "${NAME} Install 'bshoshany-thread-pool' with vcpkg "
cd "/start/vcpkg"; ./vcpkg install bshoshany-thread-pool
echo "${NAME} Install 'bshoshany-thread-pool' with vcpkg "
cd "/start/vcpkg"; ./vcpkg install glog
echo "${NAME} Install 'TZ Date' with vcpkg "
cd /start/vcpkg; ./vcpkg install date
# use in CMake (Thread Pool: bshoshany-thread-pool)
#    find_path(BSHOSHANY_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")
#    target_include_directories(main PRIVATE ${BSHOSHANY_THREAD_POOL_INCLUDE_DIRS})
#
# use in CMake (Google logging : glog)
#    # this is heuristically generated, and may not be correct
#    find_package(glog CONFIG REQUIRED)
#    target_link_libraries(main PRIVATE glog::glog)
#
# use in CMake (Google Testing : gtest)
#    find_package(GTest CONFIG REQUIRED)
#    target_link_libraries(main PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)
#
# use in CMake (TZ date: date)
#    find_package(date CONFIG REQUIRED)
#    target_link_libraries(main PRIVATE date::date date::date-tz)

echo "${NAME} FINISHED "
