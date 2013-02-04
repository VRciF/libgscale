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
#include <deque>
#include <map>
#include <iostream>

#include <boost/function.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/asio.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>


#include "localnode.hpp"
#include "backend/ibackend.hpp"

namespace GScale{

#define LOG() std::cout << __FILE__ << ":" << __LINE__ << ":" << __PRETTY_FUNCTION__ << std::endl;

class INodeCallback;
class Packet;
class Exception;
class GroupNodesDAO;

class LocalNodeRow{
   public:
	    LocalNodeRow(uint64_t id, LocalNode node, boost::posix_time::ptime time_connect) : node(node){
	    	this->id = id;
	    	this->time_connect = time_connect;
	    	this->backendrefcount = 0;
	    }

	    const boost::uuids::uuid nodeuuid() const { return node.getNodeUUID(); }

	    uint64_t id;

	    LocalNode node;

	    boost::posix_time::ptime time_connect;
	    boost::posix_time::ptime time_disconnect;
	    uint16_t backendrefcount;
};
class BackendRow {
   public:
	    BackendRow(GScale::Backend::IBackend *backend, std::string type){
	    	this->backend = backend;
	    	this->enabled = false;
	    	this->type = type;
	    }

	    void enable(){ this->enabled = true; }
	    void disable(){ this->enabled = false; }

	    GScale::Backend::IBackend *backend;
	    std::string type;
	    bool enabled;
};

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

	    void write(const GScale::Packet &payload);

	    int getEventDescriptor();

	    std::string getName();

	    GScale::Group* getGroupAPI();

	public:

        // define a multiply indexed set with indices by id and name
	    struct idxnode_id{};
	    struct idxnode_time_connect{};
	    struct idxnode_time_disconnect{};
	    struct idxnode_nodeuuid{};
	    struct idxnode_brefcnt{};

	    typedef boost::multi_index_container<
	      LocalNodeRow,
	      boost::multi_index::indexed_by<
              boost::multi_index::ordered_unique<
                  boost::multi_index::tag<idxnode_id>,
                  boost::multi_index::member<LocalNodeRow, uint64_t, &LocalNodeRow::id>
              >,
	          boost::multi_index::ordered_non_unique<
	              boost::multi_index::tag<idxnode_time_connect>,
	              boost::multi_index::member<LocalNodeRow, boost::posix_time::ptime, &LocalNodeRow::time_connect>
	          >,
	          boost::multi_index::ordered_non_unique<
	              boost::multi_index::tag<idxnode_time_disconnect>,
	              boost::multi_index::member<LocalNodeRow, boost::posix_time::ptime, &LocalNodeRow::time_disconnect>
	          >,
	          boost::multi_index::ordered_unique<
	              boost::multi_index::tag<idxnode_nodeuuid>,
	              boost::multi_index::const_mem_fun<LocalNodeRow,const boost::uuids::uuid,&LocalNodeRow::nodeuuid>
	          >,
	          boost::multi_index::ordered_non_unique<
	              boost::multi_index::tag<idxnode_brefcnt>,
	              boost::multi_index::member<LocalNodeRow, uint16_t, &LocalNodeRow::backendrefcount>
	          >
	      >
	    > LocalNodeSet;

	    typedef LocalNodeSet::index<idxnode_id>::type LocalNodeSetIdx_id;
	    typedef LocalNodeSet::index<idxnode_time_connect>::type LocalNodeSetIdx_time_connect;
	    typedef LocalNodeSet::index<idxnode_time_disconnect>::type LocalNodeSetIdx_time_disconnect;
	    typedef LocalNodeSet::index<idxnode_nodeuuid>::type LocalNodeSetIdx_uuid;
	    typedef LocalNodeSet::index<idxnode_brefcnt>::type LocalNodeSetIdx_brefcnt;

	private:
	    struct idxbackend_type{};
	    struct idxbackend_enabled{};

	    typedef boost::multi_index_container<
	      BackendRow,
	      boost::multi_index::indexed_by<
              boost::multi_index::ordered_unique<
                  boost::multi_index::tag<idxbackend_type>,
                  boost::multi_index::member<BackendRow, std::string, &BackendRow::type>
              >,
	          boost::multi_index::ordered_non_unique<
	              boost::multi_index::tag<idxbackend_enabled>,
	              boost::multi_index::member<BackendRow, bool, &BackendRow::enabled>
	          >
	      >
	    > BackendSet;

	    typedef BackendSet::index<idxbackend_type>::type BackendSetIdx_type;
	    typedef BackendSet::index<idxbackend_enabled>::type BackendSetIdx_enabled;

    private:
	    int connectcount;
	    LocalNodeSet localnodes;

	    BackendSet backends;

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
        typename GScale::GroupCore::LocalNodeSet::index<T>::type::iterator begin(){
            return this->groupc->localnodes.get<T>().begin();
        }
        template<typename T>
        typename  GScale::GroupCore::LocalNodeSet::index<T>::type::iterator end(){
            return this->groupc->localnodes.get<T>().end();
        }

        std::pair<GScale::GroupCore::LocalNodeSetIdx_time_connect::iterator, GScale::GroupCore::LocalNodeSetIdx_time_connect::iterator>
            rangeByCreated(boost::posix_time::ptime min = boost::posix_time::ptime(), boost::posix_time::ptime max = boost::posix_time::ptime());
        GScale::GroupCore::LocalNodeSetIdx_uuid::iterator findByUUID(boost::uuids::uuid search);

        template <class Backend> void increaseLocalNodeBackendRefCount(uint64_t id){
        	GScale::GroupCore::LocalNodeSetIdx_id &idx = this->groupc->localnodes.get<GScale::GroupCore::idxnode_id>();
        	GScale::GroupCore::LocalNodeSetIdx_id::iterator it = idx.find(id);
            if(it != idx.end()){
            	//idx.erase(it);

            	LocalNodeRow row= *it;

            	row.backendrefcount++;
            	idx.replace(it, row);
            }
        }
        template <class Backend> void decreaseLocalNodeBackendRefCount(uint64_t id){
        	GScale::GroupCore::LocalNodeSetIdx_id &idx = this->groupc->localnodes.get<GScale::GroupCore::idxnode_id>();
        	GScale::GroupCore::LocalNodeSetIdx_id::iterator it = idx.find(id);
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
