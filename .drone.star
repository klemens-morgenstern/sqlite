# Use, modification, and distribution are
# subject to the Boost Software License, Version 1.0. (See accompanying
# file LICENSE.txt)
#
# Copyright Rene Rivera 2020.

# For Drone CI we use the Starlark scripting language to reduce duplication.
# As the yaml syntax for Drone CI is rather limited.
#
#
globalenv={'B2_CI_VERSION': '1', 'B2_VARIANT': 'release'}
linuxglobalimage="cppalliance/droneubuntu1804:1"
windowsglobalimage="cppalliance/dronevs2019"

def main(ctx):
  return [
  linux_cxx("docs", "", packages="docbook docbook-xml docbook-xsl xsltproc libsaxonhe-java default-jre-headless flex libfl-dev bison unzip rsync mlocate", image="cppalliance/droneubuntu1804:1", buildtype="docs", buildscript="drone", environment={ "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al", "COMMENT": "docs"}, globalenv=globalenv),
  linux_cxx("asan",         "g++-8",  packages="g++-8",  buildtype="boost", buildscript="drone", image=linuxglobalimage,                        environment={ "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al", 'COMMENT': 'asan',  'B2_VARIANT': 'debug', 'B2_TOOLSET': 'gcc-8',  'B2_CXXSTD': '14', 'B2_ASAN':  '1', 'B2_DEFINES': 'BOOST_NO_STRESS_TEST=1', 'DRONE_EXTRA_PRIVILEGED': 'True', 'DRONE_JOB_UUID': '356a192b79'}, globalenv=globalenv, privileged=True),
  linux_cxx("ubsan",        "g++-8",  packages="g++-8",  buildtype="boost", buildscript="drone", image=linuxglobalimage,                        environment={ "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al", 'COMMENT': 'ubsan', 'B2_VARIANT': 'debug', 'B2_TOOLSET': 'gcc-8',  'B2_CXXSTD': '14', 'B2_UBSAN': '1', 'B2_DEFINES': 'BOOST_NO_STRESS_TEST=1', 'B2_LINKFLAGS': '-fuse-ld=gold',  'DRONE_JOB_UUID': '77de68daec'}, globalenv=globalenv),
  linux_cxx("gcc 11 arm64", "g++-11", packages="g++-11", buildtype="boost", buildscript="drone", image="cppalliance/droneubuntu2004:multiarch", environment={ "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al",                                            'B2_TOOLSET': 'gcc-11', 'B2_CXXSTD': '14',                                                                                            'DRONE_JOB_UUID': '17ba079169m'}, arch="arm64", globalenv=globalenv),
  linux_cxx("GCC 10, Debug + Coverage", "g++-10", packages="g++-10 sqlite3 libffi-dev binutils-gold gdb mlocate", 
                                                                                                 image="cppalliance/droneubuntu2004:1", buildtype="boost", buildscript="drone", environment={ "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al", "GCOV": "gcov-10", "LCOV_VERSION": "1.15", "VARIANT": "process_coverage", "TOOLSET": "gcc", "COMPILER": "g++-10", "CXXSTD": "14", "DRONE_BEFORE_INSTALL" : "process_coverage", "CODECOV_TOKEN": {"from_secret": "codecov_token"}}, globalenv=globalenv, privileged=True),
  # A set of jobs based on the earlier .travis.yml configuration:
  linux_cxx("Default clang++ with libc++", "clang++-libc++", packages="libc++-dev mlocate", image="cppalliance/droneubuntu1604:1", buildtype="buildtype", buildscript="drone", environment={  "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al", "B2_TOOLSET": "clang-7", "B2_CXXSTD": "14", "VARIANT": "debug", "TOOLSET": "clang", "COMPILER": "clang++-libc++", "CXXSTD": "14", "CXX_FLAGS": "<cxxflags>-stdlib=libc++ <linkflags>-stdlib=libc++", "TRAVISCLANG" : "yes" }, globalenv=globalenv),
  linux_cxx("Default g++", "g++", packages="mlocate", image="cppalliance/droneubuntu1604:1", buildtype="buildtype", buildscript="drone", environment={ "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al",  "VARIANT": "release", "TOOLSET": "gcc", "COMPILER": "g++", "CXXSTD": "14" }, globalenv=globalenv),
  linux_cxx("Clang 3.8, UBasan", "clang++-3.8", packages="clang-3.8 sqlite3 mlocate", llvm_os="precise", llvm_ver="3.8", image="cppalliance/droneubuntu1604:1", buildtype="boost", buildscript="drone", environment={ "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al", "VARIANT": "process_ubasan", "TOOLSET": "clang", "COMPILER": "clang++-3.8", "CXXSTD": "14", "UBSAN_OPTIONS": 'print_stacktrace=1', "DRONE_BEFORE_INSTALL": "UBasan" }, globalenv=globalenv),
  linux_cxx("gcc 6", "g++-6", packages="g++-6", buildtype="boost", buildscript="drone", image=linuxglobalimage, environment={ "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al", 'B2_TOOLSET': 'gcc-6', 'B2_CXXSTD': '14', 'DRONE_JOB_UUID': '902ba3cda1'}, globalenv=globalenv),
  linux_cxx("clang 3.8", "clang++-3.8", packages="clang-3.8", buildtype="boost", buildscript="drone", image="cppalliance/droneubuntu1604:1", environment={ "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al", 'B2_TOOLSET': 'clang', 'COMPILER': 'clang++-3.8', 'B2_CXXSTD': '14', 'DRONE_JOB_UUID': '7b52009b64'}, globalenv=globalenv),
  osx_cxx("clang", "g++", packages="", buildtype="boost",         buildscript="drone", environment={ "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al", 'B2_TOOLSET': 'clang', 'B2_CXXSTD': '14,17', 'DRONE_JOB_UUID': '91032ad7bb'}, globalenv=globalenv),
  linux_cxx("coverity", "g++", packages="", buildtype="coverity", buildscript="drone", image=linuxglobalimage, environment={ "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al", 'COMMENT': 'Coverity Scan', 'B2_TOOLSET': 'clang', 'DRONE_JOB_UUID': '472b07b9fc'}, globalenv=globalenv),
  windows_cxx("msvc-14.3", "", image="cppalliance/dronevs2022:1", buildtype="boost", buildscript="drone", environment={ "BOOST_REQUEST_HTTPBIN": "httpbin.cpp.al", "VARIANT": "release", "TOOLSET": "msvc-14.3", "CXXSTD": "17", "ADDRESS_MODEL": "64"}),
    ]

# from https://github.com/boostorg/boost-ci
load("@boost_ci//ci/drone/:functions.star", "linux_cxx","windows_cxx","osx_cxx","freebsd_cxx")
