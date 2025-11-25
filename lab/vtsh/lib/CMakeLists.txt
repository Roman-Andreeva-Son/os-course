add_library(
    libvtsh
    STATIC
    vtsh.c
    util.c
    parser.c
)

target_include_directories(
    libvtsh
    PUBLIC
    .
)
