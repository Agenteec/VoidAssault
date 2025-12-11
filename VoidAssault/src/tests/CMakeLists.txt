
    add_executable(test_net
        test_net.cpp
    )
        target_include_directories(test_net PUBLIC ${FIX_EXTERNAL_INCLUDE_DIR})

    target_link_libraries(test_net PRIVATE enet::enet_shared GameCommon)