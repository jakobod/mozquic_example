# Try to find mozquic headers and libraries
#
# Use this module as follows:
#
#     find_package(Mozquic)
#
# Variables used by this module (they can change the default behavior and need
# to be set before calling find_package):
#
#  MOZQUIC_ROOT_DIR Set this variable either to an installation prefix or to
#                   the MOZQUIC root directory where to look for the library.
#
# Variables defined by this module:
#
#  MOZQUIC_FOUND        Found library and header
#  MOZQUIC_LIBRARY      Path to library
#  MOZQUIC_INCLUDE_DIR  Include path for headers
#

find_library(MOZQUIC_LIBRARY
        NAMES mozquic
        HINTS ${MOZQUIC_ROOT_DIR}
        )

find_path(MOZQUIC_INCLUDE_DIR
        NAMES "MozQuic.h"
        HINTS ${MOZQUIC_ROOT_DIR}
        )

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
        MOZQUIC
        DEFAULT_MSG
        MOZQUIC_LIBRARY
        MOZQUIC_INCLUDE_DIR
)

mark_as_advanced(
        MOZQUIC_ROOT_DIR
        MOZQUIC_LIBRARY
        MOZQUIC_INCLUDE_DIR
)