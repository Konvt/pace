#include "pace/MultiBar.hpp"
#include "pace/BlockBar.hpp"
#include "pace/FlowBar.hpp"
#include <chrono>
#include <random>
#include <thread>
using namespace std;

int main()
{
  auto mbar =
    pace::make_multi( pace::config::Flow( pace::option::Except<>(),
                                          pace::option::Filler( "━" ),
                                          pace::option::FillerForecolor( pace::Color::Red ),
                                          pace::option::Lead( "━━" ),
                                          pace::option::LeadForecolor( pace::Color::White ),
                                          pace::option::InfoForecolor( "#F5B0B6" ),
                                          pace::option::Starting(),
                                          pace::option::Ending() ),
                      pace::config::Block( pace::option::Lead( { " ", "▁", "▂", "▃", "▄", "▅", "▆", "▇" } ),
                                           pace::option::InfoForecolor( "#F7A699" ) ),
                      pace::config::Block( pace::option::Filler( "⠿" ),
                                           pace::option::Lead( { " ", "⠄", "⠆", "⠇", "⠧", "⠷" } ),
                                           pace::option::InfoForecolor( "#7DD4DF" ) ),
                      pace::config::Block( pace::option::Lead( { " ", "▖", "▞", "▛" } ),
                                           pace::option::InfoForecolor( "#8AB7EB" ) ) );
  mbar.at<0>().config().quota( ( tuple_size<decltype( mbar )>::value - 1 ) * 2 );
  // Bind a callback that will be executed at the end of the iteration below.
  mbar.at<0>() |=
    [&]( pace::FlowBar<>& self ) { self.config().filler_forecolor( pace::Color::Green ).lead( "" ); };
  // Bind a callback to mark that the current bar has been compeleted.
  mbar.at<1>() |= [&]() { mbar.at<0>().tick(); };
  mbar.at<2>() |= [&]() { mbar.at<0>().tick(); };
  mbar.at<3>() |= [&]() { mbar.at<0>().tick(); };

  vector<thread> pool;
  pool.emplace_back( [&]() {
    mt19937 rd { random_device {}() };
    mbar.at<1>().config().quota( 30000 );
    mbar.at<0>().tick();
    for ( size_t i = 0; i < 30000; ++i ) {
      mbar.at<1>().tick();
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<>( 1, 1025 )( rd ) ) );
    }
  } );
  pool.emplace_back( [&]() {
    mt19937 rd { random_device {}() };
    mbar.at<0>().tick();
    mbar.at<2>().iterate( 10000, [&rd]( int ) {
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<>( 10, 1100 )( rd ) ) );
    } );
  } );
  pool.emplace_back( [&]() {
    mt19937 rd { random_device {}() };
    mbar.at<0>().tick();
    for ( auto _ : mbar.at<3>().iterate( 80000 ) )
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<>( 1, 1005 )( rd ) ) );
  } );

  for ( auto& td : pool ) {
    if ( td.joinable() )
      td.join();
  }
}
