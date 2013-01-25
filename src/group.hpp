/*
 * group.hpp
 *
 */

#ifndef GROUP_HPP_
#define GROUP_HPP_

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

namespace GScale{

#define LOG() std::cout << __FILE__ << ":" << __LINE__ << ":" << __PRETTY_FUNCTION__ << std::endl;

namespace Backend{
    class IBackend;
}

class INode;
class INodeCallback;
class Packet;
class Exception;
class GroupNodesDAO;

class Group{
    friend class GroupNodesDAO;

	public:
	    Group(std::string gname) throw();
	    ~Group();

	    template <class Backend> void attachBackend() throw(){
	    	Backend *b = new Backend();
	    	if(b==NULL){
	    		throw GScale::Exception("not enough memory", ENOMEM, __FILE__,__LINE__);
	    	}
	    	b->initialize(this, this->gdao);

			std::pair<std::map<std::string, GScale::Backend::IBackend*>::iterator,bool> p;
			p = this->backends.insert(  std::pair<std::string, GScale::Backend::IBackend*>(typeid(Backend).name(),b) );
			if(p.second == false){
				throw GScale::Exception("backend insert failed", ENOMEM, __FILE__,__LINE__);
			}
	    }
	    template <class Backend> void detachBackend(){
	    	std::map<std::string, GScale::Backend::IBackend*>::iterator it;
	    	it = this->backends.find(typeid(Backend).name());
	    	if(it!=this->backends.end()){
				Backend *b = it->second;
				delete b;
				this->backends.erase(it);
	    	}
	    }

	    void runWorker(struct timeval *timeout);

	    const GScale::LocalNodePtr connect(std::string nodealias, GScale::INodeCallback &cbs);
	    const GScale::LocalNodePtr connect(GScale::INodeCallback &cbs);

	    void disconnect(const GScale::LocalNodePtr node);

	    void write(const GScale::Packet &payload);

/*
	    int32_t write(GScale::INode *src,
	            const boost::uuids::uuid *dst, std::string *data);
	    template <class Backend>
	    int32_t writeBackend(uint8_t type, GScale::INode *src,
	            const boost::uuids::uuid *dst, std::string *data);
*/
	    int getEventDescriptor();

	    std::string getName();


        // define a multiply indexed set with indices by id and name
	    struct idxname_created{};
	    struct idxname_nodeuuid{};
	    typedef boost::multi_index_container<
	      LocalNodePtr,
	      boost::multi_index::indexed_by<
	          boost::multi_index::ordered_unique<
	              boost::multi_index::tag<idxname_created>,
	              boost::multi_index::mem_fun<GScale::INode,boost::posix_time::ptime,&GScale::INode::created>
	          >,
	          boost::multi_index::ordered_unique<
	              boost::multi_index::tag<idxname_nodeuuid>,
	              boost::multi_index::const_mem_fun<GScale::INode,const boost::uuids::uuid,&GScale::INode::getNodeUUID>
	          >
	      >
	    > LocalNodesSet;
	    typedef LocalNodesSet::index<idxname_created>::type LocalNodesSetIdx_creationtime;
	    typedef LocalNodesSet::index<idxname_nodeuuid>::type LocalNodesSetIdx_uuid;
/*
	    typedef std::pair<GScale::LocalNodesSetIdx_creationtime::iterator, GScale::LocalNodesSetIdx_creationtime::iterator> LocalNodesSetRange_creationtime;
	    typedef LocalNodesSetIdx_creationtime::iterator LocalNodesSetIterator_creationtime;
	    typedef LocalNodesSetIdx_uuid::iterator LocalNodesSetIterator_uuid;
*/
    private:
	    LocalNodesSet localnodes;

	    std::map<std::string, GScale::Backend::IBackend*> backends;

	    std::string gname;
	    int processingdeferred;

	    boost::asio::io_service io_service;
	    GroupNodesDAO *gdao;
};


class GroupNodesDAO{
    public:
        GroupNodesDAO(GScale::Group *group);
        ~GroupNodesDAO();

        template<Typename T>
        GScale::Group::LocalNodesSet::index<T>::type::iterator begin(){
            return this->group->localnodes.get<T>().begin();
        }
        template<Typename T>
        GScale::Group::LocalNodesSet::index<T>::type::iterator end(){
            return this->group->localnodes.get<T>().end();
        }

        std::pair<GScale::Group::LocalNodesSetIdx_creationtime::iterator, GScale::Group::LocalNodesSetIdx_creationtime::iterator>
            rangeByCreated(boost::posix_time::ptime min = boost::posix_time::ptime(), boost::posix_time::ptime max = boost::posix_time::ptime());
        GScale::Group::LocalNodesSetIdx_uuid::iterator findByUUID(boost::uuids::uuid search);

    private:
        GScale::Group *group;
};


}

#endif /* GROUP_HPP_ */
