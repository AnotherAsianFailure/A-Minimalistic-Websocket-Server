project(websocketC)

set(CMAKE_C_STANDARD 11)

add_executable(wsserver
    sha1.c
    base64.c
    wsserver.c
)

target_compile_options(wsserver PRIVATE -Wall -Wextra -Wpedantic)
