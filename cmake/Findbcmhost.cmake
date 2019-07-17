find_library(BCMHOST_LIBRARY
	NAMES
	bcm_host
	PATHS
	"/opt/vc"
	PATH_SUFFIXES
	"lib"
)

find_library(MMAL_LIBRARY
	NAMES
	mmal
	PATHS
	"/opt/vc"
	PATH_SUFFIXES
	"lib"
)

find_library(MMAL_COMPONENTS_LIBRARY
	NAMES
	mmal_components
	PATHS
	"/opt/vc"
	PATH_SUFFIXES
	"lib"
)

find_library(MMAL_CORE_LIBRARY
	NAMES
	mmal_core
	PATHS
	"/opt/vc"
	PATH_SUFFIXES
	"lib"
)

find_library(MMAL_UTIL_LIBRARY
	NAMES
	mmal_util
	PATHS
	"/opt/vc"
	PATH_SUFFIXES
	"lib"
)

find_library(VCOS_LIBRARY
	NAMES
	vcos
	PATHS
	"/opt/vc"
	PATH_SUFFIXES
	"lib"
)

find_library(VCHIQ_LIBRARY
	NAMES
	vchiq_arm
	PATHS
	"/opt/vc"
	PATH_SUFFIXES
	"lib"
)

find_path(BCMHOST_INCLUDE_DIR
	NAMES
	bcm_host.h
	PATHS
	"/opt/vc"
	PATH_SUFFIXES
	"include"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BCMHOST
	DEFAULT_MSG
	BCMHOST_LIBRARY
	BCMHOST_INCLUDE_DIR
)
