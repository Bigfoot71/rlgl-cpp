#ifndef RLGL_EXCEPTION_HPP
#define RLGL_EXCEPTION_HPP

#include <exception>
#include <string>

namespace rlgl {

    class RLGLException : public std::exception
    {
      public:
        RLGLException(const std::string& _message) : message(_message) { }

        inline const char* what() const noexcept override
        {
            return message.c_str();
        }

      private:
        std::string message;
    };

}

#endif //RLGL_EXCEPTION_HPP
