#include "pace/MultiBar.hpp"
#include "pace/BlockBar.hpp"
#include "pace/FlowBar.hpp"
#include <chrono>
#include <random>
#include <thread>
using namespace std;

int main()
{
  auto mbar = pace::make_multi(
    pace::config::Flow( pace::option::Except<>(),
                         pace::option::Filler( "━" ),
                         pace::option::FillerColor( pace::Color::Red ),
                         pace::option::Lead( "━━" ),
                         pace::option::LeadColor( pace::Color::White ),
                         pace::option::InfoColor( "#F5B0B6" ),
                         pace::option::Starting(),
                         pace::option::Ending() ),
    pace::config::Block( pace::option::Lead( { " ", "▁", "▂", "▃", "▄", "▅", "▆", "▇" } ),
                          pace::option::InfoColor( "#F7A699" ) ),
    pace::config::Block( pace::option::Filler( "⠿" ),
                          pace::option::Lead( { " ", "⠄", "⠆", "⠇", "⠧", "⠷" } ),
                          pace::option::InfoColor( "#7DD4DF" ) ),
    pace::config::Block( pace::option::Lead( { " ", "▖", "▞", "▛" } ),
                          pace::option::InfoColor( "#8AB7EB" ) ) );
  mbar.config<0>().quota( ( tuple_size<decltype( mbar )>::value - 1 ) * 2 );
  // Bind a callback that will be executed at the end of the iteration below.
  mbar.at<0>() |=
    [&]( pace::FlowBar<>& self ) { self.config().filler_color( pace::Color::Green ).lead( "" ); };
  // Bind a callback to mark that the current bar has been compeleted.
  mbar.at<1>() |= [&]() { mbar.tick<0>(); };
  mbar.at<2>() |= [&]() { mbar.tick<0>(); };
  mbar.at<3>() |= [&]() { mbar.tick<0>(); };

  vector<thread> pool;
  pool.emplace_back( [&]() {
    mt19937 rd { random_device {}() };
    mbar.config<1>().quota( 30000 );
    mbar.tick<0>();
    for ( size_t i = 0; i < 30000; ++i ) {
      mbar.tick<1>();
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<>( 1, 1025 )( rd ) ) );
    }
  } );
  pool.emplace_back( [&]() {
    mt19937 rd { random_device {}() };
    mbar.tick<0>();
    mbar.iterate<2>( 10000, [&rd]( int ) {
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<>( 10, 1100 )( rd ) ) );
    } );
  } );
  pool.emplace_back( [&]() {
    mt19937 rd { random_device {}() };
    mbar.tick<0>();
    for ( auto _ : mbar.iterate<3>( 80000 ) )
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<>( 1, 1005 )( rd ) ) );
  } );

  for ( auto& td : pool ) {
    if ( td.joinable() )
      td.join();
  }
  mbar.wait();
}
