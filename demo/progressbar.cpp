#include "pgbar/ProgressBar.hpp"

int main()
{
  pgbar::ProgressBar<> pbar;
  pbar.config().enable_all().quota( 2147483647 );
  for ( size_t i = 0; i < 2147483647; ++i )
    pbar.tick();
}
