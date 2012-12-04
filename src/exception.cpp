/*
 * exception.cpp
 *
 */

#include "exception.hpp"

namespace GScale{

Exception::Exception(const char *file, const int line) : message(NULL){
	this->message = new std::stringstream();
	this->setFile(file);
	this->setLine(line);
}
Exception::Exception(const std::string file, const int line) : message(NULL){
	this->message = new std::stringstream();
	this->setFile(file);
	this->setLine(line);
}
Exception::~Exception() throw(){
	if(message!=NULL){
	    delete this->message;
	}
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

std::stringstream& Exception::getMessage(){
	return *this->message;
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
