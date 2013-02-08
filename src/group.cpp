/*
 * group.cpp
 *
 */

#include "group.hpp"

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lambda/lambda.hpp>

#include "exception.hpp"

#include "backend/ibackend.hpp"
#include "packet.hpp"
#include "groupcore.hpp"

namespace GScale{

Group::Group(std::string gname) throw(){
	if(gname.length()<=0){
		throw GScale::Exception(std::string("Invalid gname given:")+gname, EINVAL, __FILE__, __LINE__);
	}
	this->gcore = new GScale::GroupCore(this, gname);
}
Group::~Group(){
	delete this->gcore;
}

void Group::runWorker(struct timeval *timeout){
	this->gcore->runWorker(timeout);
}

const GScale::LocalNode Group::connect(std::string nodealias, GScale::INodeCallback &cbs){
	return this->gcore->connect(nodealias, cbs);
}
const GScale::LocalNode Group::connect(GScale::INodeCallback &cbs){
	return this->connect(std::string(), cbs);
}

void Group::disconnect(const GScale::LocalNode node){
	this->gcore->disconnect(node);
}

void Group::write(const boost::shared_ptr<GScale::Packet> packet){
	this->gcore->write(packet);
}

int Group::getEventDescriptor(){
	return this->gcore->getEventDescriptor();
}

std::string Group::getName(){
	return this->gcore->getName();
}

void Group::attachBackend(std::string type, GScale::Backend::IBackend *backend){
	this->gcore->attachBackend(type, backend);
}
void Group::detachBackend(std::string type){
	this->gcore->detachBackend(type);
}

}
