find_library(AVFORMAT_LIBRARY
	NAMES avformat
	PATH_SUFFIXES
	"lib"
)

find_library(AVCODEC_LIBRARY
	NAMES avcodec
	PATH_SUFFIXES
	"lib"
)

find_library(AVUTIL_LIBRARY
	NAMES avutil
	PATH_SUFFIXES
	"lib"
)

find_path(AVFORMAT_INCLUDE_DIR
	NAMES
	libavformat/avformat.h
	PATH_SUFFIXES
	"include"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVFORMAT
	DEFAULT_MSG
	AVFORMAT_LIBRARY
	AVFORMAT_INCLUDE_DIR
)

