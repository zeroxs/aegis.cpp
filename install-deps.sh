#!/usr/bin/env sh

set -e

echo "This install script only works with GCC at the moment. Support for other compilers may come later."

if [ -z $CXX ]; then echo "CXX not set. Defaulting to GCC."; CXX=gcc; else echo "CXX set to '$CXX'"; fi


currentver="$(${CXX} -dumpversion)"
requiredver="5.4.0"
if [ "$(printf '%s\n' "$requiredver" "$currentver" | sort -V | head -n1)" = "$requiredver" ]; then
  echo "GCC passed version check. (5.4.0+)"
else
  echo "GCC version too old. Please update to at least 5.4.0"
  exit 1
fi

if [ -x "$(command -v nproc)" ]; then
  core_count=`nproc`
else
  core_count=`grep -c ^processor /proc/cpuinfo`
fi

echo "Core count for make set to ${core_count}."

if [ ! \( -f "/usr/local/include/asio.hpp" -a -f "/usr/include/asio.hpp" \) ]; then
  echo "ASIO not found."
  echo "Configuring ASIO."
  cp -r lib/asio/asio/include/asi* /usr/local/include/
#  (cd lib/asio/asio && ./autogen.sh && ./configure && make install -j${core_count})
fi

if [ ! \( -f "/usr/local/include/nlohmann/json.hpp" -a -f "/usr/include/nlohmann/json.hpp" \) ]; then
  echo "JSON not found."
  echo "Configuring JSON."
  (cd lib/json && mkdir -p build && cd build && cmake -DJSON_MultipleHeaders=ON -DJSON_BuildTests=OFF .. && make install)
fi

if [ ! \( -f "/usr/local/include/spdlog/spdlog.h" -a -f "/usr/include/spdlog/spdlog.h" \) ]; then
  echo "Spdlog not found."
  echo "Configuring Spdlog."
  (cd lib/spdlog && mkdir -p build && cd build && cmake -DSPDLOG_BUILD_TESTS=OFF -DSPDLOG_BUILD_EXAMPLE=OFF -DSPDLOG_INSTALL=ON .. && make install)
fi

if [ ! \( -f "/usr/local/include/websocketpp/version.hpp" -a -f "/usr/include/websocketpp/version.hpp" \) ]; then
  echo "Websocket++ not found."
  echo "Configuring Websocket++."
  (cd lib/websocketpp && mkdir -p build  && cd build && cmake .. && make install)
fi

echo "Dependencies complete. Here is a simple line to build libaegis using default settings."
echo "mkdir -p build && cd build && CXX=${CXX} cmake .. && make -j${core_count} && sudo make install"

