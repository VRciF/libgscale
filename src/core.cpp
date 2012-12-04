/*
 * core.cpp
 *
 */

#include "core.hpp"

namespace GScale{

Core *Core::inst = NULL;

Core* Core::getInstance(){
	if(Core::inst == NULL){
		Core::inst = new Core();
	}
	return Core::inst;
}

Core::Core() : hostuuid(boost::uuids::uuid()){}
Core::~Core(){}

boost::uuids::uuid Core::getHostUUID(){
	return this->hostuuid;
}

}
