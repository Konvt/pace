#ifndef PGBAR_TIMER
#define PGBAR_TIMER

#include "../io/CharPipeline.hpp"
#include "../traits/TypeSet.hpp"

namespace pgbar {
  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Timer : public Base {
      protected:
        static constexpr auto& _default_timer       = u8"??:??:??";
        static constexpr types::Char _overflow_char = '#';
        static constexpr auto& _default_overflow    = u8"##:##:##";

        PGBAR__NODISCARD PGBAR__FORCEINLINE io::CharPipeline& to_hms( io::CharPipeline& pipeline,
                                                                      details::types::Tempus duration ) const
        {
          auto zfill2 = [&]( std::int64_t num_time ) -> io::CharPipeline& {
            PGBAR__TRUST( num_time >= 0 );
            if ( num_time > 99 )
              return pipeline.append( _overflow_char, 2 );

            auto ret = utils::format( num_time );
            if ( ret.size() < 2 )
              ret.insert( 0, 1, '0' );
            return pipeline << ret;
          };
          const auto hours = std::chrono::duration_cast<std::chrono::hours>( duration );
          duration -= hours;
          const auto minutes = std::chrono::duration_cast<std::chrono::minutes>( duration );
          duration -= minutes;

          zfill2( hours.count() ) << ':';
          zfill2( minutes.count() ) << ':';
          return zfill2( std::chrono::duration_cast<std::chrono::seconds>( duration ).count() );
        }

        template<typename... Options>
        constexpr Timer( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
        {}

        PGBAR__SPECIAL_MEMBERS( Timer );
      };
    } // namespace aspects
  } // namespace details
} // namespace pgbar

#endif
