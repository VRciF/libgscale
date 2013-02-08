/*
 * group.hpp
 *
 */

#ifndef GROUPCORE_HPP_
#define GROUPCORE_HPP_

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

#include <typeinfo>
#include <utility>
#include <string>
#include <set>
#include <map>
#include <iostream>
#include <vector>

#include <boost/function.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/asio.hpp>

#include "localnode.hpp"
#include "backend/ibackend.hpp"
#include "util/groupcore.hpp"

namespace GScale{

class INodeCallback;
class Packet;
class Exception;
class GroupNodesDAO;

class Group;

class GroupCore{
    friend class GroupNodesDAO;

	public:
        GroupCore(GScale::Group *group, std::string gname) throw();
	    ~GroupCore();

	    void attachBackend(std::string type, GScale::Backend::IBackend *backend);
	    void detachBackend(std::string type);

	    void runWorker(struct timeval *timeout = NULL);

	    const GScale::LocalNode connect(std::string nodealias, GScale::INodeCallback &cbs);
	    const GScale::LocalNode connect(GScale::INodeCallback &cbs);

	    void disconnect(const GScale::LocalNode node);

	    void write(const PacketPtr packet);

	    int getEventDescriptor();

	    std::string getName();

	    GScale::Group* getGroupAPI();

    private:
	    GScale::Index::LocalNodeSet localnodes;
	    GScale::Index::BackendSet backends;
	    GScale::Index::PacketSet sendqueue;

	    int connectcount;
	    std::string gname;

	    boost::asio::io_service io_service;
	    GroupNodesDAO *gdao;
	    // the backend's need to be able, to provide the node callbacks the public Group API
	    // thus we need the group instance here
	    GScale::Group *group;
};


class GroupNodesDAO{
    public:
        GroupNodesDAO(GScale::GroupCore *groupc);
        ~GroupNodesDAO();

        template<typename T>
        typename GScale::Index::LocalNodeSet::index<T>::type::iterator begin(){
            return this->groupc->localnodes.get<T>().begin();
        }
        template<typename T>
        typename  GScale::Index::LocalNodeSet::index<T>::type::iterator end(){
            return this->groupc->localnodes.get<T>().end();
        }

        std::pair<GScale::Index::LocalNodeSet_time_connect::iterator, GScale::Index::LocalNodeSet_time_connect::iterator>
            rangeByCreated(boost::posix_time::ptime min = boost::posix_time::ptime(), boost::posix_time::ptime max = boost::posix_time::ptime());
        GScale::Index::LocalNodeSet_uuid::iterator findByUUID(boost::uuids::uuid search);

        template <class Backend> void increaseLocalNodeBackendRefCount(uint64_t id){
        	GScale::Index::LocalNodeSet_id &idx = this->groupc->localnodes.get<GScale::Index::node_id>();
        	GScale::Index::LocalNodeSet_id::iterator it = idx.find(id);
            if(it != idx.end()){
            	//idx.erase(it);

            	LocalNodeRow row= *it;

            	row.backendrefcount++;
            	idx.replace(it, row);
            }
        }
        template <class Backend> void decreaseLocalNodeBackendRefCount(uint64_t id){
        	GScale::Index::LocalNodeSet_id &idx = this->groupc->localnodes.get<GScale::Index::node_id>();
        	GScale::Index::LocalNodeSet_id::iterator it = idx.find(id);
            if(it != idx.end()){
            	//idx.erase(it);

            	LocalNodeRow row= *it;

            	row.backendrefcount--;
            	idx.replace(it, row);
            }
        }
    private:
        GScale::GroupCore *groupc;
};


}

#endif /* GROUP_HPP_ */
