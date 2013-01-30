/*
 * inode.hpp
 *
 */

#ifndef INODE_HPP_
#define INODE_HPP_

#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace GScale{

class INode{
	public:
	    INode();
	    INode(std::string alias);
	    INode(boost::uuids::uuid uuid);
	    INode(boost::uuids::uuid uuid, std::string alias);

        INode(boost::uuids::uuid hostuuid, boost::uuids::uuid uuid);

        virtual ~INode();

        static const INode& getNilNode();

        const boost::uuids::uuid getHostUUID() const;
	    const boost::uuids::uuid getNodeUUID() const;

	    std::string getAlias() const;
	    virtual bool isLocal() const;

        boost::posix_time::ptime created() const;
        boost::posix_time::ptime created(boost::posix_time::ptime ctime);

	    inline bool operator== (const INode &b) const;

	    // the reason i don't use uuid serialize is
	    // 1) serialize may produce great overhead in sense of raw data VS serialized data size
	    // 2) to reduce possible overhead, binary archives may be used, but this could introduce byte order problems
	    //    if the serialized data is sent over the wire to another host using different host byte order
	    friend class boost::serialization::access;
	    template<class Archive>
	    void save(Archive & ar, const unsigned int version) const
	    {
	        unsigned char uuid[16];

	        this->saveUUID(this->hostuuid, uuid);
	        ar & uuid;  // hostuuid

	        this->saveUUID(this->nodeuuid, uuid);
            ar & uuid;  // nodeuuid

            ar & alias;
	    }
	    template<class Archive>
	    void load(Archive & ar, const unsigned int version)
	    {
	        unsigned char uuid[16];

            ar & uuid;  // hostuuid
            this->loadUUID(this->hostuuid, uuid);

            ar & uuid;  // nodeuuid
            this->loadUUID(this->nodeuuid, uuid);

            ar & alias;
	    }
	    BOOST_SERIALIZATION_SPLIT_MEMBER();

	protected:
	    void saveUUID(const boost::uuids::uuid &source, unsigned char (&result)[16]);
	    void loadUUID(boost::uuids::uuid &result, unsigned char (&source)[16]);

		boost::uuids::uuid hostuuid;
		boost::uuids::uuid nodeuuid;
		std::string alias;

	    boost::posix_time::ptime ctime;
};

}

BOOST_CLASS_VERSION(GScale::INode, 1)

#endif /* INODE_HPP_ */
