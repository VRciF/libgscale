/*
 * group.cpp
 *
 */

#include "groupcore.hpp"

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

GroupCore::GroupCore(GScale::Group *group, std::string gname) throw(){
	this->group = group;
	if(gname.length()<=0){
		throw GScale::Exception(std::string("Invalid gname given:")+gname, EINVAL, __FILE__, __LINE__);
	}
	this->gname = gname;

	this->connectcount = 0;
	this->gdao = new GroupNodesDAO(this);
}
GroupCore::~GroupCore(){
	delete this->gdao;
}

void GroupCore::attachBackend(std::string type, GScale::Backend::IBackend *backend){
	BackendSetIdx_type &idx = this->backends.get<idxbackend_type>();

	if(idx.find(type) == idx.end()){
	    try{
	        BackendRow row(backend, type);
	        backend->initialize(this, this->gdao);
	        row.enable();

	        idx.insert(row);
	    }
	    catch(...){
		    throw GScale::Exception("Couldn't insert new backend", errno, __FILE__, __LINE__);
	    }
	}
	else{
	    throw GScale::Exception("Backend already attached", EALREADY, __FILE__, __LINE__);
	}
}
void GroupCore::detachBackend(std::string type){
	BackendSetIdx_type &idx = this->backends.get<idxbackend_type>();
	BackendSetIdx_type::iterator it = idx.find(type);
	if(it!=idx.end()){
		BackendRow row = *it;
		row.disable();
		idx.replace(it, row);
	}
}

void GroupCore::runWorker(struct timeval *timeout){
    {
        BackendSetIdx_enabled &beidx = this->backends.get<idxbackend_enabled>();
        // the all nodes with backendref equals one <=> range (0,1]
        std::pair<BackendSetIdx_enabled::iterator,BackendSetIdx_enabled::iterator> p = beidx.range(false<boost::lambda::_1,boost::lambda::_1<=true);
        for(;p.first!=p.second;p.first++){
    	    this->io_service.dispatch(boost::bind(&GScale::Backend::IBackend::Worker, p.first->backend));
        }
    }

    {
        // Construct a timer without setting an expiry time.
        boost::asio::deadline_timer timer(this->io_service);
        if(timeout!=NULL){
            // Set an expiry time relative to now.
            timer.expires_from_now(boost::posix_time::seconds(timeout->tv_sec)+boost::posix_time::microseconds(timeout->tv_usec));
            // Wait for the timer to expire.
            timer.async_wait(boost::bind(&boost::asio::io_service::stop, &this->io_service));
        }
        if(this->io_service.stopped()){
            this->io_service.reset();
        }
        this->io_service.run();
    }

    {
        // drop every node with backendrefcount zero
    	LocalNodeSetIdx_brefcnt &idx = this->localnodes.get<idxnode_brefcnt>();
        // the all nodes with backendref equals zero <=> range [0,1)
        std::pair<LocalNodeSetIdx_brefcnt::iterator,LocalNodeSetIdx_brefcnt::iterator> p = idx.range(0<=boost::lambda::_1,boost::lambda::_1<1);
        if(p.first!=p.second && p.first!=idx.end()){
    	    idx.erase(p.first, p.second);
        }
    }

    {
        // drop deleted backends
        BackendSetIdx_enabled &beidx = this->backends.get<idxbackend_enabled>();
        // the all nodes with backendref equals zero <=> range [0,1)
        std::pair<BackendSetIdx_enabled::iterator,BackendSetIdx_enabled::iterator> p = beidx.range(false<=boost::lambda::_1,boost::lambda::_1<true);
        if(p.first!=p.second && p.first!=beidx.end()){
    	    beidx.erase(p.first, p.second);
        }
    }
}

const GScale::LocalNode GroupCore::connect(std::string nodealias, GScale::INodeCallback &cbs){
	this->connectcount++;

	LocalNodeRow row(this->connectcount, LocalNode(nodealias, cbs), boost::posix_time::microsec_clock::universal_time());

	this->localnodes.insert(row);

    BackendSetIdx_enabled &beidx = this->backends.get<idxbackend_enabled>();
    // the all nodes with backendref equals one <=> range (0,1]
    std::pair<BackendSetIdx_enabled::iterator,BackendSetIdx_enabled::iterator> p = beidx.range(false<boost::lambda::_1,boost::lambda::_1<=true);
    for(;p.first!=p.second;p.first++){
	    this->io_service.post(boost::bind(&GScale::Backend::IBackend::OnLocalNodeAvailable, p.first->backend, row.node));
    }

	return row.node;
}
const GScale::LocalNode GroupCore::connect(GScale::INodeCallback &cbs){
	return this->connect(std::string(), cbs);
}

void GroupCore::disconnect(const GScale::LocalNode node){
	LocalNodeSetIdx_uuid &idx = this->localnodes.get<idxnode_nodeuuid>();
	LocalNodeSetIdx_uuid::iterator it = idx.find(node.getNodeUUID());
    if(it != idx.end()){
    	//idx.erase(it);

    	LocalNodeRow row= *it;

    	row.time_disconnect = boost::posix_time::microsec_clock::universal_time();
    	idx.replace(it, row);
    }

    BackendSetIdx_enabled &beidx = this->backends.get<idxbackend_enabled>();
    // the all nodes with backendref equals one <=> range (0,1]
    std::pair<BackendSetIdx_enabled::iterator,BackendSetIdx_enabled::iterator> p = beidx.range(false<boost::lambda::_1,boost::lambda::_1<=true);
    for(;p.first!=p.second;p.first++){
	    this->io_service.post(boost::bind(&GScale::Backend::IBackend::OnLocalNodeUnavailable, p.first->backend, node));
    }
}

void GroupCore::write(const GScale::Packet &packet/*, Future result */){
    BackendSetIdx_enabled &beidx = this->backends.get<idxbackend_enabled>();
    // the all nodes with backendref equals one <=> range (0,1]
    std::pair<BackendSetIdx_enabled::iterator,BackendSetIdx_enabled::iterator> p = beidx.range(false<boost::lambda::_1,boost::lambda::_1<=true);
    for(;p.first!=p.second;p.first++){
	    this->io_service.post(boost::bind(&GScale::Backend::IBackend::OnLocalNodeWritesToGroup, p.first->backend, packet));
    }
}

int GroupCore::getEventDescriptor(){ return 0; }

std::string GroupCore::getName(){
	return this->gname;
}

GScale::Group* GroupCore::getGroupAPI(){
	return this->group;
}

////////////////////////////////// GroupNodesDAO ////////////////////////////////////

GroupNodesDAO::GroupNodesDAO(GScale::GroupCore *groupc){
    this->groupc = groupc;
}
GroupNodesDAO::~GroupNodesDAO(){}

std::pair<GScale::GroupCore::LocalNodeSetIdx_time_connect::iterator, GScale::GroupCore::LocalNodeSetIdx_time_connect::iterator>
GroupNodesDAO::rangeByCreated(boost::posix_time::ptime min, boost::posix_time::ptime max){
    if(!min.is_not_a_date_time() && !max.is_not_a_date_time() && min>max){
        boost::posix_time::ptime swap;

        swap = min;
        min = max;
        max = swap;
    }

    std::pair<GScale::GroupCore::LocalNodeSetIdx_time_connect::iterator, GScale::GroupCore::LocalNodeSetIdx_time_connect::iterator> result;
    if(min.is_not_a_date_time() && max.is_not_a_date_time()){
        result = this->groupc->localnodes.get<GScale::GroupCore::idxnode_time_connect>().range(boost::multi_index::unbounded, boost::multi_index::unbounded);
    }
    else if(min.is_not_a_date_time()){
        result = this->groupc->localnodes.get<GScale::GroupCore::idxnode_time_connect>().range(boost::multi_index::unbounded, boost::lambda::_1<=max);
    }
    else if(max.is_not_a_date_time()){
        result = this->groupc->localnodes.get<GScale::GroupCore::idxnode_time_connect>().range(min<boost::lambda::_1, boost::multi_index::unbounded);
    }
    else{
        result = this->groupc->localnodes.get<GScale::GroupCore::idxnode_time_connect>().range(min<=boost::lambda::_1, boost::lambda::_1<=max);
    }

    return result;
}

GScale::GroupCore::LocalNodeSetIdx_uuid::iterator GroupNodesDAO::findByUUID(boost::uuids::uuid search){
    GScale::GroupCore::LocalNodeSetIdx_uuid::iterator result = this->groupc->localnodes.get<GScale::GroupCore::idxnode_nodeuuid>().find(search);
    if(result == this->groupc->localnodes.get<GScale::GroupCore::idxnode_nodeuuid>().end()){
        throw GScale::Exception(std::string("node uuid not found:")+boost::lexical_cast<std::string>(search), ENOENT);
    }
    return result;
}


}
