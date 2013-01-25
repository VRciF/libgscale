/*
 * protocol.hpp
 *
 *  Created on: Jan 25, 2013
 *      Author: tux
 */

#ifndef PROTOCOL_HPP_
#define PROTOCOL_HPP_

#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>

namespace GScale{

namespace Backend{

template<class AssemblyLine>
class TCP_Protocol{
    public:
        TCP_Protocol(boost::asio::ip::tcp::socket &socket_, AssemblyLine &al) : socket_(socket_),al(al) {
            this->outstate = this->instate = TRANSFER.FREE;
            this->outbuffer = boost::circular_buffer<char>(8192);

            boost::system::error_code ec;
            this->onRead(ec, 0);
        }
        ~TCP_Protocol(){}

        void onRead(const boost::system::error_code& error, std::size_t bytes_transferred){
            if(error){
                this->al.onReadError(error);
            }else{
                // process read buffer
                this->processReadBuffer(bytes_transferred);
            }

            if(this->socket_.is_open()){
                this->socket_.async_read_some(boost::asio::buffer(this->inputbuffer, sizeof(this->inputbuffer)),
                    boost::bind(&TCP_Protocol<AssemblyLine>::onRead, this, boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
            }
        }

        template<typename Packet>
        void onSend(Packet &p, const boost::system::error_code& error, std::size_t bytes_transferred){
            if(error){
                this->al.onReadError(error);
            }
            else{
                if(bytes_transferred>0){
                    this->outbuffer.erase_begin(bytes_transferred);
                }
                this->alreadysent += bytes_transferred; // includes header size
                switch(this->outstate){
                    case TRANSFER.FREE:
                        if(this->outbuffer.reserved() < 4){
                            break;
                        }
                        this->outstate = TRANSFER.HEADER;
                    case TRANSFER.HEADER:
                        uint32t size = htonl(this->al.size(p));
                        char *csize = (char*)&size;
                        this->outbuffer.insert(this->outbuffer.end(),csize, csize + sizeof(size));
                        this->alreadysent = 0;
                        this->outstate = TRANSFER.BODY;
                        break;
                    case TRANSFER.BODY:
                        uint32_t todo = this->al.size(p);
                        uint32_t fillsize = this->outbuffer.reserved();
                        if(fillsize > todo-this->alreadysent-4){ // 4 = headersize
                            fillsize = todo-this->alreadysent-4;
                        }

                        if(fillsize>0){
                            this->al.fillOutBuffer(this->outbuffer, fillsize);
                        }
                        else{
                            // everything sent
                            this->outstate = TRANSFER.FREE;
                            this->al.finished(p);
                            return;
                        }
                        break;
                }
            }

            if(this->socket_.is_open()){
                this->asiooutbuffers.clear();

                array_range ar = buff.array_one();
                if(ar.second>0){
                    this->asiooutbuffers.push_back(boost::asio::buffer(ar.first, ar.second));
                }
                ar = buff.array_two();
                if(ar.second>0){
                    this->asiooutbuffers.push_back(boost::asio::buffer(ar.first, ar.second));
                }

                this->socket_.async_write_some(this->asiooutbuffers,
                    boost::bind(&TCP_Protocol<AssemblyLine>::onSend, this, p, boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
            }
        }

        template<typename Packet> void send(Packet &p){
            if(outstate != TRANSFER.FREE){
                throw GScale::Exception("socket is in transmission state", EFAULT, __FILE__, __LINE__);
            }
            boost::system::error_code ec;
            this->onSend(p, ec, 0);
        }

    protected:
        void processReadBuffer(std::size_t bytes_transferred){
            while(bytes_transferred>0){
                switch(this->inputstate){
                    case TRANSFER.FREE: // this means, we're expecting a header at first
                        this->inputstate = TRANSFER.HEADER;
                        this->alreadyread = 0;
                    case TRANSFER.HEADER:
                        uint8_t pendingheadread = 4-this->alreadyread;
                        if(bytes_transferred<pendingheadread){
                            pendingheadread = bytes_transferred;
                        }
                        memcpy(((char*)&this->inframesize)+this->alreadyread, this->inbuffer, pendingheadread);
                        memove();
                        memmove(this->inputbuffer, this->inputbuffer+pendingheadread, sizeof(this->inbuffer)-pendingheadread);

                        bytes_transferred -= pendingheadread;
                        this->alreadyread += pendingheadread;
                        if(this->alreadyread<4){ // no more data to read
                            return;
                        }
                        else{
                            this->inframesize = ntohl(this->inframesize); // header has been finished
                            this->inputstate = TRANSFER.BODY;
                            this->alreadyread = 0;
                            this->al.resetPacket();
                        }
                    break;
                    case TRANSFER.BODY:
                        uint32_t appendsize = this->inframesize;
                        appendsize -= (this->alreadyread);  // 4 = header size

                        if(bytes_transferred<appendsize){
                            appendsize = bytes_transferred;
                        }
                        this->al.appendToPacket(this->inbuffer, appendsize);
                        bytes_transferred -= appendsize;
                        this->alreadyread += appendsize;
                        if(this->alreadyread==this->inframesize){
                            this->al.finishPacket();
                            this->inputstate = TRANSFER.FREE;
                            this->alreadyread = 0;
                        }
                    break;
                }
            }
        }

        AssemblyLine &al;
        boost::asio::ip::tcp::socket &socket_;

        enum TRANSFER { MIN=0, FREE, HEADER, BODY, MAX} instate, outstate;

        uint32_t alreadysent;
        uint32_t alreadyread;

        char inbuffer[8192];
        uint32_t inframesize;

        std::vector<boost::asio::const_buffer> asiooutbuffers;
        boost::circular_buffer<char> outbuffer;
};

}

}

#endif /* PROTOCOL_HPP_ */
