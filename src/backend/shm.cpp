/*
 * shm.cpp
 *
 */

#include "shm.hpp"

#include <iostream>

#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/uuid/uuid_generators.hpp>

#include <boost/chrono.hpp>
#include <boost/crc.hpp>  // for boost::crc_32_type
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "group.hpp"

namespace GScale{

namespace Backend{

SharedMemory::SharedMemory(){
    this->shm = NULL;
    this->group = NULL;
}
void SharedMemory::initialize(GScale::Group *group, GScale::GroupNodesDAO *gdao){
    this->group = group;
    this->gdao = gdao;

    //unsigned int memsize = 3 * 1024 * 1024;
    unsigned int memsize = 5;
    //bool created = false;
    std::cout << __FILE__ << ":" << __LINE__ << std::endl;

    this->ipckey = this->group->getName();

    {
        try{
        boost::interprocess::named_mutex::remove((this->ipckey+":shminit").c_str());
        }catch(...){}

        boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, (this->ipckey+":shminit").c_str());
        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex, boost::posix_time::second_clock::universal_time()+boost::posix_time::seconds(2));

        if(!lock){
            throw std::string("failed");
        }
        // initialize shared memory segment
        // at this point we can be sure that no other process
        // is accessing the segment for intialization purpose
        // so either the segment already exists and is in use - so we just attach
        // or we need to create a new segment - thus we can safely initialize it

        try{
            this->shm = new boost::interprocess::managed_shared_memory
               (boost::interprocess::create_only                  //only create
               ,this->ipckey.c_str()              //name
               ,memsize     // size
               );

            try{
                boost::interprocess::shared_memory_object::remove(this->ipckey.c_str());
                std::cout << "" << std::endl;
            }catch(...){
                std::cout << "" << std::endl;
            }

            std::cout << __FILE__ << ":" << __LINE__ << "shm created" << std::endl;
        }catch(boost::interprocess::interprocess_exception &ex){
            std::cout << __FILE__ << ":" << __LINE__ << ex.what() << std::endl;
            this->shm = new boost::interprocess::managed_shared_memory
               (boost::interprocess::open_or_create_t()               //open or create
               ,this->ipckey.c_str()              //name
               ,memsize     // size
               );

            try{
                boost::interprocess::shared_memory_object::remove(this->ipckey.c_str());
                std::cout << __LINE__ << std::endl;
            }catch(...){
                std::cout << __LINE__ << std::endl;
            }

            std::cout << __FILE__ << ":" << __LINE__ << "shm found" << std::endl;
        }
        std::cout << __LINE__ << std::endl;
        try{

            std::cout << __LINE__ << std::endl;
            ShmemAllocator alloc_inst (this->shm->get_segment_manager());
            std::cout << __LINE__ << std::endl;
            try{
                std::cout << __LINE__ << std::endl;
                this->shm_circular_buffer = this->shm->construct<ShmCircBuff>((this->ipckey+":circular_buffer").c_str())(alloc_inst);
                std::cout << __LINE__ << std::endl;

                return;

                this->shm_circular_buffer->push_back('a');
                this->shm_circular_buffer->push_back('b');
                this->shm_circular_buffer->push_back('c');
                this->shm_circular_buffer->push_back('d');
                this->shm_circular_buffer->push_back('e');

                std::cout << __LINE__ << "circ buff reserve: " << this->shm_circular_buffer->reserve() << std::endl;
                std::cout << __LINE__ << "circ buff empty: " << this->shm_circular_buffer->empty() << std::endl;
                std::cout << __LINE__ << "circ buff full: " << this->shm_circular_buffer->full() << std::endl;
            }
            catch(...){
                std::cout << __LINE__ << std::endl;
                this->shm_circular_buffer  = this->shm->find_or_construct<ShmCircBuff>((this->ipckey+":circular_buffer").c_str())(alloc_inst);
                std::cout << __LINE__ << std::endl;

                std::cout << __LINE__ << "circ buff reserve: " << this->shm_circular_buffer->reserve() << std::endl;
                std::cout << __LINE__ << "circ buff empty: " << this->shm_circular_buffer->empty() << std::endl;
                std::cout << __LINE__ << "circ buff full: " << this->shm_circular_buffer->full() << std::endl;

                return;

                std::cout << *this->shm_circular_buffer->begin() << std::endl;
                this->shm_circular_buffer->pop_front();
                std::cout << *this->shm_circular_buffer->begin() << std::endl;


                boost::interprocess::shared_memory_object::remove((this->ipckey+":circular_buffer").c_str());

                // rearrange read/write iterator's
                //this->shm_circular_buffer
            }
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
        this->shm->destroy<ShmCircBuff>((this->ipckey+":circular_buffer").c_str());
        boost::interprocess::shared_memory_object::remove(this->ipckey.c_str());
        std::cout << "" << std::endl;
    }catch(...){
        std::cout << "" << std::endl;
    }
}

void SharedMemory::OnLocalNodeAvailable(GScale::LocalNode node){
    /*
    SharedMemory_EventHeader evavail;

    evavail.type = SharedMemory_EventHeader::NODEAVAIL;
    evavail.srcnode = *node;
    evavail.dstnode = INode::getNilNode();
    evavail.length = 0;

    std::stringstream ss;

    boost::archive::binary_oarchive oa(ss);
    oa << evavail;
    std::string msg = ss.str();

    boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, (this->ipckey+":shmwrite").c_str());
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);

    if(this->shm_circular_buffer->reserve() >= msg.length()){
        this->shm_circular_buffer->insert(this->shm_circular_buffer->end(), msg.begin(), msg.end());
    }
    else{
        //throw new Exception();
    }

    //GScale_Backend_LocalSharedMemory_Util_NotifyMulticastEvent(storage);
     */
}
/* called when a local node becomes unavailable */
void SharedMemory::OnLocalNodeUnavailable(GScale::LocalNode node){
    /*
    SharedMemory_EventHeader evavail;

    evavail.type = SharedMemory_EventHeader::NODEUNAVAIL;
    evavail.srcnode = *node;
    evavail.dstnode = INode::getNilNode();
    evavail.length = 0;

    std::stringstream ss;

    boost::archive::binary_oarchive oa(ss);
    oa << evavail;
    std::string msg = ss.str();

    boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, (this->ipckey+":shmwrite").c_str());
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);

    if(this->shm_circular_buffer->reserve() >= msg.length()){
        this->shm_circular_buffer->insert(this->shm_circular_buffer->end(), msg.begin(), msg.end());
    }
    else{
        //throw new Exception();
    }

    //GScale_Backend_LocalSharedMemory_Util_NotifyMulticastEvent(storage);
     */
}

/* called when a local node writes data to the group */
unsigned int SharedMemory::OnLocalNodeWritesToGroup(const GScale::Packet &packet){
    return 0;
    /*
    SharedMemory_EventHeader evavail;
    unsigned int writelength = 0;

    evavail.type = SharedMemory_EventHeader::DATA;
    evavail.srcnode = *packet.getSender();
    evavail.dstnode = *packet.getReceiver();
    evavail.length = 0;

    std::stringstream ss;

    boost::archive::binary_oarchive oa(ss);
    oa << evavail;
    std::string msg = ss.str();

    boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, (this->ipckey+":shmwrite").c_str());
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);

    if(this->shm_circular_buffer->reserve() > msg.length()){
        this->shm_circular_buffer->insert(this->shm_circular_buffer->end(), msg.begin(), msg.end());

        writelength = this->shm_circular_buffer->reserve();
        std::string::const_iterator endit;

        if(writelength > packet.size()){
            endit = packet.getPayload().end();
        }
        else{
            endit = (packet.getPayload().begin() + writelength);
        }

        try{
            this->shm_circular_buffer->insert(this->shm_circular_buffer->end(), packet.getPayload().begin(), endit);
        }
        catch(...){
            // drop the already written header!
            this->shm_circular_buffer->erase_end(msg.length());
            writelength = 0;
        }
    }
    else{
        //throw new Exception();
    }

    //GScale_Backend_LocalSharedMemory_Util_NotifyMulticastEvent(storage);

    return writelength;
    */
}


void SharedMemory::Worker(){
    /* testing shared memory */
    std::cout << __FILE__ << ":" << __LINE__ << std::endl;
}

}

}

/*

#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/uuid/uuid_generators.hpp>

#include <sstream>
#include <iostream>

#include "shm.hpp"

namespace GScale{

namespace Backend{

SharedMemory::SharedMemory(GScale::Group *group){
	this->shm = NULL;
	this->group = group;


    //GScale_Backend_LocalSharedMemory_Util_CreateMulticastServer(storage);
    //GScale_Backend_LocalSharedMemory_Util_CreateMulticastClient(storage);
    //GScale_Backend_Internal_AddEventDescriptor(_this->g, storage->multicastclient, EPOLLIN);

    this->ipckey = this->group->getName();

    boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, (this->ipckey+":shminit").c_str());
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
    // initialize shared memory segment
    // at this point we can be sure that no other process
    // is accessing the segment for intialization purpose
    // so either the segment already exists and is in use - so we just attach
    // or we need to create a new segment - thus we can safely initialize it

    boost::interprocess::mapped_region region;
    try{
        this->shm_obj = shared_memory_object(boost::interprocess::create_only                  //only create
                                            ,(this->ipckey+":circular_buffer").c_str()         //name
                                            ,boost::interprocess::read_write                   //read-write mode
                                            );

        this->shm_obj.truncate(3 * 1024 * 1024);
        //Map the whole shared memory in this process
        region = boost::interprocess::mapped_region(this->shm_obj, boost::interprocess::read_write);
        //Write all the memory to 0
        std::memset(region.get_address(), 0, region.get_size());
    }catch(boost::interprocess::interprocess_exception &ex){
        this->shm_obj = shared_memory_object(boost::interprocess::open_or_create               //open or create doesnt throw
                                            ,(this->ipckey+":circular_buffer").c_str()         //name
                                            ,boost::interprocess::read_write                   //read-write mode
                                            );
        region = boost::interprocess::mapped_region(this->shm_obj, boost::interprocess::read_write);
    }

    this->circbuff = new GScale::Datastructure::CircularBuffer(region.get_address());


    //ringBuffer_initDataPointer(&storage->sharedbuffer,
    //                          shmds.shm_segsz-offsetof(GScale_Backend_LocalSharedMemory_Buffer, cshm),
    //                          ((char*)storage->shm)+offsetof(GScale_Backend_LocalSharedMemory_Buffer, cshm));

    //// grab a copy of the current state buffer
    //storage->readbuffer = storage->shm->meta;
    //tmpbuffer = storage->sharedbuffer;
    //tmpbuffer.meta = &storage->readbuffer;
    //// ignore everything that has not been read until now
    //// begin with a completely empty copy of the buffer
    ////
    //rlength = ringBuffer_getFIONREAD(&tmpbuffer);
    //if (rlength > 0) {
    //    ringBuffer_readExt(&tmpbuffer, NULL, rlength, GSCALE_RINGBUFFER_OPTION_SKIPDATA);
    //}
    //GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_MAINTENANCELOCK, 0);

}
SharedMemory::~SharedMemory(){
    delete this->circbuff;
	try{
		boost::interprocess::shared_memory_object::remove((this->ipckey+":circular_buffer").c_str());
	}catch(...){}
}

void SharedMemory::OnLocalNodeAvailable(const GScale::INode *node,
                          const GScale::LocalNodes &localnodes){
    struct _evNode evavail;

    evavail.type = NODEAVAIL;
    evavail.srchost = Core::getInstance()->getHostUUID();
    evavail.srcnode = node;
    evavail.dsthost = boost::uuids::nil_uuid();
    evavail.dstnode = NULL;
    evavail.length = 0;

    std::stringstream ss;

    boost::archive::binary_oarchive oa(ss);
    oa << evavail;
    std::string msg = ss.str();


    boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, (this->ipckey+":shmwrite").c_str());
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);

    this->circbuff->writeExt(&storage->sharedbuffer, msg, length, GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);

    GScale_Backend_LocalSharedMemory_Util_NotifyMulticastEvent(storage);

}
// called when a local node becomes unavailable
void SharedMemory::OnLocalNodeUnavailable(const GScale::INode *node,
                            const GScale::LocalNodes &localnodes){
    struct _evNode evavail;

    evavail.type = NODEUNAVAIL;
    evavail.srchost = Core::getInstance()->getHostUUID();
    evavail.srcnode = node;
    evavail.dsthost = boost::uuids::nil_uuid();
    evavail.dstnode = NULL;
    evavail.length = 0;

    std::stringstream ss;

    boost::archive::binary_oarchive oa(ss);
    oa << evavail;
    std::string msg = ss.str();

    boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, (this->ipckey+":shmwrite").c_str());
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);

    this->circbuff->writeExt(&storage->sharedbuffer, msg, length, GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);

    GScale_Backend_LocalSharedMemory_Util_NotifyMulticastEvent(storage);

}

// called when a local node writes data to the group
unsigned int SharedMemory::OnLocalNodeWritesToGroup(const GScale::Packet &packet,
                                      const GScale::LocalNodes &localnodes){
    struct _evNode evavail;

    evavail.type = NODEUNAVAIL;
    evavail.srchost = Core::getInstance()->getHostUUID();
    evavail.srcnode = node;
    evavail.dsthost = boost::uuids::nil_uuid();
    evavail.dstnode = NULL;
    evavail.length = 0;

    std::stringstream ss;

    boost::archive::binary_oarchive oa(ss);
    oa << evavail;
    std::string msg = ss.str();

    boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, (this->ipckey+":shmwrite").c_str());
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);

    writelength = ringBuffer_getFIONWRITE(&storage->sharedbuffer);
    this->circbuff->writeExt(&storage->sharedbuffer, msg, length, GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);



    GScale_Backend_LocalSharedMemory_Util_NotifyMulticastEvent(storage);

    GScale_Backend_LocalSharedMemory_Message msg;
    GScale_Backend_LocalSharedMemory_Storage *storage = NULL;
    uint16_t headersize = 0;
    uint32_t writelength = 0;
    int32_t readcnt = 0;
    void *ptr[4] = { NULL, NULL, NULL, NULL };
    int32_t lengths[4] = { 0, 0, 0, 0 };
    GScale_Exception except;

    storage = (GScale_Backend_LocalSharedMemory_Storage*) _this->backend_private;
    if (storage == NULL) {
        ThrowException("no backend initialized", EINVAL);
    }

    GScale_Backend_LocalSharedMemory_AcquireLock(storage, GSCALE_BACK_LSHM_SEM_WRITELOCK, SEM_UNDO, NULL);

    Try{
        headersize = sizeof(msg.header)+sizeof(packet->header);

        writelength = ringBuffer_getFIONWRITE(&storage->sharedbuffer);
        if (writelength > headersize) {

            readcnt = GScale_Backend_LocalSharedMemory_GetParticipantCount(storage);
            readcnt--; // decrease me
            if (readcnt <= 0) {
                return 0;
            }
            writelength -= headersize;
            writelength = writelength > packet->data.length ? packet->data.length : writelength;

            msg.header.readcnt = readcnt;
            msg.header.type.nodeavail = 0;
            msg.header.type.nodeunavail = 0;
            msg.header.type.data |= 1;
            msg.header.length = sizeof(packet->header) + writelength;

            ptr[0] = &msg.header;
            ptr[1] = &packet->header;
            ptr[2] = packet->data.buffer;
            lengths[0] = sizeof(msg.header);
            lengths[1] = sizeof(packet->header);
            lengths[2] = writelength;
            ringBuffer_multiWriteExt(&storage->sharedbuffer, ptr, lengths, GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);

        }
    }
    Catch(except){
        GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_WRITELOCK, 0);
        Throw except;
    }
    GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_WRITELOCK, 0);

    GScale_Backend_LocalSharedMemory_Util_NotifyMulticastEvent(storage);

    return writelength;
}


void SharedMemory::Worker(struct timeval *timeout){
    // testing shared memory
    std::cout << __FILE__ << ":" << __LINE__ << std::endl;

    GScale_Backend_LocalSharedMemory_Storage *storage = NULL;
    ringBufferMeta copym;
    ringBuffer copy;
    ringBufferMemMap memmap;
    GScale_Backend_LocalSharedMemory_Message msg;
    uint8_t skipit = 0;
    GScale_Packet message;
    uint32_t length = 0;
    uint32_t maxread = 0;
    char nullbuffer[1024] = { '\0' };
    GScale_Exception except;

    if (_this == NULL || _this->backend_private == NULL) ThrowInvalidArgumentException();

    storage = (GScale_Backend_LocalSharedMemory_Storage*) _this->backend_private;
    // grab read lock and read the buffer

    // just read out multicastclient
    while(read(storage->multicastclient, nullbuffer, sizeof(nullbuffer)) > 0){}

    GScale_Backend_LocalSharedMemory_AcquireLock(storage, GSCALE_BACK_LSHM_SEM_MSGREADLOCK, SEM_UNDO | IPC_NOWAIT, timeout);

    Try{
        // problem(s):
        // o) i need to separatly keep track of the readoffset or so i dont waste cpu on messages already parsed
        //    not sure how i can solve this easily, also the readoffset needs to be changed
        //    by the single last reader of the message
        //
        // solved:
        // o) messages from my own host, need to be reread again and just droped
        //    not sure if there is a more efficient way - this one is the easiest
        // o) the problem was that its not sure which message has been written
        //    e.g. in case the ringbuffer swaps from the end of the buffer to the beginning
        //    because there are 2 memcpy operations necessary or to say it in another way
        //    the cpu copies the memory not by using an atomic operation
        //
        //    the solution was that the writeoffset - and thus the remaining memory
        //    which can be read is changed by the ringBuffer_Write operation in just one line
        //    at the end of the function. this means if the writeoffset is changed
        //    the data will be sure to be readable
        //
        // read lock acquired, lets have a look
        // first we get the message header

        copym = storage->readbuffer;
        copym.writepos = storage->sharedbuffer.meta->writepos;
        copy = storage->sharedbuffer;
        copy.meta = &copym;

        if (!ringBuffer_isEmpty(&copy) &&
            ringBuffer_initMemMap(&copy, &memmap, &msg.header, sizeof(msg.header)) == GSCALE_OK &&
            ringBuffer_readExt(&copy, (char *)&msg.header, sizeof(msg.header), GSCALE_RINGBUFFER_OPTION_ALLORNOTHING) > 0) {

            if (msg.header.type.nodeavail || msg.header.type.nodeunavail) {
                if (ringBuffer_readExt(&copy, (char *)&msg.payload.id, sizeof(msg.payload.id),
                        GSCALE_RINGBUFFER_OPTION_ALLORNOTHING) > 0) {
                    if (GScale_Backend_Internal_HostUUIDEqual(
                            GScale_Backend_Internal_GetBinaryHostUUID(),
                            (const GScale_Host_UUID*) &msg.payload.id.hostuuid)) {
                        skipit = 1;
                    } else {
                        // nice a host is available

                        // add remote node
                        if (msg.header.type.nodeavail) {
                            GScale_Backend_Internal_OnNodeAvailable(_this, &msg.payload.id, NULL);
                        } else {
                            GScale_Backend_Internal_OnNodeUnavailable(_this, &msg.payload.id);
                        }
                    }
                }
            } else if (msg.header.type.data) {
                if (ringBuffer_readExt(&copy, (char *)&message.header, sizeof(message.header),
                        GSCALE_RINGBUFFER_OPTION_ALLORNOTHING) > 0) {
                    if (GScale_Backend_Internal_HostUUIDEqual(
                            GScale_Backend_Internal_GetBinaryHostUUID(),
                            (const GScale_Host_UUID*) &message.header.src.hostuuid)) {
                        skipit = 1;
                    } else {
                        message.data.length = msg.header.length - sizeof(GScale_Packet_Header);

                        // read the rest of the package and forwared as a remote packet

                        // the idea is the following
                        // im using ringbuffers dma method to get a pointer of the shared memory
                        // using a const pointer i forward the data to the data to the client code
                        // if the buffer wraps around for this only message, i will use 2 calls to
                        // clients read method
                        //

                        // 1) grab dma pointer of ringbuffer message
                        length = message.data.length;

                        while (length > 0) {
                            maxread = (uint32_t)ringBuffer_dmaRead(&copy, (void**)&message.data.buffer);
                            if (maxread <= 0) {
                                break;
                            }

                            message.data.length
                                    = maxread > message.data.length ? message.data.length
                                            : maxread;
                            ringBuffer_readExt(&copy, NULL, message.data.length,
                                    GSCALE_RINGBUFFER_OPTION_SKIPDATA);

                            GScale_Backend_Internal_SendLocalMessage(_this,
                                    &message);

                            length -= message.data.length;
                        }

                    }
                }
            }

            if(!skipit){
                msg.header.readcnt--;
                if (msg.header.readcnt <= 0) {
                    // this messages isnt needed any more, skip it
                    ringBuffer_readExt(&storage->sharedbuffer, NULL, msg.header.length + sizeof(msg.header), GSCALE_RINGBUFFER_OPTION_SKIPDATA);
                }
                ringBuffer_writeMemMap(&copy, &memmap,
                        GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);
            }

            storage->readbuffer = copym;
        }
    }
    Catch(except){
        GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_MSGREADLOCK, 0);
        Throw except;
    }

    GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_MSGREADLOCK, 0);
}


}

}
*/
