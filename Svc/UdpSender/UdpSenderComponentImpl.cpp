// ====================================================================== 
// \title  UdpSenderImpl.cpp
// \author tcanham
// \brief  cpp file for UdpSender component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the Office
// of Technology Transfer at the California Institute of Technology.
// 
// This software may be subject to U.S. export control laws and
// regulations.  By accepting this document, the user agrees to comply
// with all U.S. export laws and regulations.  User has the
// responsibility to obtain export licenses, or other export authority
// as may be required before exporting such information to foreign
// countries or providing access to foreign persons.
// ====================================================================== 


#include <Svc/UdpSender/UdpSenderComponentImpl.hpp>
#include "Fw/Types/BasicTypes.hpp"
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

//#define DEBUG_PRINT(x,...) printf(x,##__VA_ARGS__)
#define DEBUG_PRINT(x,...)

namespace Svc {

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction 
  // ----------------------------------------------------------------------

  UdpSenderComponentImpl ::
#if FW_OBJECT_NAMES == 1
    UdpSenderComponentImpl(
        const char *const compName
    ) :
      UdpSenderComponentBase(compName)
#else
    UdpSenderImpl(void)
#endif
    ,m_fd(-1)
    ,m_packetsSent(0)
    ,m_bytesSent(0)
    ,m_seq(0)
  {

  }

  void UdpSenderComponentImpl ::
    init(
        const NATIVE_INT_TYPE queueDepth,
        const NATIVE_INT_TYPE msgSize,
        const NATIVE_INT_TYPE instance
    ) 
  {
    UdpSenderComponentBase::init(queueDepth, msgSize, instance);
  }

  UdpSenderComponentImpl ::
    ~UdpSenderComponentImpl(void)
  {
      if (this->m_fd != -1) {
          close(this->m_fd);
      }
  }

  void UdpSenderComponentImpl::open(
          const char* addr,  /*!< server address */
          const char* port /*!< server port */
  ) {

      FW_ASSERT(addr);
      FW_ASSERT(port);
      // open UDP connection
      this->m_fd = socket(AF_INET, SOCK_DGRAM, 0);
      if (-1 == this->m_fd) {
          Fw::LogStringArg arg(strerror(errno));
          this->log_WARNING_HI_US_SocketError(arg);
          return;
      }

      /* fill in the server's address and data */
      memset((char*)&m_servAddr, 0, sizeof(m_servAddr));
      m_servAddr.sin_family = AF_INET;
      m_servAddr.sin_port = htons(atoi(port));
      inet_aton(addr , &m_servAddr.sin_addr);

      Fw::LogStringArg arg(addr);
      this->log_ACTIVITY_HI_US_PortOpened(arg,atoi(port));

  }



  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  void UdpSenderComponentImpl ::
    Sched_handler(
        const NATIVE_INT_TYPE portNum,
        NATIVE_UINT_TYPE context
    )
  {
      this->tlmWrite_US_BytesSent(this->m_bytesSent);
      this->tlmWrite_US_PacketsSent(this->m_packetsSent);
  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined serial input ports
  // ----------------------------------------------------------------------

  void UdpSenderComponentImpl ::
    PortsIn_handler(
        NATIVE_INT_TYPE portNum, /*!< The port number*/
        Fw::SerializeBufferBase &Buffer /*!< The serialization buffer*/
    )
  {
      // return if we never successfully created the socket
      if (-1 == this->m_fd) {
          return;
      }

      DEBUG_PRINT("PortsIn_handler: %d\n",portNum);
      Fw::SerializeStatus stat;
      m_sendBuff.resetSer();

      // serialize sequence number
      stat = m_sendBuff.serialize(this->m_seq++);
      FW_ASSERT(Fw::FW_SERIALIZE_OK == stat,stat);
      // serialize port call
      stat = m_sendBuff.serialize(static_cast<U8>(portNum));
      FW_ASSERT(Fw::FW_SERIALIZE_OK == stat,stat);
      // serialize port arguments buffer
      stat = m_sendBuff.serialize(Buffer);
      FW_ASSERT(Fw::FW_SERIALIZE_OK == stat,stat);
      // send on UDP socket
      DEBUG_PRINT("Sending %d bytes\n",m_sendBuff.getBuffLength());
      ssize_t sendStat = sendto(this->m_fd,
              m_sendBuff.getBuffAddr(),
              m_sendBuff.getBuffLength(),
              0,
              (struct sockaddr *) &m_servAddr,
              sizeof(m_servAddr));
      if (-1 == sendStat) {
          Fw::LogStringArg arg(strerror(errno));
          this->log_WARNING_HI_US_SendError(arg);
      } else {
          FW_ASSERT((int)m_sendBuff.getBuffLength() == sendStat,(int)m_sendBuff.getBuffLength(),sendStat,portNum);
          this->m_packetsSent++;
          this->m_bytesSent += sendStat;
      }
  }

#ifdef BUILD_UT
  void UdpSenderComponentImpl::UdpSerialBuffer::operator=(const Svc::UdpSenderComponentImpl::UdpSerialBuffer& other) {
      this->resetSer();
      this->serialize(other.getBuffAddr(),other.getBuffLength(),true);
  }

  UdpSenderComponentImpl::UdpSerialBuffer::UdpSerialBuffer(
          const Fw::SerializeBufferBase& other) : Fw::SerializeBufferBase() {
      FW_ASSERT(sizeof(this->m_buff)>= other.getBuffLength(),sizeof(this->m_buff),other.getBuffLength());
      memcpy(this->m_buff,other.getBuffAddr(),other.getBuffLength());
      this->setBuffLen(other.getBuffLength());
  }

  UdpSenderComponentImpl::UdpSerialBuffer::UdpSerialBuffer(
          const UdpSenderComponentImpl::UdpSerialBuffer& other) : Fw::SerializeBufferBase() {
      FW_ASSERT(sizeof(this->m_buff)>= other.getBuffLength(),sizeof(this->m_buff),other.getBuffLength());
      memcpy(this->m_buff,other.m_buff,other.getBuffLength());
      this->setBuffLen(other.getBuffLength());
  }

  UdpSenderComponentImpl::UdpSerialBuffer::UdpSerialBuffer(): Fw::SerializeBufferBase() {

  }

#endif


} // end namespace Svc
