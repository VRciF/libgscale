/*
 * exception.hpp
 *
 */

#ifndef EXCEPTION_HPP_
#define EXCEPTION_HPP_

#include <errno.h>

#include <string>
#include <exception>
#include <sstream>

namespace GScale{

class Exception : public std::exception{
    public:
        Exception(const char *message=NULL, const int code=0, const char *file=NULL, const int line=0);
        Exception(const std::string message, const int code=0, const std::string file=std::string(), const int line=0);

	    virtual ~Exception() throw();

	    int getCode();
	    int getSystemCode();
	    std::string getFile();
	    int getLine();

	    Exception& setLine(const int line);
	    Exception& setFile(const char *file);
	    Exception& setFile(const std::string file);
	    Exception& setCode(const int code);

	    template<class T> Exception& operator<<(T& r) {
	        if(this->message!=NULL){
	            *this->message << r;
	        }
	        return *this;
	    }

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
        int errn;
        std::stringstream *message;
};

}

#endif /* EXCEPTION_HPP_ */
