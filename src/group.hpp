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
#include "exception.hpp"

namespace GScale{

namespace Backend{
    class IBackend;
}

class GroupCore;
class INodeCallback;
class Packet;

class Group{
    friend class GroupNodesDAO;

	public:
	    Group(std::string gname) throw();
	    ~Group();

	    template <class Backend> void attachBackend(){
	    	Backend *b = NULL;
	    	try{
	    	    b = new Backend();
	    	    this->attachBackend(typeid(Backend).name(), b);
	    	}
	    	catch(GScale::Exception &g){
	    		if(b!=NULL){ delete b; }
	    		throw g;
	    	}
	    	catch(...){
	    		if(b!=NULL){ delete b; }
	    		throw GScale::Exception(__FILE__, __LINE__);
	    	}
	    }
	    template <class Backend> void detachBackend(){
	    	this->detachBackend(typeid(Backend).name());
	    }

	    void runWorker(struct timeval *timeout = NULL);

	    const GScale::LocalNode connect(std::string nodealias, GScale::INodeCallback &cbs);
	    const GScale::LocalNode connect(GScale::INodeCallback &cbs);

	    void disconnect(const GScale::LocalNode node);

	    void write(const GScale::Packet &payload);

	    int getEventDescriptor();

	    std::string getName();

	private:
	    void attachBackend(std::string type, GScale::Backend::IBackend *backend);
	    void detachBackend(std::string type);

	    GScale::GroupCore *gcore;
};

}

#endif /* GROUP_HPP_ */
