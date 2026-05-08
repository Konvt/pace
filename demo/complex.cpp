#include "pgbar/BlockBar.hpp"
#include "pgbar/MultiBar.hpp"
#include "pgbar/ProgressBar.hpp"
#include "pgbar/SweepBar.hpp"
#include <chrono>
#include <random>
#include <thread>
#include <vector>
using namespace std;

int main()
{
  auto bar =
    pgbar::make_multi( pgbar::config::Line( pgbar::option::Prefix( "Eating something...." ),
                                            pgbar::option::Filler( "⠇" ),
                                            pgbar::option::Lead( { "⠈", "⠐", "⠠", "⢀", "⡀", "⠄", "⠂", "⠁" } ),
                                            pgbar::option::Shift( 1 ),
                                            pgbar::option::InfoColor( "#7D7" ) ),
                       pgbar::config::Block( pgbar::option::Prefix( "Picking something..." ),
                                             pgbar::option::Filler( "⠿" ),
                                             pgbar::option::Lead( { " ", "⠄", "⠆", "⠇", "⠧", "⠷" } ),
                                             pgbar::option::InfoColor( "#7BD" ) ),
                       pgbar::config::Sweep( pgbar::option::Prefix( "Doing something....." ),
                                             pgbar::option::Filler( "." ),
                                             pgbar::option::Lead( "·" ),
                                             pgbar::option::InfoColor( "#26B4EB" ) ) );

  vector<thread> pool;
  pool.emplace_back( [&]() {
    bool flag = true;
    // Bind a callback executed at the end of the iteration below.
    bar.at<0>() |= [&]( pgbar::ProgressBar<>& self ) {
      if ( flag )
        self.config().prefix( "✔ Mission Accomplished" ).prefix_color( pgbar::Color::Green );
      else
        self.config().prefix( "❌ Mission failed" ).prefix_color( pgbar::Color::Red );
    };

    mt19937 rd { random_device {}() };
    bar.iterate<0>( 50000, [&rd]( int ) {
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<int>( 1, 1025 )( rd ) ) );
    } );
  } );
  pool.emplace_back( [&]() {
    bool flag = true;
    bar.at<1>() |= [&]( pgbar::BlockBar<>& self ) {
      if ( flag )
        self.config().prefix( "✔ Mission Accomplished" ).prefix_color( pgbar::Color::Green );
      else
        self.config().prefix( "❌ Mission failed" ).prefix_color( pgbar::Color::Red );
    };

    mt19937 rd { random_device {}() };
    bar.config<1>().quota( 10000 );
    const size_t terminate_val = 5000 + uniform_int_distribution<int>( 10, 1000 )( rd );
    for ( size_t i = 0; i < terminate_val; ++i ) {
      bar.tick<1>();
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<int>( 1, 1105 )( rd ) ) );
    }
    flag = false;
    bar.reset<1>();
  } );
  bar.tick<2>();

  for ( auto& td : pool )
    td.join();
  bar.reset<2>();
  bar.wait();
}
