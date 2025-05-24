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

# Find Qt packages - Core components for HTTP-only networking
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

# Link libraries - REMOVED SSL DEPENDENCIES FOR HTTP-ONLY
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

    # Update deployment rules WITHOUT SSL dependencies
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/android/deployment-rules.cmake")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/android/deployment-rules.cmake"
            "# Auto-generated deployment rules for HTTP-only networking\n"
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
            "    # Network support (HTTP only)\n"
            "    libQt6Network.so\n"
            ")\n"
        )
    endif()

    # Additional libraries needed for Android
    set(ANDROID_EXTRA_LIBS
        ${CMAKE_ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libs/${CMAKE_ANDROID_ARCH_ABI}/libc++_shared.so
    )

    # Define JNI libs search path
    set(ANDROID_JNI_LIBS_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/android/libs/${CMAKE_ANDROID_ARCH_ABI})

    # Add compile definition to disable SSL features
    target_compile_definitions(ARClothTryOn PRIVATE QT_NO_SSL)
endif()

# Install target
install(TARGETS ARClothTryOn
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
