#include "pgbar/SweepBar.hpp"

int main()
{
  pgbar::SweepBar<> swpbar;
  swpbar.config().enable_all().quota( 2147483647 );
  for ( size_t i = 0; i < 2147483647; ++i )
    swpbar.tick();
}
