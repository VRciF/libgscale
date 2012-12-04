/*
 * shm.hpp
 *
 */

#ifndef SHM_HPP_
#define SHM_HPP_

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

		/* called when a node becomes available */
		void OnLocalNodeAvailable(const GScale::INode *node,
				                  const GScale::LocalNodes &localnodes);
		/* called when a local node becomes unavailable */
		void OnLocalNodeUnavailable(const GScale::INode *node,
				                    const GScale::LocalNodes &localnodes);

		/* called when a local node writes data to the group */
		unsigned int OnLocalNodeWritesToGroup(const GScale::Packet &packet,
				                              const GScale::LocalNodes &localnodes);

		void Worker(struct timeval *timeout);
	private:
		typedef boost::interprocess::allocator<int, boost::interprocess::managed_shared_memory::segment_manager>  ShmemAllocator;
		typedef boost::circular_buffer<char, ShmemAllocator> ShmCircBuff;

		GScale::Group *group;
		std::string ipckey;
		boost::interprocess::managed_shared_memory *shm;
		ShmCircBuff *shm_circular_buffer;
};

}

}

#endif /* SHM_HPP_ */
