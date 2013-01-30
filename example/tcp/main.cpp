/*
 * main.cpp
 *
 */

#include <string>
#include <iostream>
#include <map>
#include <boost/uuid/uuid_io.hpp>
#include <gscale.hpp>

class TCPCallbacks : public GScale::INodeCallback{
    public:
        TCPCallbacks(){
        }
        ~TCPCallbacks(){}

        void OnRead(GScale::Group *g, const GScale::Packet &packet) {
            std::cout << "node '" << (packet.getReceiver().getAlias()) << "' ";
            std::cout << "received '" << packet.payload() << "' ";
            std::cout << "from '" << (packet.getSender().getAlias()) << "'" << std::endl;

            //this->handleCommunication(g, packet.getSender(), packet.getReceiver());
        }

        void OnNodeAvailable(GScale::Group *g, const GScale::INode *src,
                             const GScale::INode *dst) {
            std::cout << "node '" << (dst->getAlias()) << "' has been notified that ";
            std::cout << "'" << (src->getAlias()) << "' has become available" << std::endl;

            //this->handleCommunication(g, src, dst);
        }
        void OnNodeUnavailable(GScale::Group *g, const GScale::INode *src,
                               const GScale::INode *dst) {
            std::cout << "node '" << (dst->getAlias()) << "' has been notified that ";
            std::cout << "'" << (src->getAlias()) << "' has become unavailable" << std::endl;

            //this->handleCommunication(g, src, dst);

        }
    protected:
        /*
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
                GScale::Packet data(dst, src);
                data.setPayload(it->second);

                group->write(data);
            }
            this->node2message.erase(it);
        }

        std::multimap<std::string, std::string> node2message;
        */
};

int main(int argc, char** argv){
    LOG();

    GScale::Group *g = new GScale::Group("networkchat-example");
    g->attachBackend<GScale::Backend::TCP>();

    TCPCallbacks cbs;

    //const GScale::LocalNode *carol = g->connect("Carol", cbs);
    //const GScale::LocalNode *carlos = g->connect("Carlos", cbs);
    //const GScale::LocalNode *nameless = g->connect(cbs);
    //g->runWorker(NULL);

    //g->disconnect(carlos);
    //g->disconnect(nameless);
    //g->disconnect(carol);
    while(1){
        g->runWorker(NULL);
        sleep(1);
    }

    return 0;
}
