#include "pace/BlockBar.hpp"

int main()
{
  pace::BlockBar<> blckbar;
  blckbar.config().enable_all().quota( 2147483647 ).percent_decs( 0 );
  for ( size_t i = 0; i < 2147483647; ++i )
    blckbar.tick();
}
