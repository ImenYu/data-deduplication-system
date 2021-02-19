cmake_minimum_required(VERSION 3.0.0)
project(deduplication VERSION 0.1.0)

include(CTest)
enable_testing()

link_libraries(gcrypt)
aux_source_directory(./src DIR_SRCS)
aux_source_directory(./src/key_value_storage DIR_SRC_KEY_VALUE_STORAGE)
add_executable(deduplication ${DIR_SRCS} ${DIR_SRC_KEY_VALUE_STORAGE})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
