#include "pgbar/SpinBar.hpp"

int main()
{
  pgbar::SpinBar<> spibar;
  spibar.config()
    .prefix( "Working" )
    .lead( { ".", "..", "..." } )
    .quota( 2147483647 )
    .disable_all()
    .enable<pgbar::facade::SpinPlot, pgbar::facade::Speed, pgbar::facade::Elapsed, pgbar::facade::ETA>();
  for ( size_t i = 0; i < 2147483647; ++i )
    spibar.tick();
}
