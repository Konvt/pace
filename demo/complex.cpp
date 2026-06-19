#include "pace/BlockBar.hpp"
#include "pace/MultiBar.hpp"
#include "pace/ProgressBar.hpp"
#include "pace/SweepBar.hpp"
#include <chrono>
#include <random>
#include <thread>
#include <vector>
using namespace std;

int main()
{
  auto bar =
    pace::make_multi( pace::config::Line( pace::option::Prefix( "Eating something...." ),
                                          pace::option::Filler( "⠇" ),
                                          pace::option::Lead( { "⠈", "⠐", "⠠", "⢀", "⡀", "⠄", "⠂", "⠁" } ),
                                          pace::option::Shift( 1 ),
                                          pace::option::InfoForecolor( "#7D7" ) ),
                      pace::config::Block( pace::option::Prefix( "Picking something..." ),
                                           pace::option::Filler( "⠿" ),
                                           pace::option::Lead( { " ", "⠄", "⠆", "⠇", "⠧", "⠷" } ),
                                           pace::option::InfoForecolor( "#7BD" ) ),
                      pace::config::Sweep( pace::option::Prefix( "Doing something....." ),
                                           pace::option::Filler( "." ),
                                           pace::option::Lead( "·" ),
                                           pace::option::InfoForecolor( "#26B4EB" ) ) );

  vector<thread> pool;
  pool.emplace_back( [&]() {
    bool flag = true;
    // Bind a callback executed at the end of the iteration below.
    bar.at<0>() |= [&]( pace::ProgressBar<>& self ) {
      if ( flag )
        self.config().prefix( "✔ Mission Accomplished" ).prefix_forecolor( pace::Color::Green );
      else
        self.config().prefix( "❌ Mission failed" ).prefix_forecolor( pace::Color::Red );
    };

    mt19937 rd { random_device {}() };
    bar.at<0>().iterate( 50000, [&rd]( int ) {
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<int>( 1, 1025 )( rd ) ) );
    } );
  } );
  pool.emplace_back( [&]() {
    bool flag = true;
    bar.at<1>() |= [&]( pace::BlockBar<>& self ) {
      if ( flag )
        self.config().prefix( "✔ Mission Accomplished" ).prefix_forecolor( pace::Color::Green );
      else
        self.config().prefix( "❌ Mission failed" ).prefix_forecolor( pace::Color::Red );
    };

    mt19937 rd { random_device {}() };
    bar.at<1>().config().quota( 10000 );
    const size_t terminate_val = 5000 + uniform_int_distribution<int>( 10, 1000 )( rd );
    for ( size_t i = 0; i < terminate_val; ++i ) {
      bar.at<1>().tick();
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<int>( 1, 1105 )( rd ) ) );
    }
    flag = false;
    bar.at<1>().reset();
  } );
  bar.at<2>().tick();

  for ( auto& td : pool )
    td.join();
  bar.at<2>().reset();
}
