set(BUILD_MLU ON CACHE BOOL "")
set(BUILD_CUDA NO CACHE BOOL "")
set(BUILD_GIT_VERSION YES CACHE BOOL "")
set(BUILD_TESTING ON CACHE BOOL "")
set(WITH_ONEDNN OFF CACHE BOOL "")
set(TREAT_WARNINGS_AS_ERRORS OFF CACHE BOOL "")
set(THIRD_PARTY_MIRROR aliyun CACHE STRING "")
set(PIP_INDEX_MIRROR "https://pypi.tuna.tsinghua.edu.cn/simple" CACHE STRING "")
set(CMAKE_BUILD_TYPE Release CACHE STRING "")
set(CMAKE_GENERATOR Ninja CACHE STRING "")
set(BUILD_CPP_API OFF CACHE BOOL "")
set(WITH_MLIR OFF CACHE BOOL "")
set(BUILD_FOR_CI ON CACHE BOOL "")
set(BUILD_SHARED_LIBS ON CACHE BOOL "")
set(CMAKE_C_COMPILER_LAUNCHER ccache CACHE STRING "")
set(CMAKE_CXX_COMPILER_LAUNCHER ccache CACHE STRING "")
