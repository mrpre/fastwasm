project(fastwasm)

add_definitions("-Wall -g -Werror")
aux_source_directory(./bin DIRSRCS)
include_directories(./src/exec)
include_directories(./src/core)
include_directories(./src/module)
include_directories(./src/jit)
include_directories(./src/instr)
add_subdirectory(src)
add_executable(run ${DIRSRCS})
target_link_libraries(run module exec jit instr core)

