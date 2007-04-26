/*
  Copyright (c) 2004-2007 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/



#include "gloox.h"

#include "connectiontcpbase.h"
#include "dns.h"
#include "logsink.h"
#include "prep.h"
#include "mutex.h"
#include "mutexguard.h"

#ifdef __MINGW32__
# include <winsock.h>
#endif

#if !defined( WIN32 ) && !defined( _WIN32_WCE )
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/select.h>
# include <unistd.h>
#else
# include <winsock.h>
#endif

#include <time.h>

#include <cstdlib>
#include <string>

#ifndef _WIN32_WCE
# include <sstream>
#endif

namespace gloox
{

  ConnectionTCPBase::ConnectionTCPBase( const LogSink& logInstance )
    : ConnectionBase( 0 ),
      m_logInstance( logInstance ), m_buf( 0 ), m_socket( -1 ), m_totalBytesIn( 0 ),
      m_totalBytesOut( 0 ), m_bufsize( 1024 ), m_cancel( true )
  {
    m_port = -1;
    init();
  }

  ConnectionTCPBase::ConnectionTCPBase( const LogSink& logInstance,
                                const std::string& server, int port )
    : ConnectionBase( 0 ),
      m_logInstance( logInstance ), m_buf( 0 ), m_socket( -1 ), m_totalBytesIn( 0 ),
      m_totalBytesOut( 0 ), m_bufsize( 1024 ), m_cancel( true )
  {
    m_server = prep::idna( server );
    m_port = port;
    init();
  }

  ConnectionTCPBase::ConnectionTCPBase( ConnectionDataHandler *cdh, const LogSink& logInstance,
                                const std::string& server, int port )
    : ConnectionBase( cdh ),
      m_logInstance( logInstance ), m_buf( 0 ), m_socket( -1 ), m_totalBytesIn( 0 ),
      m_totalBytesOut( 0 ), m_bufsize( 1024 ), m_cancel( true )
  {
    m_server = prep::idna( server );
    m_port = port;
    init();
  }

  void ConnectionTCPBase::init()
  {
    m_buf = (char*)calloc( m_bufsize + 1, sizeof( char ) );
    m_sendMutex = new Mutex();
    m_recvMutex = new Mutex();
  }

  ConnectionTCPBase::~ConnectionTCPBase()
  {
    delete m_sendMutex;
    delete m_recvMutex;
    cleanup();
    free( m_buf );
    m_buf = 0;
  }

  void ConnectionTCPBase::disconnect()
  {
    m_cancel = true;
  }

  bool ConnectionTCPBase::dataAvailable( int timeout )
  {
    if( m_socket < 0 )
      return true; // let recv() catch the closed fd

    fd_set fds;
    struct timeval tv;

    FD_ZERO( &fds );
    FD_SET( m_socket, &fds );

    tv.tv_sec = timeout / 1000000;
    tv.tv_usec = timeout % 1000000;

    return ( ( select( m_socket + 1, &fds, 0, 0, timeout == -1 ? 0 : &tv ) > 0 )
             && FD_ISSET( m_socket, &fds ) != 0 );
  }

  ConnectionError ConnectionTCPBase::recv( int timeout )
  {
    MutexGuard mg( m_recvMutex );

    if( m_cancel || m_socket < 0 )
      return ConnNotConnected;

    if( !dataAvailable( timeout ) )
      return ConnNoError;

#ifdef SKYOS
    int size = ::recv( m_socket, (unsigned char*)m_buf, m_bufsize, 0 );
#else
    int size = ::recv( m_socket, m_buf, m_bufsize, 0 );
#endif

    mg.unlock();

    if( size <= 0 )
    {
      ConnectionError error = ( size ? ConnIoError : ConnStreamClosed );
      if( m_handler )
        m_handler->handleDisconnect( error );
      return error;
    }

    m_buf[size] = '\0';

    m_totalBytesIn += size;
    if( m_handler )
      m_handler->handleReceivedData( std::string( m_buf, size ) );

    return ConnNoError;
  }

  ConnectionError ConnectionTCPBase::receive()
  {
    if( m_socket < 0 )
      return ConnNotConnected;

    ConnectionError err = ConnNoError;
    while( !m_cancel && ( err = recv( 10 ) ) == ConnNoError )
      ;
    return err == ConnNoError ? ConnNotConnected : err;
  }

  bool ConnectionTCPBase::send( const std::string& data )
  {
    MutexGuard mg( m_sendMutex );

    if( data.empty() || ( m_socket < 0 ) )
      return false;

    int sent = 0;
    for( size_t num = 0, len = data.length(); sent != -1 && num < len; num += len )
    {
#ifdef SKYOS
      sent = ::send( m_socket, (unsigned char*)(data.c_str()+num), len - num, 0 );
#else
      sent = ::send( m_socket, (data.c_str()+num), len - num, 0 );
#endif
    }

    mg.unlock();

    m_totalBytesOut += data.length();
    if( sent == -1 && m_handler )
      m_handler->handleDisconnect( ConnStreamClosed );

    return sent != -1;
  }

  void ConnectionTCPBase::getStatistics( int &totalIn, int &totalOut )
  {
    totalIn = m_totalBytesIn;
    totalOut = m_totalBytesOut;
  }

  void ConnectionTCPBase::cleanup()
  {
    if( m_socket >= 0 )
    {
      DNS::closeSocket( m_socket );
      m_socket = -1;
    }
    m_state = StateDisconnected;
    m_cancel = true;
  }

}