# CMake generated Testfile for 
# Source directory: C:/Projects/CZIConvert/third_party/libczi/Src/libCZI_UnitTests
# Build directory: C:/Projects/CZIConvert/third_party/libczi/build/Src/libCZI_UnitTests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(libCZI_UnitTests "C:/Projects/CZIConvert/third_party/libczi/build/Src/libCZI_UnitTests/Debug/libCZI_UnitTests.exe")
  set_tests_properties(libCZI_UnitTests PROPERTIES  _BACKTRACE_TRIPLES "C:/Projects/CZIConvert/third_party/libczi/Src/libCZI_UnitTests/CMakeLists.txt;60;add_test;C:/Projects/CZIConvert/third_party/libczi/Src/libCZI_UnitTests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(libCZI_UnitTests "C:/Projects/CZIConvert/third_party/libczi/build/Src/libCZI_UnitTests/Release/libCZI_UnitTests.exe")
  set_tests_properties(libCZI_UnitTests PROPERTIES  _BACKTRACE_TRIPLES "C:/Projects/CZIConvert/third_party/libczi/Src/libCZI_UnitTests/CMakeLists.txt;60;add_test;C:/Projects/CZIConvert/third_party/libczi/Src/libCZI_UnitTests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(libCZI_UnitTests "C:/Projects/CZIConvert/third_party/libczi/build/Src/libCZI_UnitTests/MinSizeRel/libCZI_UnitTests.exe")
  set_tests_properties(libCZI_UnitTests PROPERTIES  _BACKTRACE_TRIPLES "C:/Projects/CZIConvert/third_party/libczi/Src/libCZI_UnitTests/CMakeLists.txt;60;add_test;C:/Projects/CZIConvert/third_party/libczi/Src/libCZI_UnitTests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(libCZI_UnitTests "C:/Projects/CZIConvert/third_party/libczi/build/Src/libCZI_UnitTests/RelWithDebInfo/libCZI_UnitTests.exe")
  set_tests_properties(libCZI_UnitTests PROPERTIES  _BACKTRACE_TRIPLES "C:/Projects/CZIConvert/third_party/libczi/Src/libCZI_UnitTests/CMakeLists.txt;60;add_test;C:/Projects/CZIConvert/third_party/libczi/Src/libCZI_UnitTests/CMakeLists.txt;0;")
else()
  add_test(libCZI_UnitTests NOT_AVAILABLE)
endif()
