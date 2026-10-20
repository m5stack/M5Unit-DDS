# ESP-IDF component registration for M5Unit-DDS.
# Upstream: https://github.com/m5stack/M5Unit-DDS
#
# Only the M5UnitUnified component (src/unit) is built. The legacy driver (src/Unit_DDS.*) depends on
# Arduino Wire, so it is Arduino-only and not part of this component.
# The public headers include M5UnitUnified only; M5Utility is used by the .cpp only.
file(GLOB_RECURSE SRCS "src/unit/*.cpp")
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "src"
    REQUIRES M5UnitUnified
    PRIV_REQUIRES M5Utility
)
