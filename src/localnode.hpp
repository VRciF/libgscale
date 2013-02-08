/*
 * localnode.hpp
 *
 */

#ifndef LOCALNODE_HPP_
#define LOCALNODE_HPP_

#include <string>
#include <utility>

#include <boost/asio.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "inode.hpp"
#include "inodecallback.hpp"

namespace GScale{

class LocalNode : public GScale::INode, GScale::INodeCallback{
    public:
        LocalNode(boost::asio::io_service io_service, GScale::INodeCallback &cbs);
        LocalNode(boost::asio::io_service io_service, std::string alias, GScale::INodeCallback &cbs);
        LocalNode(const LocalNode &lnode);
        ~LocalNode();

	    virtual void OnRead(GScale::Group *g, const GScale::Packet &packet);
	    virtual void OnNodeAvailable(GScale::Group *g, const GScale::INode *src, const GScale::INode *dst);
	    virtual void OnNodeUnavailable(GScale::Group *g, const GScale::INode *src, const GScale::INode *dst);

        GScale::INodeCallback& getCallback() const;
        void operator=(const LocalNode &source );

    protected:
	    GScale::INodeCallback &cbs;
	    boost::asio::io_service io_service;
};

typedef boost::shared_ptr<LocalNode> LocalNodePtr;

}

#endif /* LOCALNODE_HPP_ */
