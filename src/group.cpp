/*
 * group.cpp
 *
 */

#include <boost/bind.hpp>

#include "exception.hpp"
#include "group.hpp"

#include "backend/ibackend.hpp"
#include "inodecallback.hpp"
#include "localnode.hpp"
#include "packet.hpp"

namespace GScale{

Group::Group(std::string gname) throw(GScale::Exception){
	processingdeferred = 0;
	if(gname.length()<=0){
		GScale::Exception except(__FILE__,__LINE__);
		except.getMessage() << "Invalid gname["<< gname <<"] given";
		throw except;
	}
	this->gname = gname;
}
Group::~Group(){}

void Group::runWorker(struct timeval *timeout){
	this->processDefferedBackendCallback();
}

void Group::processDefferedBackendCallback(){
	processingdeferred++;

	while(this->deferred_backend.size()>0){
		func_t a_function = this->deferred_backend.front();
		this->deferred_backend.pop_front();
		a_function();
	}

	processingdeferred--;
}

const GScale::LocalNode* Group::connect(std::string nodealias, GScale::INodeCallback &cbs){
	GScale::LocalNode *node = NULL;
	node = new GScale::LocalNode(nodealias, cbs);

	std::map<std::string, GScale::Backend::IBackend*>::iterator it;

	// -1 is the newly inserted node above
	for(it = this->backends.begin(); it != this->backends.end(); it++){
		func_t a_function = boost::bind(&GScale::Backend::IBackend::OnLocalNodeAvailable, it->second, node, boost::ref(this->nodes));
		if(this->processingdeferred>0)
		    this->deferred_backend.push_front(a_function);
		else
			this->deferred_backend.push_back(a_function);
	}

	this->nodes.insert(node);
	return node;
}
const GScale::LocalNode* Group::connect(GScale::INodeCallback &cbs){
	return this->connect(std::string(), cbs);
}

void Group::disconnect(const GScale::LocalNode *node){
	this->nodes.erase((GScale::LocalNode*)node);

	std::map<std::string, GScale::Backend::IBackend*>::iterator it;

	// -1 is the newly inserted node above
	for(it = this->backends.begin(); it != this->backends.end(); it++){
		func_t a_function = boost::bind(&GScale::Backend::IBackend::OnLocalNodeUnavailable, it->second, node, boost::ref(this->nodes));
		if(this->processingdeferred>0)
		    this->deferred_backend.push_front(a_function);
		else
			this->deferred_backend.push_back(a_function);
	}
}

void Group::write(const GScale::Packet &payload/*, Future result */){
	std::map<std::string, GScale::Backend::IBackend*>::iterator it;

	// -1 is the newly inserted node above
	for(it = this->backends.begin(); it != this->backends.end(); it++){
		func_t a_function = boost::bind(&GScale::Backend::IBackend::OnLocalNodeWritesToGroup, it->second, payload, boost::ref(this->nodes));
		if(this->processingdeferred>0)
		    this->deferred_backend.push_front(a_function);
		else
			this->deferred_backend.push_back(a_function);
	}
}

int Group::getEventDescriptor(){ return 0; }

std::string Group::getName(){
	return this->gname;
}

}
