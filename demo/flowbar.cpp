#include "pace/FlowBar.hpp"
#include <chrono>
#include <thread>

int main()
{
  pace::FlowBar<> flwbar;
  flwbar.config().reverse( true ); // reverses the direction of animation.
  flwbar.tick();
  std::this_thread::sleep_for( std::chrono::seconds( 20 ) );
  flwbar.reset();
}
