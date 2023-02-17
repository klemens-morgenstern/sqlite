
@ECHO ON
setlocal enabledelayedexpansion

if "%DRONE_JOB_BUILDTYPE%" == "boost" (

echo "============> INSTALL"

SET DRONE_BUILD_DIR=%CD: =%
choco install --no-progress -y sqltie --x64 --version 1.1.1.1000
SET BOOST_BRANCH=develop
IF "%DRONE_BRANCH%" == "master" SET BOOST_BRANCH=master
cp tools\user-config.jam %USERPROFILE%\user-config.jam
cd ..
SET GET_BOOST=!DRONE_BUILD_DIR!\tools\get-boost.sh
bash -c "$GET_BOOST $DRONE_BRANCH $DRONE_BUILD_DIR"
cd boost-root
call bootstrap.bat
b2 headers

echo "============> SCRIPT"

IF DEFINED DEFINE SET B2_DEFINE="define=%DEFINE%"

echo "Running tests"
b2 --debug-configuration variant=%VARIANT% cxxstd=%CXXSTD% %B2_DEFINE% address-model=%ADDRESS_MODEL% toolset=%TOOLSET% --verbose-test libs/sqlite/test -j3
if !errorlevel! neq 0 exit /b !errorlevel!

echo "Running libs/sqlite/example"
b2 --debug-configuration variant=%VARIANT% cxxstd=%CXXSTD% %B2_DEFINE% address-model=%ADDRESS_MODEL% toolset=%TOOLSET% libs/sqlite/example -j3
if !errorlevel! neq 0 exit /b !errorlevel!

echo "============> COMPLETED"

)