# Include mesh stack headers for direct access layer messaging
zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)
zephyr_library_sources(
btmesh.c
cli.c
gateway.c
)
