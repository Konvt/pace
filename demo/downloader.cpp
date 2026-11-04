#include "pace/ProgressBar.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
using namespace std;

struct Package {
  string name;
  string version;
  string repo;
  string arch;
  int size_kb;
};

struct InstallStep {
  string text;
  int delay_ms;
};

double random_speed_mbs( mt19937_64& rd )
{
  uniform_real_distribution<double> dist( 0.5, 5.0 );
  return dist( rd );
}

string human_size( int kb )
{
  if ( kb >= 1024 ) {
    ostringstream os;
    os << fixed << setprecision( 1 ) << ( kb / 1024.0 ) << " MB";
    return os.str();
  }
  return to_string( kb ) + " kB";
}

vector<Package> make_packages()
{
  return {
    { "libxml2-16",      "2.15.2+dfsg-0.1ubuntu0.3", "resolute-updates/main", "amd64", 607  },
    { "libuv1t64",       "1.51.0-2ubuntu1",          "resolute/main",         "amd64", 103  },
    { "cmake-data",      "4.2.3-2ubuntu2",           "resolute/main",         "all",   2581 },
    { "libarchive13t64", "3.8.5-1ubuntu2.3",         "resolute-updates/main", "amd64", 397  },
    { "libcurl4t64",     "8.18.0-1ubuntu2.8",        "resolute-updates/main", "amd64", 430  },
    { "libjsoncpp26",    "1.9.6-5",                  "resolute/main",         "amd64", 84   },
    { "librhash1",       "1.4.6-1.1",                "resolute/main",         "amd64", 133  },
    { "libssl3t64",      "3.5.2-1ubuntu3",           "resolute/main",         "amd64", 2210 },
    { "openssl",         "3.5.2-1ubuntu3",           "resolute/main",         "amd64", 1180 },
    { "gcc-14-base",     "14.2.0-19ubuntu2",         "resolute/main",         "amd64", 51   },
    { "libstdc++6",      "14.2.0-19ubuntu2",         "resolute/main",         "amd64", 685  },
  };
}

vector<InstallStep> make_install_steps( const vector<Package>& packages, mt19937_64& rd )
{
  vector<InstallStep> steps;
  steps.reserve( packages.size() * 3 + 3 );

  auto rnd = [&]( int lo, int hi ) { return uniform_int_distribution<int>( lo, hi )( rd ); };

  for ( const auto& pkg : packages ) {
    steps.push_back( { "Selecting previously unselected package " + pkg.name + ".", rnd( 60, 120 ) } );
    steps.push_back( { "Unpacking " + pkg.name + " (" + pkg.version + ") ...", rnd( 90, 180 ) } );
    steps.push_back( { "Setting up " + pkg.name + " (" + pkg.version + ") ...", rnd( 110, 240 ) } );
  }

  steps.push_back( { "Processing triggers for man-db (2.13.1-1) ...", 220 } );
  steps.push_back( { "Processing triggers for libc-bin (2.41-6ubuntu1) ...", 180 } );
  steps.push_back( { "Processing triggers for ca-certificates (20240203) ...", 160 } );

  return steps;
}

void print_apt_preamble( const vector<Package>& packages )
{
  int total_kb = 0;
  for ( const auto& p : packages )
    total_kb += p.size_kb;

  cout << "Reading package lists... Done\n"
       << "Building dependency tree... Done\n"
       << "Reading state information... Done\n"
       << "The following NEW packages will be installed:\n  ";

  for ( size_t i = 0; i < packages.size(); ++i ) {
    cout << packages[i].name;
    if ( i + 1 < packages.size() )
      cout << ( ( i % 4 == 3 ) ? "\n  " : " " );
  }

  cout << "\n0 upgraded, " << packages.size() << " newly installed, 0 to remove and 0 not upgraded.\n"
       << "Need to get " << human_size( total_kb ) << " of archives.\n"
       << "After this operation, " << human_size( total_kb * 3 )
       << " of additional disk space will be used.\n"
       << "Do you want to continue? [Y/n] Y\n\n";
}

int main()
{
  cout << "[simulation] fake apt-like output, no real packages are installed" << endl;

  pace::config::hide_completed( true );
  mt19937_64 rd { random_device {}() };

  const auto packages = make_packages();
  print_apt_preamble( packages );

  { // stage 1, downloading only
    pace::ProgressBar<pace::Channel::Err, pace::Policy::Sync, pace::Region::Relative> bar;
    bar.config()
      .divider( " " )
      .percent_decs( 0 )
      .info_forecolor( pace::Color::Yellow )
      .postfix_forecolor( pace::Color::Yellow )
      .font_bold( false )
      .disable<pace::facade::Counter,
               pace::facade::CharPlot,
               pace::facade::Speed,
               pace::facade::Elapsed,
               pace::facade::ETA>();

    for ( size_t i = 0; i < packages.size(); ++i ) {
      const auto& pkg = packages[i];

      // Derive this package's download timeline from its size and the
      // simulated link speed. One tick per ~150ms keeps the visual lively
      // without flooding the (slow) Sync renderer.
      const double speed    = random_speed_mbs( rd );
      const double duration = ( pkg.size_kb / 1024.0 ) / speed * 1000.0; // ms
      const int ticks       = max( 5, static_cast<int>( duration / 150.0 ) );
      const int tick_ms     = max( 10, static_cast<int>( duration / ticks ) );

      // Begin this package's task cycle: fresh quota => progress 0/ticks.
      bar.config().quota( ticks );
      bar.config().postfix( "[waiting for headers] " + pkg.name + " | archive.ubuntu.com" );
      bar.poke(); // show handshake state before first tick

      for ( int t = 0; t < ticks; ++t ) {
        if ( t == ticks / 3 ) {
          ostringstream sp;
          sp << "[" << fixed << setprecision( 1 ) << speed << " MB/s] " << pkg.repo << " " << pkg.name << " "
             << pkg.version;
          bar.config().postfix( sp.str() );
        }
        bar.tick();
        this_thread::sleep_for( chrono::milliseconds( tick_ms ) );
      }

      // The last tick completed this package's quota, so the bar has reset
      // itself and been erased. Safe to emit this package's own Get: line,
      // newline-terminated, exactly like real apt.
      cout << "Get:" << ( i + 1 ) << " http://archive.ubuntu.com/ubuntu " << pkg.repo << " " << pkg.arch
           << " " << pkg.name << " " << pkg.arch << " " << pkg.version << " [" << pkg.size_kb << " kB]"
           << endl;
    }
  } // bar destroyed here

  const auto install_steps = make_install_steps( packages, rd );
  { // stage 2: installing
    pace::ProgressBar<pace::Channel::Err, pace::Policy::Async, pace::Region::Bottom> bar;
    bar.config()
      .with( pace::option::Quota( install_steps.size() ),
             pace::option::Filler( "#" ),
             pace::option::Lead( "" ),
             pace::option::Remain( " " ),
             pace::option::Divider( " " ),
             pace::option::Prefix( "Progress:" ),
             pace::option::PercentDecs( 0 ),
             pace::option::FontBold( false ),
             pace::option::FillerForecolor(),
             pace::option::InfoForecolor(),
             pace::option::PrefixBackcolor( pace::Color::Green ),
             pace::option::Only<pace::facade::Percentage, pace::facade::CharPlot>() )
      .bar_width( pace::config::terminal_width( pace::Channel::Err ) - bar.layout_width() );

    for ( const auto& step : install_steps ) {
      bar.tick();
      cout << step.text << endl;
      this_thread::sleep_for( chrono::milliseconds( step.delay_ms ) );
    }
  }

  cout << "All packages installed successfully." << endl;
}
