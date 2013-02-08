/*
 * groupcore.hpp
 *
 */

#ifndef UTIL_GROUPCORE_HPP_
#define UTIL_GROUPCORE_HPP_

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

#include "packet.hpp"

///////////////// Packet ///////////////////////

struct PacketRow{
	    PacketRow(PacketPtr packet){
	    	this->packet = packet;
	    }

	    uint16_t count() const { return this->backendref.size(); }
	    uint32_t bytes() const { return this->packet->size(); }

	    PacketPtr packet;

	    std::set<std::string> backendref;
};
namespace Index{
struct packet{};
struct packet_count{};
struct packet_bytes{};

typedef boost::multi_index_container<
  PacketRow,
  boost::multi_index::indexed_by<
      boost::multi_index::sequenced<
          boost::multi_index::tag<packet>
      >,
      boost::multi_index::ordered_non_unique<
          boost::multi_index::tag<packet_count>,
          boost::multi_index::const_mem_fun<PacketRow,const uint16_t,&PacketRow::count>
      >,
      boost::multi_index::ordered_non_unique<
          boost::multi_index::tag<packet_bytes>,
          boost::multi_index::const_mem_fun<PacketRow,const uint32_t,&PacketRow::bytes>
      >
  >
> PacketSet;

typedef PacketSet::index<packet>::type PacketSet_packet;
typedef PacketSet::index<packet_count>::type PacketSet_count;
typedef PacketSet::index<packet_bytes>::type PacketSet_bytes;
};

//////////////// LocalNode ///////////////////

struct LocalNodeRow{
	    LocalNodeRow(uint64_t id, LocalNode node, boost::posix_time::ptime time_connect) : node(node){
	    	this->id = id;
	    	this->time_connect = time_connect;
	    	this->backendrefcount = 0;
	    }

	    const boost::uuids::uuid nodeuuid() const { return node.getNodeUUID(); }

	    uint64_t id;

	    LocalNode node;

	    boost::posix_time::ptime time_connect;
	    boost::posix_time::ptime time_disconnect;
	    uint16_t backendrefcount;
};
namespace Index{
// define a multiply indexed set with indices by id and name
struct node_id{};
struct node_time_connect{};
struct node_time_disconnect{};
struct node_nodeuuid{};
struct node_brefcnt{};

typedef boost::multi_index_container<
  LocalNodeRow,
  boost::multi_index::indexed_by<
      boost::multi_index::ordered_unique<
          boost::multi_index::tag<node_id>,
          boost::multi_index::member<LocalNodeRow, uint64_t, &LocalNodeRow::id>
      >,
      boost::multi_index::ordered_non_unique<
          boost::multi_index::tag<node_time_connect>,
          boost::multi_index::member<LocalNodeRow, boost::posix_time::ptime, &LocalNodeRow::time_connect>
      >,
      boost::multi_index::ordered_non_unique<
          boost::multi_index::tag<node_time_disconnect>,
          boost::multi_index::member<LocalNodeRow, boost::posix_time::ptime, &LocalNodeRow::time_disconnect>
      >,
      boost::multi_index::ordered_unique<
          boost::multi_index::tag<node_nodeuuid>,
          boost::multi_index::const_mem_fun<LocalNodeRow,const boost::uuids::uuid,&LocalNodeRow::nodeuuid>
      >,
      boost::multi_index::ordered_non_unique<
          boost::multi_index::tag<node_brefcnt>,
          boost::multi_index::member<LocalNodeRow, uint16_t, &LocalNodeRow::backendrefcount>
      >
  >
> LocalNodeSet;

typedef LocalNodeSet::index<node_id>::type LocalNodeSet_id;
typedef LocalNodeSet::index<node_time_connect>::type LocalNodeSet_time_connect;
typedef LocalNodeSet::index<node_time_disconnect>::type LocalNodeSet_time_disconnect;
typedef LocalNodeSet::index<node_nodeuuid>::type LocalNodeSet_uuid;
typedef LocalNodeSet::index<node_brefcnt>::type LocalNodeSet_brefcnt;

};

///////////////// Backend ///////////////////////

struct BackendRow {
	    BackendRow(GScale::Backend::IBackend *backend, std::string type){
	    	this->backend = backend;
	    	this->enabled = false;
	    	this->type = type;
	    }

	    void enable(){ this->enabled = true; }
	    void disable(){ this->enabled = false; }
	    uint64_t bytes() const {
	    	uint64_t b = 0;

	    	std::vector<PacketPtr>::iterator it = this->sendqueue.begin();
	    	for(;it!=this->sendqueue.end();it++){
	    		b += (*it)->size();
	    	}

	    	return b;
	    }

	    GScale::Backend::IBackend *backend;
	    std::string type;
	    bool enabled;

	    std::vector<PacketPtr> sendqueue;
};
namespace Index{

struct backend_type{};
struct backend_enabled{};

typedef boost::multi_index_container<
  BackendRow,
  boost::multi_index::indexed_by<
      boost::multi_index::ordered_unique<
          boost::multi_index::tag<backend_type>,
          boost::multi_index::member<BackendRow, std::string, &BackendRow::type>
      >,
      boost::multi_index::ordered_non_unique<
          boost::multi_index::tag<backend_enabled>,
          boost::multi_index::member<BackendRow, bool, &BackendRow::enabled>
      >
  >
> BackendSet;

typedef BackendSet::index<idxbackend_type>::type BackendSetIdx_type;
typedef BackendSet::index<idxbackend_enabled>::type BackendSetIdx_enabled;

};

#endif /* UTIL_GROUPCORE_HPP_ */
