#include "pace/SweepBar.hpp"

int main()
{
  auto swpbar = pace::auto_finish<pace::SweepBar<>>();
  swpbar->config().enable_all().quota( 2147483647 );
  for ( size_t i = 0; i < 2147483647; ++i )
    swpbar.tick();
}
