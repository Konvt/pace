#include "pgbar/MultiBar.hpp"
#include "pgbar/BlockBar.hpp"
#include "pgbar/FlowBar.hpp"
#include <chrono>
#include <random>
#include <thread>
using namespace std;

int main()
{
  auto mbar = pgbar::make_multi(
    pgbar::config::Flow( pgbar::option::Except<>(),
                         pgbar::option::Filler( "━" ),
                         pgbar::option::FillerColor( pgbar::Color::Red ),
                         pgbar::option::Lead( "━━" ),
                         pgbar::option::LeadColor( pgbar::Color::White ),
                         pgbar::option::InfoColor( "#F5B0B6" ),
                         pgbar::option::Starting(),
                         pgbar::option::Ending() ),
    pgbar::config::Block( pgbar::option::Lead( { " ", "▁", "▂", "▃", "▄", "▅", "▆", "▇" } ),
                          pgbar::option::InfoColor( "#F7A699" ) ),
    pgbar::config::Block( pgbar::option::Filler( "⠿" ),
                          pgbar::option::Lead( { " ", "⠄", "⠆", "⠇", "⠧", "⠷" } ),
                          pgbar::option::InfoColor( "#7DD4DF" ) ),
    pgbar::config::Block( pgbar::option::Lead( { " ", "▖", "▞", "▛" } ),
                          pgbar::option::InfoColor( "#8AB7EB" ) ) );
  mbar.config<0>().quota( ( tuple_size<decltype( mbar )>::value - 1 ) * 2 );
  // Bind a callback that will be executed at the end of the iteration below.
  mbar.at<0>() |=
    [&]( pgbar::FlowBar<>& self ) { self.config().filler_color( pgbar::Color::Green ).lead( "" ); };
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
