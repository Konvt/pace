# parse_version(<out_version_var> <out_channel_var>)
function(parse_version OUT_VERSION OUT_CHANNEL)
  set(VERSION_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/pace/details/core/Version.hpp")
  file(READ ${VERSION_HEADER_PATH} _content)

  # major
  string(REGEX MATCH "#[ \t]*define[ \t]+PACE_MAJOR[ \t]+([A-Za-z0-9_]+)" _match "${_content}")
  set(_major "${CMAKE_MATCH_1}")
  # minor
  string(REGEX MATCH "#[ \t]*define[ \t]+PACE_MINOR[ \t]+([A-Za-z0-9_]+)" _match "${_content}")
  set(_minor "${CMAKE_MATCH_1}")
  # patch
  string(REGEX MATCH "#[ \t]*define[ \t]+PACE_PATCH[ \t]+([A-Za-z0-9_]+)" _match "${_content}")
  set(_patch "${CMAKE_MATCH_1}")
  # channel
  string(REGEX MATCH "#[ \t]*define[ \t]+PACE_STAGE[ \t]+\"([^\"]+)\"" _match "${_content}")

  set(${OUT_CHANNEL} "${CMAKE_MATCH_1}" PARENT_SCOPE)
  set(${OUT_VERSION} "${_major}.${_minor}.${_patch}" PARENT_SCOPE)
endfunction()
