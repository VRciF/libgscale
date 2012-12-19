/*
 * inode.hpp
 *
 */

#ifndef INODE_HPP_
#define INODE_HPP_

#include <string>

#include <boost/uuid/uuid_serialize.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>


namespace GScale{

class INode{
	public:
	    INode();
	    INode(std::string uuid);
	    INode(std::string uuid, std::string alias);
        virtual ~INode();
	    const boost::uuids::uuid getNodeUUID() const;
	    const boost::uuids::uuid getHostUUID() const;

	    std::string getAlias() const;
	    bool isLocal();

	    inline bool operator== (const INode &b) const;

	    friend class boost::serialization::access;
	    // When the class Archive corresponds to an output archive, the
	    // & operator is defined similar to <<.  Likewise, when the class Archive
	    // is a type of input archive the & operator is defined similar to >>.
	    template<class Archive>
	    void serialize(Archive & ar, const unsigned int version)
	    {
	        ar & hostuuid;
	        ar & nodeuuid;
	        ar & alias;
	    }
	protected:
		boost::uuids::uuid hostuuid;
		boost::uuids::uuid nodeuuid;
		std::string alias;
};

}

#endif /* INODE_HPP_ */
