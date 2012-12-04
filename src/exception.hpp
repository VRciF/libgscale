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
	    virtual ~Exception() throw();

	    Exception& setLine(const int line);
	    Exception& setFile(const char *file);
	    Exception& setFile(const std::string file);
	    Exception& setCode(const int code);

	    std::stringstream& getMessage();

        virtual const char* what() const throw();

    protected:
        int line;
        std::string file;

        int code;
        std::stringstream *message;
};

}

#endif /* EXCEPTION_HPP_ */
