/*
 * main.cpp
 *
 */

#include <string>
#include <iostream>
#include <map>
#include <boost/uuid/uuid_io.hpp>
#include <gscale.hpp>

class LoopbackCallbacks : public GScale::INodeCallback{
	public:
	    LoopbackCallbacks(){
	    	node2message.insert(std::pair<std::string,std::string>("Carol->Carlos", "Hello node!"));
	    	node2message.insert(std::pair<std::string,std::string>("Carlos->Carol", "Greetings back to you!"));
	    	node2message.insert(std::pair<std::string,std::string>("Carol->Carlos", "")); // EOCommunication

	    	node2message.insert(std::pair<std::string,std::string>("Carol->", "Hello node!"));
	    	node2message.insert(std::pair<std::string,std::string>("->Carol", "Hi, howre u?"));
	    	node2message.insert(std::pair<std::string,std::string>("Carol->", "Fine thx"));
	    	node2message.insert(std::pair<std::string,std::string>("->Carol", "")); // EOCommunication
	    	node2message.insert(std::pair<std::string,std::string>("Carlos->", "Ehlo!"));
	    	node2message.insert(std::pair<std::string,std::string>("->Carlos", "Oh, nice, a second node to communicate, howre u?"));
	    	node2message.insert(std::pair<std::string,std::string>("Carlos->", "Great, things seem to work"));
	    	node2message.insert(std::pair<std::string,std::string>("->Carlos", "good to hear"));
	    	node2message.insert(std::pair<std::string,std::string>("Carlos->", "")); // EOCommunication

	    	node2message.insert(std::pair<std::string,std::string>("Carol->Carlos", "ure leaving? too bad"));
	    	node2message.insert(std::pair<std::string,std::string>("Carlos->Carol", "maybe im back soon, bye"));
	    	node2message.insert(std::pair<std::string,std::string>("Carol->Carlos", "cya"));
	    	node2message.insert(std::pair<std::string,std::string>("Carlos->Carol", "")); // EOCommunication
	    	node2message.insert(std::pair<std::string,std::string>("->Carlos", "hope to cya soon"));
	    	node2message.insert(std::pair<std::string,std::string>("Carlos->", "me too, bye"));
	    	node2message.insert(std::pair<std::string,std::string>("->Carlos", "")); // EOCommunication

	    	node2message.insert(std::pair<std::string,std::string>("Carol->", "ure leaving too, omfg shall i handle the work all alone now?"));
	    	node2message.insert(std::pair<std::string,std::string>("->Carol", "im sure another node will be online soon, cause everything is working so great ^^"));
	    	node2message.insert(std::pair<std::string,std::string>("Carol->", "me hopes, cya"));
	    	node2message.insert(std::pair<std::string,std::string>("->Carol", "cu"));
	    }
	    ~LoopbackCallbacks(){}

	    void OnRead(GScale::Group *g, const GScale::Packet &packet) {
	    	std::cout << "node '" << (packet.getReceiver().getAlias()) << "' ";
	    	std::cout << "received '" << packet.payload() << "' ";
	    	std::cout << "from '" << (packet.getSender().getAlias()) << "'" << std::endl;

	        this->handleCommunication(g, &packet.getSender(), &packet.getReceiver());
	    }

	    void OnNodeAvailable(GScale::Group *g, const GScale::INode *src, const GScale::INode *dst) {
	    	std::cout << "node '" << (dst->getAlias()) << "' has been notified that ";
	    	std::cout << "'" << (src->getAlias()) << "' has become available" << std::endl;

	        this->handleCommunication(g, src, dst);
	    }
	    void OnNodeUnavailable(GScale::Group *g, const GScale::INode *src, const GScale::INode *dst) {
	    	std::cout << "node '" << (dst->getAlias()) << "' has been notified that ";
	    	std::cout << "'" << (src->getAlias()) << "' has become unavailable" << std::endl;

	        this->handleCommunication(g, src, dst);

	    }
	protected:
	    void handleCommunication(GScale::Group *group, const GScale::INode *src, const GScale::INode *dst) {
	    	std::string srca = src->getAlias();
	    	if(srca == boost::lexical_cast<std::string>(src->getNodeUUID())){
	    		srca = "";
	    	}
	    	std::string dsta = dst->getAlias();
	    	if(dsta == boost::lexical_cast<std::string>(dst->getNodeUUID())){
	    		dsta = "";
	    	}

	    	std::multimap<std::string, std::string>::iterator it = this->node2message.find(dsta+"->"+srca);
	    	//printf("%s:%d - handle comm %s\n", __FILE__,__LINE__, dst->getAlias().c_str());
	        if (it == this->node2message.end()) {
	            return;
	        }

	        //printf("using msg: %s\n", it->second.c_str());
	        if (it->second.length() > 0) {
	        	// NOTE: we need to switch dst/src because we're sending a new message
	        	// so the event destination becomes the packet's source
	            GScale::Packet data(*dst, *src);
	            data.payload(it->second);

	            group->write(data);
	        }
	        this->node2message.erase(it);
	    }

	    std::multimap<std::string, std::string> node2message;
};

int main(int argc, char** argv){
	GScale::Group *g = new GScale::Group("chat-example");
	g->attachBackend<GScale::Backend::Loopback>();

	LoopbackCallbacks cbs;
    /* add nodes to group */
    /* since connect triggers the available callback via the localloopback
     * one can create some kind of conversation just by adding nodes ^^
     * not sure if this could be of any use
     *
     * but this example shows that the local loopback backend doesnt need a GScale_work(); call
     * to do its magic
     */
	const GScale::LocalNode carol = g->connect("Carol", cbs);
	const GScale::LocalNode carlos = g->connect("Carlos", cbs);
	const GScale::LocalNode nameless = g->connect(cbs);
	g-> runWorker();

	g->disconnect(carlos);
	g->disconnect(nameless);
	g->disconnect(carol);
	g-> runWorker();

    return 0;
}
