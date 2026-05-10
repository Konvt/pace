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
      "tick/sec",
      "k tick/sec",
      "M tick/sec",
      "G tick/sec",
    } )
    .magnitude( 1000 )
    .info_color( 0x39c5bb )
    .colored( true )
    .font_bold( true )
    .font_underline( true )
    .enable_all();

  for ( auto _ : another.iterate( 100000 ) ) {}
}
