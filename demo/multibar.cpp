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

  auto top = pace::views::auto_finish( mbar.at<0>(),
                                       // Bind a callback to mark that the current bar has been compeleted.
                                       [&]( typename tuple_element<0, decltype( mbar )>::type& self ) {
                                         // After C++14, `auto& self` is more recommended.
                                         self.config().filler_forecolor( pace::Color::Green ).lead( "" );
                                       } );
  top->config()
    .quota( ( tuple_size<decltype( mbar )>::value - 1 ) * 2 )
    .disable<pace::facade::Counter, pace::facade::Speed, pace::facade::Elapsed, pace::facade::ETA>();

  vector<thread> pool;
  pool.emplace_back( [&]() {
    mt19937 rd { random_device {}() };
    top.tick();
    mbar.at<1>().config().quota( 30000 );
    for ( size_t i = 0; i < 30000; ++i ) {
      mbar.at<1>().tick();
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<>( 1, 1025 )( rd ) ) );
    }
    mbar.at<1>().finish();
    top.tick();
  } );
  pool.emplace_back( [&]() {
    mt19937 rd { random_device {}() };
    top.tick();
    pace::views::auto_finish( mbar.at<2>(), [&]() { top.tick(); } ).iterate( 10000, [&rd]( int ) {
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<>( 10, 1100 )( rd ) ) );
    } );
  } );
  pool.emplace_back( [&]() {
    mt19937 rd { random_device {}() };
    top.tick();
    auto bar = pace::views::auto_finish( mbar.at<3>(), [&]() { top.tick(); } );
    for ( auto _ : bar.iterate( 80000 ) )
      this_thread::sleep_for( chrono::microseconds( uniform_int_distribution<>( 1, 1005 )( rd ) ) );
  } );

  for ( auto& td : pool ) {
    if ( td.joinable() )
      td.join();
  }
}
