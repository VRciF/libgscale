/*
 * group.hpp
 *
 */

#ifndef GROUP_HPP_
#define GROUP_HPP_

#include <errno.h>
#include <string>
#include <set>
#include <deque>
#include <map>
#include <sys/time.h>
#include <typeinfo>
#include <boost/function.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "exception.hpp"

namespace GScale{

class INode;
class LocalNode;
class INodeCallback;
class Packet;

namespace Backend{
    class IBackend;
}

typedef std::set<const GScale::LocalNode*> LocalNodes;

class Group{
	public:
	    Group(std::string gname) throw(GScale::Exception);
	    ~Group();

	    template <class Backend> void attachBackend() throw(){
	    	Backend *b = new Backend(this);
	    	if(b==NULL){
	    		GScale::Exception except(__FILE__,__LINE__);
	    		except.setCode(ENOMEM).getMessage() << "not enough memory";
	    		throw except;
	    	}

			std::pair<std::map<std::string, GScale::Backend::IBackend*>::iterator,bool> p;
			p = this->backends.insert(  std::pair<std::string, GScale::Backend::IBackend*>(typeid(Backend).name(),b) );
			if(p.second == false){
				GScale::Exception except(__FILE__,__LINE__);
				except.setCode(ENOMEM).getMessage() << "backend insert failed";
				throw except;
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

	    const GScale::LocalNode* connect(std::string nodealias, GScale::INodeCallback &cbs);
	    const GScale::LocalNode* connect(GScale::INodeCallback &cbs);

	    void disconnect(const GScale::LocalNode *node);

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
	protected:
	    void processDefferedBackendCallback();
	private:
	    LocalNodes nodes;
	    std::map<std::string, GScale::Backend::IBackend*> backends;

	    std::string gname;
	    int processingdeferred;

	    // used to call defered backend functions
	    typedef boost::function<void (void)> func_t;
	    std::deque<func_t> deferred_backend;

};

}

/*
 * deferred callback

void foo(int i, string s) { ... }

typedef boost::function<void (void)> func_t;
std::vector<func_t> v;
func_t a_function = boost:bind(&foo, 1, "fred");
v.push_back(a_function);

... then later ...

func_t a_function = v.front();
a_function(); // <- invoke foo(1, "fred");
*/

#endif /* GROUP_HPP_ */
