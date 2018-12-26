cmake_minimum_required (VERSION 2.8)

AUX_SOURCE_DIRECTORY( ./ DIR_SRCS)

add_library( rtmp_wrapper STATIC ${DIR_SRCS} )
