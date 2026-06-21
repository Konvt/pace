#include "pace/DynamicBar.hpp"
#include "pace/ProgressBar.hpp"
#include <chrono>
#include <thread>
#include <vector>
using namespace std;

int main()
{
  pace::DynamicBar<> dbar;

  auto /* std::unique_ptr<BarType> */ bar1 = dbar.insert<pace::ProgressBar<>>();
  auto bar2 =
    dbar.insert( pace::config::Line( pace::option::Prefix( "No.2" ), pace::option::Quota( 8000 ) ) );

  vector<thread> pool;
  pool.emplace_back( [&bar1]() {
    bar1->config().prefix( "No.1" ).quota( 1919 );
    this_thread::sleep_for( chrono::seconds( 3 ) );
    do {
      bar1->tick();
      this_thread::sleep_for( chrono::milliseconds( 5 ) );
    } while ( bar1->active() );
  } );
  pool.emplace_back( [&bar2]() {
    this_thread::sleep_for( chrono::seconds( 2 ) );
    do {
      bar2->tick();
      this_thread::sleep_for( chrono::microseconds( 900 ) );
    } while ( bar2->active() );
  } );
  pool.emplace_back( [&dbar]() {
    auto bar = dbar.insert<pace::config::Line>( pace::option::Prefix( "No.3" ), pace::option::Quota( 1000 ) );
    // Do some ticks, then reset the current bar before completing.
    for ( int i = 0; i < 500; ++i ) {
      bar->tick();
      this_thread::sleep_for( chrono::milliseconds( 5 ) );
    }
    bar->reset();

    // passing an empty argument to delete the color
    bar->config().info_forecolor( {} );
    // an empty string is also fine
    bar->config().info_forecolor( "" );

    // Restart a new iteration, and the "No.3" bar will reappear at the bottom of the terminal.
    for ( int i = 0; i < 400; ++i ) {
      bar->tick();
      this_thread::sleep_for( chrono::milliseconds( 5 ) );
    }
    // Finally let it be destructed during running; it's safe.
  } );

  for ( auto& td : pool )
    td.join();
}
