include(ExternalProject)

ExternalProject_Add( external-link-c-sdk
        GIT_REPOSITORY https://github.com/pili-engineering/ipcam_sdk
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ipcam_sdk
        CONFIGURE_COMMAND ./autogen.sh &&
            ./configure
            --enable-static
            --disable-shared
            --enable-opensslextra
            --disable-debug
            --host=${TOOLCHAIN_PREFIX}
            --prefix=${CMAKE_CURRENT_SOURCE_DIR}/wolfssl/output/${TOOLCHAIN_TRIPLET}
        BUILD_COMMAND make
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND make install
)
