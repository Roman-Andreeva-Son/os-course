add_executable(
    vtsh
    main.c
)

target_include_directories(
    vtsh
    PUBLIC
    .
)

target_link_libraries(
    vtsh
    PUBLIC
    libvtsh
)

# <<< добавил это, чтобы бинарь лежал в build/bin >>>
set_target_properties(vtsh PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)