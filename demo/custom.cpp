#include "pace/pace.hpp"

int main()
{
  using AnotherConfig =
    pace::prefab::BasicConfig<pace::facade::Speed, pace::facade::Counter, pace::facade::Elapsed>;
  using AnotherBar = pace::prefab::BasicBar<AnotherConfig>;

  AnotherBar another;
  another.config()
    .divider( " | " )
    .speed_unit( {
      "ticks/sec",
      "k ticks/sec",
      "M ticks/sec",
      "G ticks/sec",
    } )
    .magnitude( 1000 )
    .info_forecolor( 0x39C5BB )
    .colored( true )
    .font_bold( true )
    .font_underline( true )
    .elapsed_format(u8"\u23F3 %:-3Ss")
    .enable_all()
    .quota( 2147483647 );

  for ( size_t _ = 0; _ < 2147483647; ++_ )
    another.tick();
}
