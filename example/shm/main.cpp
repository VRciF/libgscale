/*
 * main.cpp
 *
 */

#include <string>
#include <iostream>
#include <map>
#include <boost/uuid/uuid_io.hpp>
#include <gscale.hpp>

class ShmCallbacks : public GScale::INodeCallback{
    public:
    ShmCallbacks(){

        }
        ~ShmCallbacks(){}

        void OnRead(GScale::Group *g, const GScale::Packet &packet) {
            std::cout << "node '" << (packet.getReceiver()->getAlias()) << "' ";
            std::cout << "received '" << packet.getPayload() << "' ";
            std::cout << "from '" << (packet.getSender()->getAlias()) << "'" << std::endl;
        }

        void OnNodeAvailable(GScale::Group *g, const GScale::INode *src,
                             const GScale::LocalNode *dst) {
            std::cout << "node '" << (dst->getAlias()) << "' has been notified that ";
            std::cout << "'" << (src->getAlias()) << "' has become available" << std::endl;
        }
        void OnNodeUnavailable(GScale::Group *g, const GScale::INode *src,
                               const GScale::LocalNode *dst) {
            std::cout << "node '" << (dst->getAlias()) << "' has been notified that ";
            std::cout << "'" << (src->getAlias()) << "' has become unavailable" << std::endl;
        }
    protected:
};

int main(int argc, char** argv){
    GScale::Group *g = new GScale::Group("chat-example");
    g->attachBackend<GScale::Backend::SharedMemory>();

    ShmCallbacks cbs;
    /* add nodes to group */
    /* since connect triggers the available callback via the localloopback
     * one can create some kind of conversation just by adding nodes ^^
     * not sure if this could be of any use
     *
     * but this example shows that the local loopback backend doesnt need a GScale_work(); call
     * to do its magic
     */
    const GScale::LocalNode *carol = g->connect("Carol", cbs);
    const GScale::LocalNode *carlos = g->connect("Carlos", cbs);
    const GScale::LocalNode *nameless = g->connect(cbs);
    g-> runWorker(NULL);

    g->disconnect(carlos);
    g->disconnect(nameless);
    g->disconnect(carol);
    g-> runWorker(NULL);

    return 0;
}
