/*
 * session.cpp
 *
 */

#include "session.hpp"

namespace GScale{

namespace Backend{

TCP_Session::TCP_Session(TCP *backend,boost::asio::io_service& io_service) : socket_(io_service)
{
    this->backend = backend;
}

boost::asio::ip::tcp::socket& TCP_Session::socket()
{
    return this->socket_;
}

void TCP_Session::start(){
    this->io_service.dispatch(boost::bind(&TCP_Session::syncNodeList, it->second));
}
void TCP_Session::close(){
    this->socket_.close();
    if(this->onclose){
        this->onclose();
    }
}

void TCP_Session::syncNodeList(){
    std::pair<GScale::Group::LocalNodesSetIdx_creationtime::iterator, GScale::Group::LocalNodesSetIdx_creationtime::iterator> range;

    if(this->nodesyncctime.is_not_a_date_time()){ // start sync
        this->state = STATES.SYNC;
        range = this->backend->gdao->rangeByCreated();
    }
    else{
        range = this->backend->gdao->rangeByCreated(this->nodesyncctime);
    }

    if(range.first==range.second){
        this->state = STATES.AVAIL;
        return;
    }

    for(;range.first != range.second; range.first++){
        // send node avail packet
    }
}

boost::asio::ip::tcp::endpoint TCP_Session::remote_endpoint() const{
    return this->socket_.remote_endpoint();
}
boost::asio::ip::tcp::endpoint TCP_Session::local_endpoint() const{
    return this->socket_.local_endpoint();
}


}

}
