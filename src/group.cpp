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

/*
#include "inodecallback.hpp"
#include "localnode.hpp"

*/

namespace GScale{

Group::Group(std::string gname) throw(){
	processingdeferred = 0;
	if(gname.length()<=0){
		throw GScale::Exception(std::string("Invalid gname given:")+gname, EINVAL, __FILE__, __LINE__);
	}
	this->gname = gname;

	this->gdao = new GroupNodesDAO(this);
}
Group::~Group(){}

void Group::runWorker(struct timeval *timeout){
    LOG();
    std::map<std::string, GScale::Backend::IBackend*>::iterator it;
    for(it = this->backends.begin(); it != this->backends.end(); it++){
        this->io_service.dispatch(boost::bind(&GScale::Backend::IBackend::Worker, it->second));
    }

    // Construct a timer without setting an expiry time.
    boost::asio::deadline_timer timer(this->io_service);
    if(timeout!=NULL){
        LOG();
        // Set an expiry time relative to now.
        timer.expires_from_now(boost::posix_time::seconds(timeout->tv_sec)+boost::posix_time::microseconds(timeout->tv_usec));
        // Wait for the timer to expire.
        timer.async_wait(boost::bind(&boost::asio::io_service::stop, &this->io_service));
    }
    if(this->io_service.stopped()){
        this->io_service.reset();
    }
    this->io_service.run();

    LOG();
}

const GScale::LocalNodePtr Group::connect(std::string nodealias, GScale::INodeCallback &cbs){
    LocalNodePtr node;
    boost::posix_time::ptime now;

    do{
        now = boost::posix_time::microsec_clock::universal_time();
    }while(this->localnodes.get<0>().find(now)!=this->localnodes.end());

    node.reset(new GScale::LocalNode(nodealias, cbs, now));

	std::map<std::string, GScale::Backend::IBackend*>::iterator it;

	for(it = this->backends.begin(); it != this->backends.end(); it++){
	    this->io_service.post(boost::bind(&GScale::Backend::IBackend::OnLocalNodeAvailable, it->second, node.get()));
	}

	this->localnodes.insert(node);
	return node;
}
const GScale::LocalNodePtr Group::connect(GScale::INodeCallback &cbs){
	return this->connect(std::string(), cbs);
}

void Group::disconnect(const GScale::LocalNodePtr node){
    if(this->localnodes.get<1>().find(node->getNodeUUID()) != this->localnodes.get<1>().end()){
        this->localnodes.get<1>().erase(this->localnodes.get<1>().find(node->getNodeUUID()));
    }

	std::map<std::string, GScale::Backend::IBackend*>::iterator it;

	// -1 is the newly inserted node above
	for(it = this->backends.begin(); it != this->backends.end(); it++){
	    this->io_service.post(boost::bind(&GScale::Backend::IBackend::OnLocalNodeUnavailable, it->second, node.get()));
	}
}

void Group::write(const GScale::Packet &payload/*, Future result */){
	std::map<std::string, GScale::Backend::IBackend*>::iterator it;

	// -1 is the newly inserted node above
	for(it = this->backends.begin(); it != this->backends.end(); it++){
	    this->io_service.post(boost::bind(&GScale::Backend::IBackend::OnLocalNodeWritesToGroup, it->second, payload));
	}
}

int Group::getEventDescriptor(){ return 0; }

std::string Group::getName(){
	return this->gname;
}

////////////////////////////////// GroupNodesDAO ////////////////////////////////////

GroupNodesDAO::GroupNodesDAO(GScale::Group *group){
    this->group = group;
}
GroupNodesDAO::~GroupNodesDAO(){}

std::pair<GScale::Group::LocalNodesSetIdx_creationtime::iterator, GScale::Group::LocalNodesSetIdx_creationtime::iterator>
GroupNodesDAO::rangeByCreated(boost::posix_time::ptime min, boost::posix_time::ptime max){
    if(!min.is_not_a_date_time() && !max.is_not_a_date_time() && min>max){
        boost::posix_time::ptime swap;

        swap = min;
        min = max;
        max = swap;
    }

    std::pair<GScale::Group::LocalNodesSetIdx_creationtime::iterator, GScale::Group::LocalNodesSetIdx_creationtime::iterator> result;
    if(min.is_not_a_date_time() && max.is_not_a_date_time()){
        result = this->group->localnodes.get<GScale::Group::idxname_created>().range(boost::multi_index::unbounded, boost::multi_index::unbounded);
    }
    else if(min.is_not_a_date_time()){
        result = this->group->localnodes.get<GScale::Group::idxname_created>().range(boost::multi_index::unbounded, boost::lambda::_1<=max);
    }
    else if(max.is_not_a_date_time()){
        result = this->group->localnodes.get<GScale::Group::idxname_created>().range(min<boost::lambda::_1, boost::multi_index::unbounded);
    }
    else{
        result = this->group->localnodes.get<GScale::Group::idxname_created>().range(min<=boost::lambda::_1, boost::lambda::_1<=max);
    }

    return result;
}

GScale::Group::LocalNodesSetIdx_uuid::iterator GroupNodesDAO::findByUUID(boost::uuids::uuid search){
    GScale::Group::LocalNodesSetIdx_uuid::iterator result = this->group->localnodes.get<GScale::Group::idxname_nodeuuid>().find(search);
    if(result == this->group->localnodes.get<GScale::Group::idxname_nodeuuid>().end()){
        throw GScale::Exception(std::string("node uuid not found:")+boost::lexical_cast<std::string>(search), ENOENT);
    }
    return result;
}


}
