cmake_minimum_required(VERSION 3.16)
project(ARClothTryOn VERSION 1.0 LANGUAGES C CXX)

# Android ABI selection
set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
# set(CMAKE_ANDROID_ARCH_ABI x86_64)
set(CMAKE_ANDROID_STL_TYPE c++_shared)

# Qt build settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)

# Find Qt packages - INCLUDING SSL SUPPORT
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Quick
    QuickControls2
    Multimedia
    Qml
    Network
    Quick3D
    DataVisualization
    3DCore
    3DRender
    3DInput
    3DExtras
    OpenGL
    ShaderTools
    Svg
)

# Add Qt resources
qt_add_resources(QRC_FILES
    resources.qrc
)

# Define executable
qt_add_executable(ARClothTryOn
    src/main.cpp
    src/QMLManager.cpp
    src/QMLManager.h
    src/BodyTracker.cpp
    src/BodyTracker.h
    src/ClothFitter.cpp
    src/ClothFitter.h
    src/ClothScanner.cpp
    src/ClothScanner.h
    src/ImageConverter.cpp
    src/NetworkManager.cpp
    src/NetworkManager.h
    resources.qrc
    ${QRC_FILES}
)

qt_policy(SET QTP0004 NEW)
qt_policy(SET QTP0002 NEW)

# Add QML module
qt_add_qml_module(ARClothTryOn
    URI ARClothTryOn
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/MainPage.qml
        qml/CameraPage.qml
        qml/ScanResultPage.qml
        qml/GarmentSelectionPage.qml
        qml/GarmentPreviewPage.qml
        qml/AuthorizationPage.qml
        qml/Style.qml
    RESOURCES
        qml/qmldir
    RESOURCE_PREFIX /
    SOURCES src/NetworkManager.cpp
    SOURCES src/NetworkManager.h
)

# Include directories
target_include_directories(ARClothTryOn PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries - WITH SSL SUPPORT
target_link_libraries(ARClothTryOn PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::Qml
    Qt6::Network
    Qt6::CorePrivate
    Qt6::Quick3D
    Qt6::DataVisualization
    Qt6::3DCore
    Qt6::3DRender
    Qt6::3DInput
    Qt6::3DExtras
    Qt6::OpenGL
    Qt6::ShaderTools
)

# Android configuration
if(ANDROID)
    # Set Android package source directory
    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")

    # Method 1: REMOVED automatic SSL deployment due to Qt SSL not found
    # set_target_properties(ARClothTryOn PROPERTIES
    #     QT_ANDROID_DEPLOYMENT_DEPENDENCIES "ssl"
    # )

    # Method 2: Manual SSL library inclusion
    # Path to OpenSSL libraries in your project structure
    # The CMakeLists.txt is in the "client" directory, so android/libs is correct
    set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android/libs")

    # Set OpenSSL library paths for arm64-v8a
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/arm64-v8a/libcrypto_3.so")
        set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/arm64-v8a/libssl_3.so")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/x86_64/libcrypto_3.so")
        set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/x86_64/libssl_3.so")
    endif()

    # Debug: Print library paths during configuration
    message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS "OPENSSL_ROOT_DIR: ${OPENSSL_ROOT_DIR}")
    message(STATUS "Looking for SSL library at: ${OPENSSL_SSL_LIBRARY}")
    message(STATUS "Looking for Crypto library at: ${OPENSSL_CRYPTO_LIBRARY}")

    # Verify that the SSL libraries exist
    if(EXISTS "${OPENSSL_SSL_LIBRARY}")
        message(STATUS "✓ SSL library found")
    else()
        message(WARNING "✗ SSL library not found at: ${OPENSSL_SSL_LIBRARY}")
    endif()

    if(EXISTS "${OPENSSL_CRYPTO_LIBRARY}")
        message(STATUS "✓ Crypto library found")
    else()
        message(WARNING "✗ Crypto library not found at: ${OPENSSL_CRYPTO_LIBRARY}")
    endif()

    # Include OpenSSL libraries in deployment
    set(ANDROID_EXTRA_LIBS "")

    # Add C++ shared library (correct path for newer NDK versions)
    if(CMAKE_ANDROID_NDK)
        # Try multiple possible locations for libc++_shared.so
        set(CPP_SHARED_LIB_PATHS
            "${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so"
            "${CMAKE_ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libs/${CMAKE_ANDROID_ARCH_ABI}/libc++_shared.so"
            "${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so"
        )

        foreach(LIB_PATH ${CPP_SHARED_LIB_PATHS})
            if(EXISTS "${LIB_PATH}")
                list(APPEND ANDROID_EXTRA_LIBS "${LIB_PATH}")
                message(STATUS "Found libc++_shared.so at: ${LIB_PATH}")
                break()
            endif()
        endforeach()
    endif()

    # Add SSL libraries if they exist
    if(EXISTS "${OPENSSL_SSL_LIBRARY}" AND EXISTS "${OPENSSL_CRYPTO_LIBRARY}")
        list(APPEND ANDROID_EXTRA_LIBS
            ${OPENSSL_CRYPTO_LIBRARY}
            ${OPENSSL_SSL_LIBRARY}
        )
        message(STATUS "Added SSL libraries to ANDROID_EXTRA_LIBS")
    endif()

    # Debug: Print all extra libs being included
    message(STATUS "Final ANDROID_EXTRA_LIBS:")
    foreach(LIB ${ANDROID_EXTRA_LIBS})
        message(STATUS "  - ${LIB}")
    endforeach()

    # Update deployment rules WITH SSL dependencies
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/android/deployment-rules.cmake")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/android/deployment-rules.cmake"
            "# Auto-generated deployment rules with HTTPS/SSL support\n"
            "set(QT_ANDROID_DEPLOYMENT_DEPENDENCIES\n"
            "    # Core libraries\n"
            "    libQt6Core.so\n"
            "    libQt6Gui.so\n"
            "    libQt6Network.so\n"
            "    libQt6QmlModels.so\n"
            "    libQt6Qml.so\n"
            "    libQt6Quick.so\n"
            "    libQt6QuickControls2.so\n"
            "    libQt6Multimedia.so\n"
            "    # 3D rendering\n"
            "    libQt63DCore.so\n"
            "    libQt63DRender.so\n"
            "    libQt63DInput.so\n"
            "    libQt63DExtras.so\n"
            "    libQt6OpenGL.so\n"
            "    libQt6ShaderTools.so\n"
            "    # Android extras\n"
            "    libqtandroid.so\n"
            "    # Network support with SSL\n"
            "    libQt6Network.so\n"
            "    # OpenSSL libraries for HTTPS\n"
            "    libssl_3.so\n"
            "    libcrypto_3.so\n"
            ")\n"
        )
    endif()

    # Define JNI libs search path
    set(ANDROID_JNI_LIBS_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/android/libs/${CMAKE_ANDROID_ARCH_ABI})

    # Set Android deployment settings
    set_target_properties(ARClothTryOn PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_EXTRA_LIBS "${ANDROID_EXTRA_LIBS}"
    )

    # Enable SSL support without Qt's auto-deployment
    target_compile_definitions(ARClothTryOn PRIVATE QT_NETWORK_SSL)
endif()

# Install target
install(TARGETS ARClothTryOn
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
