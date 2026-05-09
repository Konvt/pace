#ifndef PACE_ERROR
#define PACE_ERROR

#include "../details/charcodes/CoWString.hpp"
#include <exception>
#include <system_error>

namespace pace {
  namespace exception {
    // The base exception class.
    class Error : public std::exception {
    protected:
      details::charcodes::CoWString message_;

    public:
      Error( details::charcodes::CoWString mes ) noexcept : message_ { std::move( mes ) } {}
      ~Error() override = default;
      PACE__NODISCARD const char* what() const noexcept override { return message_.c_str(); }
    };

    // Exception for invalid function arguments.
    class InvalidArgument : public Error {
    public:
      using Error::Error;
      ~InvalidArgument() override = default;
    };

    // Exception for error state of object.
    class InvalidState : public Error {
    public:
      using Error::Error;
      ~InvalidState() override = default;
    };

    // Exception for local system error.
    class SystemError : public Error {
    protected:
      std::error_code err_;

    public:
      SystemError( std::error_code err, details::charcodes::CoWString mes ) noexcept
        : Error( std::move( mes ) ), err_ { err }
      {}
      ~SystemError() override = default;

      const std::error_code& code() const noexcept { return err_; }
    };
  } // namespace exception
} // namespace pace

#endif
