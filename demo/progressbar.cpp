#include "pace/ProgressBar.hpp"
#include <chrono>

int main()
{
  pace::ProgressBar<> pbar;
  pbar.config().enable_all().quota( 2147483647 );
  pbar.poke(); // activate the bar only
  std::this_thread::sleep_for( std::chrono::seconds( 6 ) );
  for ( size_t i = 0; i < 2147483647; ++i )
    pbar.tick();
}
