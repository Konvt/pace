#include "pace/ProgressBar.hpp"
#include <chrono>
#include <iostream>
using namespace std;

int main()
{
  constexpr auto iteration = 2147483647;

  pace::ProgressBar<> pbar;
  pbar.config().with( pace::option::Prefix( u8"にほんご" ),
                      pace::option::Starting( u8"🔥 " ),
                      pace::option::Ending( u8" ✅" ),
                      pace::option::Lead( u8"🚀" ),
                      pace::option::Filler( u8"急" ),
                      pace::option::Postfix( u8"한국어" ),
                      pace::option::Quota( iteration ) );

  auto start = chrono::high_resolution_clock::now();
  for ( size_t i = 0; i < iteration; ++i )
    pbar.tick();
  auto duration = chrono::duration_cast<chrono::microseconds>( chrono::high_resolution_clock::now() - start );
  cout << "Average time per iteration: " << ( duration.count() / static_cast<double>( iteration ) ) << " us\n"
       << flush;
}
