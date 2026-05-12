#ifndef PACE_VERSION

# define PACE_MAJOR 1
# define PACE_MINOR 1
# define PACE_PATCH 0
# define PACE_STAGE "stable"

# define PACE__STR( x )                       #x
# define PACE__GEN_VER( major, minor, patch ) PACE__STR( major ) "." PACE__STR( minor ) "." PACE__STR( patch )

# define PACE_VERSION PACE__GEN_VER( PACE_MAJOR, PACE_MINOR, PACE_PATCH )

#endif
