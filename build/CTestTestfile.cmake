# CMake generated Testfile for 
# Source directory: /home/ritesh/Documents/Code/C++/ThreadsafeQueue/TeamB/ThreadsafeQueueLib
# Build directory: /home/ritesh/Documents/Code/C++/ThreadsafeQueue/TeamB/ThreadsafeQueueLib/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(SPSC "/home/ritesh/Documents/Code/C++/ThreadsafeQueue/TeamB/ThreadsafeQueueLib/build/test_spsc")
set_tests_properties(SPSC PROPERTIES  _BACKTRACE_TRIPLES "/home/ritesh/Documents/Code/C++/ThreadsafeQueue/TeamB/ThreadsafeQueueLib/CMakeLists.txt;31;add_test;/home/ritesh/Documents/Code/C++/ThreadsafeQueue/TeamB/ThreadsafeQueueLib/CMakeLists.txt;0;")
add_test(MPSC "/home/ritesh/Documents/Code/C++/ThreadsafeQueue/TeamB/ThreadsafeQueueLib/build/test_mpsc")
set_tests_properties(MPSC PROPERTIES  _BACKTRACE_TRIPLES "/home/ritesh/Documents/Code/C++/ThreadsafeQueue/TeamB/ThreadsafeQueueLib/CMakeLists.txt;32;add_test;/home/ritesh/Documents/Code/C++/ThreadsafeQueue/TeamB/ThreadsafeQueueLib/CMakeLists.txt;0;")
add_test(MPMC "/home/ritesh/Documents/Code/C++/ThreadsafeQueue/TeamB/ThreadsafeQueueLib/build/test_mpmc")
set_tests_properties(MPMC PROPERTIES  _BACKTRACE_TRIPLES "/home/ritesh/Documents/Code/C++/ThreadsafeQueue/TeamB/ThreadsafeQueueLib/CMakeLists.txt;33;add_test;/home/ritesh/Documents/Code/C++/ThreadsafeQueue/TeamB/ThreadsafeQueueLib/CMakeLists.txt;0;")
