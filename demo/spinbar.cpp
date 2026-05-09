#include "pace/SpinBar.hpp"

int main()
{
  pace::SpinBar<> spibar;
  spibar.config()
    .prefix( "Working" )
    .lead( { ".", "..", "..." } )
    .quota( 2147483647 )
    .disable_all()
    .enable<pace::facade::SpinPlot, pace::facade::Speed, pace::facade::Elapsed, pace::facade::ETA>();
  for ( size_t i = 0; i < 2147483647; ++i )
    spibar.tick();
}
