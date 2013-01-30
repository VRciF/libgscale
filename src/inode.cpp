/*
 * inode.cpp
 *
 */

#include "inode.hpp"

#include <arpa/inet.h>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/uuid/string_generator.hpp>

#include "core.hpp"

namespace GScale{

INode::INode() : hostuuid(Core::getInstance()->getHostUUID()), nodeuuid(boost::uuids::random_generator()()){
    this->ctime = boost::posix_time::microsec_clock::universal_time();
}
INode::INode(std::string alias) : hostuuid(Core::getInstance()->getHostUUID()), nodeuuid(boost::uuids::random_generator()()), alias(alias){
    this->ctime = boost::posix_time::microsec_clock::universal_time();
}
INode::INode(boost::uuids::uuid uuid) : hostuuid(Core::getInstance()->getHostUUID()), nodeuuid(uuid){
    this->ctime = boost::posix_time::microsec_clock::universal_time();
}
INode::INode(boost::uuids::uuid uuid, std::string alias) : hostuuid(Core::getInstance()->getHostUUID()), nodeuuid(uuid), alias(alias){
    this->ctime = boost::posix_time::microsec_clock::universal_time();
}

INode::INode(boost::uuids::uuid hostuuid, boost::uuids::uuid uuid) : hostuuid(hostuuid), nodeuuid(uuid){
    this->ctime = boost::posix_time::microsec_clock::universal_time();
}

INode::~INode(){}

const INode& INode::getNilNode(){
    static const INode n(boost::uuids::nil_uuid(), boost::uuids::nil_uuid());
    return n;
}

bool INode::isLocal() const{
    return this->hostuuid == Core::getInstance()->getHostUUID();
}

const boost::uuids::uuid INode::getNodeUUID() const{
	return this->nodeuuid;
}
const boost::uuids::uuid INode::getHostUUID() const{
    return this->hostuuid;
}

std::string INode::getAlias() const{
	if(this->alias.length()<=0){
		return boost::lexical_cast<std::string>(this->nodeuuid);
	}
	return this->alias;
}

boost::posix_time::ptime INode::created() const{
    return this->ctime;
}
boost::posix_time::ptime INode::created(boost::posix_time::ptime ctime){
    if(!ctime.is_not_a_date_time()){
        this->ctime = ctime;
    }
    return this->ctime;
}

inline bool INode::operator== (const INode &b) const
{
    return b.nodeuuid==this->nodeuuid;
}

void INode::saveUUID(const boost::uuids::uuid &source, unsigned char (&result)[16]){
    memcpy(result, &source, 16);

    *((uint32_t*)(&result[0])) = htonl(*((uint32_t*)(&result[0])));
    *((uint32_t*)(&result[4])) = htonl(*((uint32_t*)(&result[4])));
    *((uint32_t*)(&result[8])) = htonl(*((uint32_t*)(&result[8])));
    *((uint32_t*)(&result[12])) = htonl(*((uint32_t*)(&result[12])));
}
void INode::loadUUID(boost::uuids::uuid &result, unsigned char (&source)[16]){
    *((uint32_t*)(&source[0])) = ntohl(*((uint32_t*)(&source[0])));
    *((uint32_t*)(&source[4])) = ntohl(*((uint32_t*)(&source[4])));
    *((uint32_t*)(&source[8])) = ntohl(*((uint32_t*)(&source[8])));
    *((uint32_t*)(&source[12])) = ntohl(*((uint32_t*)(&source[12])));

    memcpy(source, &result, 16);
}


}
