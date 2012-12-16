/*
 * shm.cpp
 *
 */

#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <iostream>

#include "shm.hpp"

namespace GScale{

namespace Backend{

SharedMemory::SharedMemory(GScale::Group *group){
	this->shm = NULL;
	this->group = group;
	//bool created = false;
	std::cout << __FILE__ << ":" << __LINE__ << std::endl;

	this->ipckey = this->group->getName();

	{
		boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, (this->ipckey+":shminit").c_str());
		boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
		// initialize shared memory segment
		// at this point we can be sure that no other process
		// is accessing the segment for intialization purpose
		// so either the segment already exists and is in use - so we just attach
		// or we need to create a new segment - thus we can safely initialize it

		try{
			this->shm = new boost::interprocess::managed_shared_memory
			   (boost::interprocess::create_only                  //only create
			   ,this->ipckey.c_str()              //name
			   ,3 * 1024 * 1024     // size
			   );

			//created = true;
		}catch(boost::interprocess::interprocess_exception &ex){
		    std::cout << __FILE__ << ":" << __LINE__ << ex.what() << std::endl;
			this->shm = new boost::interprocess::managed_shared_memory
			   (boost::interprocess::open_or_create_t()               //open or create
			   ,this->ipckey.c_str()              //name
			   ,3 * 1024 * 1024     // size
			   );
	    }

		try{

		    ShmemAllocator alloc_inst (this->shm->get_segment_manager());
		    this->shm_circular_buffer  = this->shm->construct<ShmCircBuff>((this->ipckey+":circular_buffer").c_str())(alloc_inst);
/*
		    this->shmregion = boost::interprocess::mapped_region
		    		(this->shm
		    		,boost::interprocess::read_write
		    		);
		    if(created){
		    	// initialize to zero
		        std::memset(this->shmregion.get_address(), 0, this->shmregion.get_size());
		    }
		    */
		}catch(boost::interprocess::interprocess_exception &ex){
			// MISSING: throw GScale::Exception
		    std::cout << __FILE__ << ":" << __LINE__ << ex.what() << std::endl;
		}
	}
	std::cout << __FILE__ << ":" << __LINE__ << std::endl;
}
SharedMemory::~SharedMemory(){
	try{
		boost::interprocess::shared_memory_object::remove((this->ipckey+":circular_buffer").c_str());
	}catch(...){}
}

void SharedMemory::OnLocalNodeAvailable(const GScale::INode *node,
                          const GScale::LocalNodes &localnodes){

}
/* called when a local node becomes unavailable */
void SharedMemory::OnLocalNodeUnavailable(const GScale::INode *node,
                            const GScale::LocalNodes &localnodes){

}

/* called when a local node writes data to the group */
unsigned int SharedMemory::OnLocalNodeWritesToGroup(const GScale::Packet &packet,
                                      const GScale::LocalNodes &localnodes){
    return 0;
}


void SharedMemory::Worker(struct timeval *timeout){
    /* testing shared memory */
    std::cout << __FILE__ << ":" << __LINE__ << std::endl;
}

}

}
