/*
 * exception.hpp
 *
 */

#ifndef EXCEPTION_HPP_
#define EXCEPTION_HPP_

#include <string>
#include <exception>
#include <sstream>

namespace GScale{

class Exception : public std::exception{
    public:
	    Exception(const char *file, const int line);
	    Exception(const std::string file, const int line);
	    /*
	    interprocess_exception(const char *);
	      interprocess_exception(const error_info &, const char * = 0);
	      */
	    virtual ~Exception() throw();

	    Exception& setLine(const int line);
	    Exception& setFile(const char *file);
	    Exception& setFile(const std::string file);
	    Exception& setCode(const int code);

	    std::stringstream& getMessage();

        virtual const char* what() const throw();
        /*
        const char * what() const;
        native_error_t get_native_error() const;
        error_code_t get_error_code() const;
        */

    protected:
        int line;
        std::string file;

        int code;
        std::stringstream *message;
};

}

#endif /* EXCEPTION_HPP_ */
