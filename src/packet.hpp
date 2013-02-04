/*
 * packet.hpp
 *
 */

#ifndef PACKET_HPP_
#define PACKET_HPP_

#include <string>

#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>

#include "inode.hpp"

namespace GScale{


class Packet{
    public:
        enum METATYPE {MIN=0, NODEAVAIL, NODEUNAVAIL, DATA, MAX};

	    Packet(const GScale::INode sender = GScale::INode::getNilNode(), const GScale::INode receiver = GScale::INode::getNilNode());
	    virtual ~Packet();

	    const GScale::INode& getSender() const;
	    const GScale::INode& getReceiver() const;

	    const std::string& payload(const std::string payload);
	    const std::string& payload() const;
        unsigned int size() const;

        Packet::METATYPE type(Packet::METATYPE t=MIN);

        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int /*version*/) const
        {
            int t = this->type_;
            // note, version is always the latest when saving
            ar & this->sender;
            ar & this->receiver;
            ar & t;
        }
        template<class Archive>
        void load(Archive & ar, const unsigned int /*version*/)
        {
            int t=0;
            //if(version > 0) ...
            ar & this->sender;
            ar & this->receiver;
            ar & t;
            this->type_ = static_cast<METATYPE>(t);
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()

	protected:
	    GScale::INode sender;
	    GScale::INode receiver;

        std::string payload_;
	    METATYPE type_;
};

}

BOOST_CLASS_VERSION(GScale::Packet, 1)

#endif /* PACKET_HPP_ */
