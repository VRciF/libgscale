/*
 * exception.cpp
 *
 */

#include "exception.hpp"
#include <string.h>

namespace GScale{

Exception::Exception(const int code, const char *file, const int line) {
	this->errn_ = errno;
	this->init(std::string(),code, std::string(file),line);
}

Exception::Exception(const char *message, const int code, const char *file, const int line) {
	this->errn_ = errno;
	this->init(std::string(message),code, std::string(file),line);
}

Exception::Exception(const std::string message, const int code, const std::string file, const int line){
	this->errn_ = errno;
	this->init(message,code,file,line);
}

void Exception::init(const std::string message, const int code, const std::string file, const int line){
    char errbuffer[1024] = {'\0'};

    try{
        this->smessage = new std::stringstream();
    }
    catch(...){}

	if(message.length()>0){
		if(this->smessage!=NULL){
			(*this->smessage) << message;
		}
		else{
			this->message += message;
		}
	}
	else{
		strerror_r(this->errn_, errbuffer, sizeof(errbuffer)-1);
		if(this->smessage!=NULL){
			(*this->smessage) << std::string(errbuffer, strlen(errbuffer));
		}
		else{
			this->message += errbuffer;
		}
	}

    this->code(code).file(file).line(line);
}

Exception::~Exception() throw(){
	if(smessage!=NULL){
	    delete this->smessage;
	}
}

int Exception::code(){
    return this->code_;
}
std::string Exception::file(){
    return this->file_;
}
int Exception::line(){
    return this->line_;
}

Exception& Exception::line(const int line){
	this->line_ = line;
	return *this;
}
Exception& Exception::file(const char *file){
	this->file_ = std::string(file);
	return *this;
}
Exception& Exception::file(const std::string file){
	this->file_ = file;
	return *this;
}
Exception& Exception::code(const int code){
	this->code_ = code;
	return *this;
}

const char* Exception::what() const throw()
{
	try{
		if(this->smessage!=NULL){
	        return (this->smessage->str() + " [ " + this->message + " ] ").c_str();
		}
		else{
			return this->message.c_str();
		}
	}catch(...){
		return "internal what() error - unable to generate excetion message string";
	}
}
int Exception::get_native_error() const{
	return this->errn_;
}
int Exception::get_error_code() const{
	return this->code_;
}


}
