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
	    Exception(const int code=errno, const char *file=NULL, const int line=0);
        Exception(const char *message, const int code=errno, const char *file=NULL, const int line=0);
        Exception(const std::string message, const int code=errno, const std::string file=std::string(), const int line=0);

	    virtual ~Exception() throw();

	    int code();
	    std::string file();
	    int line();

	    Exception& line(const int line);
	    Exception& file(const char *file);
	    Exception& file(const std::string file);
	    Exception& code(const int code);

	    template<class T> Exception& operator<<(T& r) {
	        if(this->smessage!=NULL){
	            *this->smessage << r;
	        }
	        else{
	        	this->message += r;
	        }
	        return *this;
	    }

        virtual const char* what() const throw();
        int get_native_error() const;
        int get_error_code() const;

    protected:
        void init(const std::string message, const int code=0, const std::string file=std::string(), const int line=0);

        int line_;
        std::string file_;

        int code_;
        int errn_;
        std::stringstream *smessage;
        std::string message;
};

}

#endif /* EXCEPTION_HPP_ */
