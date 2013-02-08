/*
 * shm.hpp
 *
 */

#ifndef SHM_HPP_
#define SHM_HPP_

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <boost/circular_buffer.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/chrono.hpp>
#include <boost/crc.hpp>
#include <boost/serialization/split_member.hpp>

#include "ibackend.hpp"

namespace GScale{

class Group;
class INode;
class Packet;

namespace Backend{

class SharedMemory : public GScale::Backend::IBackend{
    public:
        SharedMemory();
        ~SharedMemory();

        void initialize(GScale::GroupCore *groupc, GScale::GroupNodesDAO *gdao);

        /* called when a node becomes available */
        void OnLocalNodeAvailable(GScale::LocalNode node);
        /* called when a local node becomes unavailable */
        void OnLocalNodeUnavailable(GScale::LocalNode node);

        /* called when a local node writes data to the group */
        unsigned int OnLocalNodeWritesToGroup(const boost::shared_ptr<GScale::Packet> packet);
        void Worker();
    protected:

    private:
        typedef boost::interprocess::allocator<int, boost::interprocess::managed_shared_memory::segment_manager>  ShmemAllocator;
        typedef boost::circular_buffer<char, ShmemAllocator> ShmCircBuff;

        boost::interprocess::managed_shared_memory *shm;
        ShmCircBuff *shm_circular_buffer;

        GScale::GroupCore *groupc;
        GScale::GroupNodesDAO *gdao;
        std::string ipckey;
};
/*
class SharedMemory_EventHeader{
    public:
        enum ev_types {
            EVMIN=0,

            NODEAVAIL,
            NODEUNAVAIL,
            DATA,

            EVMAX
        };
        SharedMemory_EventHeader(){
            this->signature = 0xFF;
            this->datacrc32 = 0;
            this->headercrc32 = 0;
            this->nanonow = boost::chrono::high_resolution_clock::now().time_since_epoch().count();
        }
        ~SharedMemory_EventHeader(){}
        uint32_t calculateHeaderChecksum() const{
            boost::crc_32_type result;

            result.process_bytes(&this->signature, sizeof(this->signature));
            result.process_bytes(&this->nanonow, sizeof(this->nanonow));
            result.process_bytes(&this->type, sizeof(this->type));

            std::string s;
            s = boost::uuids::to_string(this->srcnode.getHostUUID());
            result.process_bytes(s.c_str(), s.length());
            s = boost::uuids::to_string(this->srcnode.getNodeUUID());
            result.process_bytes(s.c_str(), s.length());
            result.process_bytes(this->srcnode.getAlias().c_str(), this->srcnode.getAlias().length());

            s = boost::uuids::to_string(this->dstnode.getHostUUID());
            result.process_bytes(s.c_str(), s.length());
            s = boost::uuids::to_string(this->dstnode.getNodeUUID());
            result.process_bytes(s.c_str(), s.length());
            result.process_bytes(this->dstnode.getAlias().c_str(), this->dstnode.getAlias().length());

            result.process_bytes(&this->length, sizeof(this->length));

            return result.checksum();
        }
        bool isHeaderValid(){
            return this->headercrc32 == this->calculateHeaderChecksum();
        }
        void setDataChecksum(uint32_t crc32){
            this->datacrc32 = crc32;
        }

        char signature;
        uint32_t headercrc32;
        uint32_t datacrc32;
        uint64_t nanonow;

        uint8_t type;

        INode &srcnode;
        INode &dstnode;

        uint32_t length;

    private:
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            uint32_t headercrc32 = this->calculateHeaderChecksum();
            // 0xFF byte marks the beginning of the node
            ar & signature;

            ar & headercrc32;
            ar & datacrc32;
            ar & nanonow;

            ar & type;
            ar & srcnode;
            ar & dstnode;
            ar & length;
        }
        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            // 0xFF byte marks the beginning of the node
            ar & signature;

            ar & headercrc32;
            ar & datacrc32;
            ar & nanonow;

            ar & type;
            ar & srcnode;
            ar & dstnode;
            ar & length;

            if(this->headercrc32 != this->calculateHeaderChecksum()){
                // throw exception
                throw GScale::Exception("invalid checksum during archive load", ECOMM, __FILE__, __LINE__);
            }
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
};
*/
} // Backend

} // GScale

/*

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <boost/circular_buffer.hpp>

#include "ibackend.hpp"
#include "group.hpp"
#include "inode.hpp"
#include "packet.hpp"

namespace GScale{

namespace Backend{

class SharedMemory : public GScale::Backend::IBackend{
	public:
	    SharedMemory(GScale::Group *group);
		~SharedMemory();

		// called when a node becomes available
		void OnLocalNodeAvailable(const GScale::INode *node,
				                  const GScale::LocalNodes &localnodes);
		// called when a local node becomes unavailable
		void OnLocalNodeUnavailable(const GScale::INode *node,
				                    const GScale::LocalNodes &localnodes);

		// called when a local node writes data to the group
		unsigned int OnLocalNodeWritesToGroup(const GScale::Packet &packet,
				                              const GScale::LocalNodes &localnodes);

		void Worker(struct timeval *timeout);
	protected:
		enum ev_types {
		    EVMIN=0,

		    NODEAVAIL,
		    NODEUNAVAIL,
		    DATA,

		    EVMAX
		};
		struct _evNode{
		    uint8_t type;

		    INode *srcnode;
		    INode *dstnode;

		    uint32_t length;
		    char msg[];
		};
	private:
		RingBuffer *circbuff;
		shared_memory_object shm_obj;

		GScale::Group *group;
		std::string ipckey;
};

}

}

*/

#endif /* SHM_HPP_ */
