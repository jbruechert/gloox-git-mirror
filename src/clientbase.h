/*
  Copyright (c) 2005 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warrenty.
*/



#ifndef CLIENTBASE_H__
#define CLIENTBASE_H__

#include "gloox.h"

#include "connectionlistener.h"
#include "iqhandler.h"
#include "messagehandler.h"
#include "presencehandler.h"
#include "rosterlistener.h"
#include "subscriptionhandler.h"
#include "loghandler.h"
#include "taghandler.h"
#include "jid.h"


namespace gloox
{

  class string;
  class map;
  class list;
  class Connection;
  class Packet;
  class Tag;
  class Stanza;
  class Parser;

  /**
   * @brief This is the common base class for a jabber Client and a jabber Component.
   * It manages connection establishing, authentication, filter registrationa and invocation.
   * @author Jakob Schroeter <js@camaya.net>
   * @since 0.3
   */
  class ClientBase
  {

    friend class Parser;

    public:

      /**
       * Constructs a new ClientBase.
       * You should not need to use this class directly. Use Client or Component instead.
       * @param ns The namespace which qualifies the stream. Either jabber:client or jabber:component:*
       * @param server The server to connect to.
       * @param port The port to connect to. The default of -1 means to look up the port via DNS SRV
       * or to use a default port of 5222 as defined in XMPP: Core.
       */
      ClientBase( const std::string& ns, const std::string& server, int port = -1 );

      /**
       * Constructs a new ClientBase.
       * You should not need to use this class directly. Use Client or Component instead.
       * @param ns The namespace which qualifies the stream. Either jabber:client or jabber:component:*
       * @param password The password to use for further authentication.
       * @param server The server to connect to.
       * @param port The port to connect to. The default of -1 means to look up the port via DNS SRV
       * or to use a default port of 5222 as defined in XMPP: Core.
       */
      ClientBase( const std::string& ns, const std::string& password,
                  const std::string& server, int port = -1 );

      /**
       * Virtual destrcuctor.
       */
      virtual ~ClientBase();

      /**
       * Initiates the connection to a server. This function blocks as long as a connection is
       * established.
       * You can have the connection block 'til the end of the connection, or you can have it return
       * immediately. If you choose the latter, its your responsibility to call @ref recv() every now
       * and then to actually receive data from the socket and to feed the parser.
       * @param block @b True for blocking, @b false for non-blocking connect. Defaults to @b true.
       * @return @b False if prerequisits are not met (server not set) or if the connection was refused,
       * @b true otherwise.
       */
      bool connect( bool block = true );

      /**
       * Use this periodically to receive data from the socket and to feed the parser. You need to use
       * this only if you chose to connect in non-blocking mode.
       * @param timeout The timeout to use for select. Default of -1 means blocking until data was available.
       * @return The state of the connection.
       */
      ConnectionError recv( int timeout = -1 );

      /**
       * Disconnects from the server.
       */
      void disconnect();

      /**
       * Reimplement this function to provide a username for connection purposes.
       * @return The username.
       */
      virtual const std::string username() const = 0;

      /**
       * Returns the current jabber id.
       * @return The Jabber ID.
       * @note I you change the server part of the JID, the server of the connection is not synced.
       * You have to do that manually using @ref setServer().
       */
      JID& jid() { return m_jid; };

      /**
       * Switches usage of SASL on/off. Default: on
       * @param sasl Whether to switch SASL usage on or off.
       */
      void setSasl( bool sasl ) { m_sasl = sasl; };

      /**
       * Switches usage of TLS on/off (if available). Default: on
       * @param tls Whether to switch TLS usage on or off.
       */
      void setTls( bool tls ) { m_tls = tls; };

      /**
       * Sets the port to connect to. This is not necessary if either the default port (5222) is used
       * or SRV records exist which will be resolved.
       * @param port The port to connect to.
       */
      void setPort( int port ) { m_port = port; };

      /**
       * Sets the XMPP server to connect to.
       * @param server The server to connect to. Either IP or fully qualified domain name.
       * @note If you change the server, the server part of the JID is not synced. You have to do that
       * manually using @ref jid() and @ref JID::setServer().
       */
      void setServer( const std::string &server ) { m_server = server; };

      /**
       * Sets the password to use to connect to the XMPP server.
       * @param password The password to use for authentication.
       */
      void setPassword( const std::string &password ) { m_password = password; };

      /**
       * Returns the current prepped server.
       * @return The server used to connect.
       */
      const std::string server() const { return m_server; };

      /**
       * Returns the current SASL status.
       * @return The current SASL status.
       */
      bool sasl() const { return m_sasl; };

      /**
       * Returns the current TLS status.
       * @return The current TLS status.
       */
      bool tls() const { return m_tls; };

      /**
       * Returns the port. The default of -1 means that the actual port will be looked up using
       * SRV records, or the XMPP default port of 5222 will be used.
       * @return The port used to connect.
       */
      int port() const { return m_port; };

      /**
       * Returns the current password.
       * @return The password used to connect.
       */
      virtual const std::string password() const { return m_password; };

      /**
       * Creates a string which is unique in the current instance and
       * can be used as an ID for queries.
       * @return A unique string suitable for query IDs.
       */
      const std::string getID();

      /**
       * Sends a given Tag over an established connection.
       * @param tag The Tag to send.
       */
      virtual void send( Tag *tag );

      /**
       * Returns whether authentication has taken place and was successful.
       * @return @b True if authentication has been carried out @b and was successful, @b false otherwise.
       */
      bool authed() const { return m_authed; };

      /**
       * Returns the current connection status.
       * @return The status of the connection.
       */
      ConnectionState state() const;

      /**
       * Retrieves the value of the xml:lang attribute of the initial stream.
       * Default is 'en', i.e. if not changed by a call to @ref setXmlLang().
       */
      const std::string& xmlLang() const { return m_xmllang; };

      /**
       * Sets the value for the xml:lang attribute of the initial stream.
       * @param xmllang The language identifier for the stream. It must conform to
       * section 2.12 of the XML specification and RFC 3066.
       * Default is 'en'.
       */
      void setXmlLang( const std::string& xmllang ) { m_xmllang = xmllang; };

      /**
       * Registers @c cl as object that receives connection notifications.
       * @param cl The object to receive connection notifications.
       */
      void registerConnectionListener( ConnectionListener *cl );

      /**
       * Registers @c ih as object that receives Iq stanza notifications for namespace
       * @c xmlns. Only one handler per namespace is possible.
       * @param ih The object to receive Iq stanza notifications.
       * @param xmlns The namespace the object handles.
       */
      void registerIqHandler( IqHandler *ih, const std::string& xmlns );

      /**
       * Use this function to be notified of incoming IQ stanzas with the given value of the @b id
       * attribute.
       * Since IDs are supposed to be unique, this notification works only once.
       * @param ih The IqHandler to receive notifications.
       * @param id The id to track.
       * @param context A value that allows for restoring context.
       */
      void trackID( IqHandler *ih, const std::string& id, int context );

      /**
       * Registers @c mh as object that receives Message stanza notifications.
       * @param mh The object to receive Message stanza notifications.
       */
      void registerMessageHandler( MessageHandler *mh );

      /**
       * Registers @c ph as object that receives Presence stanza notifications.
       * @param ph The object to receive Presence stanza notifications.
       */
      void registerPresenceHandler( PresenceHandler *ph );

      /**
       * Registers @c sh as object that receives Subscription stanza notifications.
       * @param sh The object to receive Subscription stanza notifications.
       */
      void registerSubscriptionHandler( SubscriptionHandler *sh );

      /**
       * Registers @c lh as object that receives all XML sent back and forth on the connection.
       * Suitable for logging to a file, etc.
       * @param lh The object to receive exchanged data.
       */
      void registerLogHandler( LogHandler *lh );

      /**
       * Registers @c th as object that receives incoming packts with a given root tag
       * qualified by the given namespace.
       * @param th The object to receive Subscription packet notifications.
       * @param tag The element's name.
       * @param xmlns The element's namespace.
       */
      void registerTagHandler( TagHandler *th, const std::string& tag, const std::string& xmlns );

      /**
       * Removes the given object from the list of connection listeners.
       * @param cl The object to remove from the list.
       */
      void removeConnectionListener( ConnectionListener *cl );

      /**
       * Removes the handler for the given namespace from the list of Iq handlers.
       * @param xmlns The namespace to remove from the list.
       */
      void removeIqHandler( const std::string& xmlns );

      /**
       * Removes the given object from the list of message handlers.
       * @param mh The object to remove from the list.
       */
      void removeMessageHandler( MessageHandler *mh );

      /**
       * Removes the given object from the list of presence handlers.
       * @param ph The object to remove from the list.
       */
      void removePresenceHandler( PresenceHandler *ph );

      /**
       * Removes the given object from the list of subscription handlers.
       * @param sh The object to remove from the list.
       */
      void removeSubscriptionHandler( SubscriptionHandler *sh );

      /**
       * Removes the given object from the list of tag handlers for the given element and namespace.
       * @param th The object to remove from the list.
       * @param tag The element to remove the handler for.
       * @param xmlns The namespace qualifying the element.
       */
      void removeTagHandler( TagHandler *th, const std::string& tag, const std::string& xmlns );

      /**
       * Removes the given object from the list of log handlers.
       * @param lh The object to remove from the list.
       */
      void removeLogHandler( LogHandler *lh );

      /**
       * Use this function to set a number of trusted root CA certificates. which shall be
       * used to verify a servers certificate.
       * @param cacerts A list of absolute paths to CA root certificate files in PEM format.
       */
      void setCACerts( const StringList& cacerts ) { m_cacerts = cacerts; };

      /**
       * Use this function to retrieve the type of the stream error after it occurs and you received a
       * ConnectionError of type CONN_STREAM_ERROR from the ConnectionListener.
       */
      StreamError streamError() const { return m_streamError; };

      /**
       * Returns the text of a stream error for the given language if available.
       * If the requested language is not available, the default text (without a xml:lang
       * attribute) will be returned.
       * @param lang The language identifier for the desired language. It must conform to
       * section 2.12 of the XML specification and RFC 3066. If empty, the default body
       * will be returned, if any.
       * @return The describing text of a stream error. Empty if no stream error occured.
       */
      const std::string streamErrorText( const std::string& lang = "default" ) const;

      /**
       * In case the defined-condition element of an stream error contains XML character data you can
       * use this function to retrieve it. RFC 3920 only defines one condition (see-other-host)where
       * this is possible.
       * @return The cdata of the stream error's text element (only for see-other-host).
       */
      const std::string streamErrorCData() const { return m_streamErrorCData; };

      /**
       * This function can be used to retrieve the application-specific error condition of a stream error.
       * @return The application-specific error element of a stream error. 0 if no respective element was
       * found or no error occured.
       */
      Tag* streamErrorAppCondition() { return m_streamErrorAppCondition; };

      /**
       * Use this function to retrieve the type of the authentication error after it occurs and you
       * received a ConnectionError of type CONN_AUTHENTICATION_FAILED from the ConnectionListener.
       * @return The type of the authentication, if any, AUTH_ERROR_UNDEFINED otherwise.
       */
      AuthenticationError authError() const { return m_authError; };

    protected:
      enum SaslMechanisms
      {
        SASL_DIGEST_MD5,          /**< SASL Digest-MD5 according to RFC 2831. */
        SASL_PLAIN,               /**< SASL PLAIN according to RFC 2595 Section 6. */
        SASL_ANONYMOUS,           /**< SASL ANONYMOUS according to draft-ietf-sasl-anon-05.txt/
                                   * RFC 2245 Section 6. */
      };

      void notifyOnResourceBindError( ConnectionListener::ResourceBindError error );
      void notifyOnSessionCreateError( ConnectionListener::SessionCreateError error );
      bool notifyOnTLSConnect( const CertInfo& info );
      void log( const std::string& xml, bool incoming );
      void notifyOnConnect();
      void disconnect( ConnectionError reason );
      void header();
      void setAuthed( bool authed ) { m_authed = authed; };
      void setAuthFailure( AuthenticationError e ) { m_authError = e; };
      virtual bool checkStreamVersion( const std::string& version );

      void startSASL( SaslMechanisms type );
      void processSASLChallenge( const std::string& challenge );
      void processSASLError( Stanza *stanza );
      void startTls();
      bool hasTls();

      JID m_jid;
      Connection *m_connection;

      std::string m_password;
      std::string m_namespace;
      std::string m_xmllang;
      std::string m_server;
      std::string m_sid;
      bool m_authed;
      bool m_sasl;
      bool m_tls;
      int m_port;

    private:
      enum NodeType
      {
        NODE_STREAM_START,             /**< The &lt;stream:stream&gt; tag. */
        NODE_STREAM_ERROR,             /**< The &lt;stream:error&gt; tag. */
        NODE_STREAM_CLOSE,             /**< The &lt;/stream:stream&gt; tag. */
        NODE_STREAM_CHILD,             /**< Everything else. */
      };

      virtual void handleStartNode() = 0;
      virtual bool handleNormalNode( Stanza *stanza ) = 0;
      void handleStreamError( Stanza *stanza );

      void notifyIqHandlers( Stanza *stanza );
      void notifyMessageHandlers( Stanza *stanza );
      void notifyPresenceHandlers( Stanza *stanza );
      void notifySubscriptionHandlers( Stanza *stanza );
      void notifyTagHandlers( Stanza *stanza );
      void notifyLogHandlers( const std::string& xml, bool incoming );
      void notifyOnDisconnect( ConnectionError e );
      void filter( NodeType type, Stanza *stanza );
      void logEvent( const char *data, size_t size, int is_incoming );
      void send( const std::string& xml );

      struct TrackStruct
      {
        IqHandler *ih;
        int context;
      };

      struct TagHandlerStruct
      {
        TagHandler *th;
        std::string xmlns;
        std::string tag;
      };

      typedef std::list<ConnectionListener*>            ConnectionListenerList;
      typedef std::map<const std::string, IqHandler*>   IqHandlerMap;
      typedef std::map<const std::string, TrackStruct>  IqTrackMap;
      typedef std::list<MessageHandler*>                MessageHandlerList;
      typedef std::list<PresenceHandler*>               PresenceHandlerList;
      typedef std::list<SubscriptionHandler*>           SubscriptionHandlerList;
      typedef std::list<LogHandler*>                    LogHandlerList;
      typedef std::list<TagHandlerStruct>               TagHandlerList;

      ConnectionListenerList  m_connectionListeners;
      IqHandlerMap            m_iqNSHandlers;
      IqTrackMap              m_iqIDHandlers;
      MessageHandlerList      m_messageHandlers;
      PresenceHandlerList     m_presenceHandlers;
      SubscriptionHandlerList m_subscriptionHandlers;
      LogHandlerList          m_logHandlers;
      TagHandlerList          m_tagHandlers;
      StringList              m_cacerts;

      Parser *m_parser;

      AuthenticationError m_authError;
      StreamError m_streamError;
      StringMap m_streamErrorText;
      std::string m_streamErrorCData;
      Tag *m_streamErrorAppCondition;
      int m_idCount;

  };

};

#endif // CLIENTBASE_H__
