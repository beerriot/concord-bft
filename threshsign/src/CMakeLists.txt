set(common_source_files
    XAssert.cpp
	Utils.cpp
	Timer.cpp
	Log.cpp
    IPublicKey.cpp
    ISecretKey.cpp
    IThresholdVerifier.cpp
    VectorOfShares.cpp
)

add_library(common OBJECT ${common_source_files})

target_include_directories(common
   PRIVATE ../include .)

add_subdirectory(bls)
