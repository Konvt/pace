// This file is only used to trigger CMake to generate compile_commands.json.
#include "pace/pace.hpp"
#include <random>
#include <thread>
using namespace std;

int main()
{
  mt19937 rd { random_device {}() };
  {
    pace::iterate<pace::config::Block>(
      10000,
      [&]( int ) {
        this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<int>( 1, 1200 )( rd ) ) );
      },
      []( pace::BlockBar<>& self ) { self.config().filler_forecolor( pace::Color::Green ); },
      pace::option::Lead( { " ", "▁", "▂", "▃", "▄", "▅", "▆", "▇" } ),
      pace::option::InfoForecolor( "#FFD200" ) );
  }
  {
    for ( auto _ : pace::iterate<pace::config::Block>(
            10000,
            []( pace::BlockBar<>& self ) { self.config().filler_forecolor( pace::Color::Green ); },
            pace::option::Lead( { " ", " ", "░", "░", "▒", "▒", "▓", "▓" } ),
            pace::option::InfoForecolor( "#00BBFF" ) ) ) {
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<int>( 1, 1200 )( rd ) ) );
    }
  }
}
