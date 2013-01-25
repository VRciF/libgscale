/*
 * exception.cpp
 *
 */

#include "exception.hpp"
#include <string.h>

namespace GScale{

Exception::Exception(const char *message, const int code, const char *file, const int line) : message(NULL){
    char errbuffer[1024] = {'\0'};

    this->errn = errno;
    try{
        this->message = new std::stringstream();
        if(message!=NULL && message[0]!='\0'){
            *this->message << message;
        }
        else{
            strerror_r(this->errn, errbuffer, sizeof(errbuffer));
            *this->message << std::string(errbuffer, strlen(errbuffer));
        }
    }
    catch(...){}

    this->setCode(code);
    this->setFile(file);
    this->setLine(line);
}

Exception::Exception(const std::string message, const int code, const std::string file, const int line) : message(NULL){
    char errbuffer[1024] = {'\0'};

    this->errn = errno;
    try{
        this->message = new std::stringstream();
        if(message.length()>0){
            *this->message << message;
        }
        else{
            strerror_r(this->errn, errbuffer, sizeof(errbuffer));
            *this->message << std::string(errbuffer, strlen(errbuffer));
        }
    }
    catch(...){}

    this->setCode(code);
	this->setFile(file);
	this->setLine(line);
}

Exception::~Exception() throw(){
	if(message!=NULL){
	    delete this->message;
	}
}

int Exception::getCode(){
    return this->code;
}
int Exception::getSystemCode(){
    return this->errn;
}
std::string Exception::getFile(){
    return this->file;
}
int Exception::getLine(){
    return this->line;
}

Exception& Exception::setLine(const int line){
	this->line = line;
	return *this;
}
Exception& Exception::setFile(const char *file){
	this->file = std::string(file);
	return *this;
}
Exception& Exception::setFile(const std::string file){
	this->file = file;
	return *this;
}
Exception& Exception::setCode(const int code){
	this->code = code;
	return *this;
}

const char* Exception::what() const throw()
{
	try{
	    return this->message->str().c_str();
	}catch(...){
		return "internal what() error - unable to generate excetion message string";
	}
}

}
