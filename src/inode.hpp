/*
 * inode.hpp
 *
 */

#ifndef INODE_HPP_
#define INODE_HPP_

#include <string>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>


namespace GScale{

class INode{
	public:
	    INode();
        virtual ~INode();
	    const boost::uuids::uuid getNodeUUID() const;

	    std::string getAlias() const;
	    virtual bool isLocal() = 0;

	    inline bool operator== (const INode &b) const;
	protected:
		boost::uuids::uuid nodeuuid;
		std::string alias;
};

}

#endif /* INODE_HPP_ */
