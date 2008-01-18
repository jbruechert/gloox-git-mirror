/*
  Copyright (c) 2004-2008 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#include "disco.h"
#include "discohandler.h"
#include "dataform.h"
#include "error.h"
#include "clientbase.h"
#include "disconodehandler.h"
#include "util.h"


namespace gloox
{

  // ---- Disco::SoftwareVersion ----
  Disco::SoftwareVersion::SoftwareVersion( const std::string& name,
                                    const std::string& version,
                                    const std::string& os )
    : StanzaExtension( ExtVersion ), m_name( name ), m_version( version ), m_os( os )
                                    {
                                    }

  Disco::SoftwareVersion::SoftwareVersion( const Tag* tag )
    : StanzaExtension( ExtVersion )
  {
    if( !tag )
      return;

    Tag* t = tag->findChild( "name" );
    if( t )
      m_name = t->cdata();

    t = tag->findChild( "version" );
    if( t )
      m_version = t->cdata();

    t = tag->findChild( "os" );
    if( t )
      m_os = t->cdata();
  }

  Disco::SoftwareVersion::~SoftwareVersion()
  {
  }

  const std::string& Disco::SoftwareVersion::filterString() const
  {
    static const std::string filter = "/iq/query[@xmlns='" + XMLNS_VERSION + "']";
    return filter;
  }

  Tag* Disco::SoftwareVersion::tag() const
  {
    Tag* t = new Tag( "query" );
    t->setXmlns( XMLNS_VERSION );

    if( !m_name.empty() )
      new Tag( t, "name", m_name );

    if( !m_version.empty() )
      new Tag( t, "version", m_version );

    if( !m_os.empty() )
      new Tag( t, "os", m_os );

    return t;
  }
  // ---- Disco::SoftwareVersion ----

  // ---- Disco::Identity ----
  Disco::Identity::Identity( const Tag* tag )
  {
    if( !tag || tag->name() != "identity" )
      return;

    m_category = tag->findAttribute( "category" );
    m_type = tag->findAttribute( "type" );
    m_name = tag->findAttribute( "name" );
  }

  Tag* Disco::Identity::tag() const
  {
    if( m_category.empty() || m_type.empty() )
      return 0;

    Tag* i = new Tag( "identity" );
    i->addAttribute( "category", m_category );
    i->addAttribute( "type", m_type );

    if( !m_name.empty() )
      i->addAttribute( "name", m_name );

    return i;
  }
  // ---- ~Disco::Identity ----

  // ---- Disco::Info ----
  Disco::Info::Info( const std::string& node )
    : StanzaExtension( ExtDiscoInfo ), m_node( node ), m_form( 0 )
  {
    m_features.push_back( XMLNS_DISCO_INFO );
    m_features.push_back( XMLNS_DISCO_ITEMS );
  }

  Disco::Info::Info( const Tag* tag )
    : StanzaExtension( ExtDiscoInfo ), m_form( 0 )
  {
    if( !tag || tag->name() != "query" || tag->xmlns() != XMLNS_DISCO_INFO )
      return;

    m_node = tag->findAttribute( "node" );

    const TagList& l = tag->children();
    TagList::const_iterator it = l.begin();
    for( ; it != l.end(); ++it )
    {
      const std::string& name = (*it)->name();
      if( name == "identity" )
        m_identities.push_back( new Identity( (*it) ) );
      else if( name == "feature" && (*it)->hasAttribute( "var" ) )
        m_features.push_back( (*it)->findAttribute( "var" ) );
      else if( !m_form && name == "x" && (*it)->xmlns() == XMLNS_X_DATA )
        m_form = new DataForm( (*it) );
    }
  }

  Disco::Info::~Info()
  {
    delete m_form;
//     util::clearList( m_identities );
    // FIXME
    IdentityList::iterator it = m_identities.begin();
    IdentityList::iterator it2;
    while( it != m_identities.end() )
    {
      it2 = it++;
      delete (*it2);
      m_identities.erase( it2 );
    }
    // ~
  }

  bool Disco::Info::hasFeature( const std::string& feature ) const
  {
    StringList::const_iterator it = m_features.begin();
    for( ; it != m_features.end() && (*it) != feature; ++it )
      ;
    return it != m_features.end();
  }

  const std::string& Disco::Info::filterString() const
  {
    static const std::string filter = "/iq/query[@xmlns='" + XMLNS_DISCO_INFO + "']";
    return filter;
  }

  Tag* Disco::Info::tag() const
  {
    Tag* t = new Tag( "query", XMLNS, XMLNS_DISCO_INFO );

    if( !m_node.empty() )
      t->addAttribute( "node", m_node );

    IdentityList::const_iterator it_i = m_identities.begin();
    for( ; it_i != m_identities.end(); ++it_i )
      t->addChild( (*it_i)->tag() );

    StringList::const_iterator it_f = m_features.begin();
    for( ; it_f != m_features.end(); ++it_f )
      new Tag( t, "feature", "var", (*it_f) );

    return t;
  }
  // ---- ~Disco::Info ----

  // ---- Disco::Item ----
  Disco::Item::Item( const Tag* tag )
  {
    if( !tag || tag->name() != "item" )
      return;

    m_jid = tag->findAttribute( "jid" );
    m_jid = tag->findAttribute( "node" );
    m_name = tag->findAttribute( "name" );
  }

  Tag* Disco::Item::tag() const
  {
    if( !m_jid )
      return 0;

    Tag* i = new Tag( "item" );
    i->addAttribute( "jid", m_jid.full() );

    if( !m_node.empty() )
      i->addAttribute( "node", m_node );
    if( !m_name.empty() )
      i->addAttribute( "name", m_name );

    return i;
  }
  // ---- ~Disco::Item ----

  // ---- Disco::Items ----
  Disco::Items::Items( const std::string& node )
    : StanzaExtension( ExtDiscoItems ), m_node( node )
  {
  }

  Disco::Items::Items( const Tag* tag )
  : StanzaExtension( ExtDiscoItems )
  {
    if( !tag || tag->name() != "query" || tag->xmlns() != XMLNS_DISCO_ITEMS )
      return;

    m_node = tag->findAttribute( "node" );

    const TagList& l = tag->children();
    TagList::const_iterator it = l.begin();
    for( ; it != l.end(); ++it )
    {
      const std::string& name = (*it)->name();
      if( name == "item" )
        m_items.push_back( new Item( (*it) ) );
    }
  }

  Disco::Items::~Items()
  {
//     util::clearList( m_items );
    // FIXME
    ItemList::iterator it = m_items.begin();
    ItemList::iterator it2;
    while( it != m_items.end() )
    {
      it2 = it++;
      delete (*it2);
      m_items.erase( it2 );
    }
    // ~
  }

  const std::string& Disco::Items::filterString() const
  {
    static const std::string filter = "/iq/query[@xmlns='" + XMLNS_DISCO_ITEMS + "']";
    return filter;
  }

  Tag* Disco::Items::tag() const
  {
    Tag* t = new Tag( "query", XMLNS, XMLNS_DISCO_ITEMS );

    if( !m_node.empty() )
      t->addAttribute( "node", m_node );

    ItemList::const_iterator it_i = m_items.begin();
    for( ; it_i != m_items.end(); ++it_i )
      t->addChild( (*it_i)->tag() );

    return t;
  }
  // ---- ~Disco::Items ----

  // ---- Disco ----
  Disco::Disco( ClientBase* parent )
    : m_parent( parent )
  {
    addFeature( XMLNS_VERSION );
//     addFeature( XMLNS_DISCO_INFO ); //handled by Disco::Info now
//     addFeature( XMLNS_DISCO_ITEMS ); //handled by Disco::Info now
    if( m_parent )
    {
      m_parent->registerIqHandler( this, ExtDiscoInfo );
      m_parent->registerIqHandler( this, ExtDiscoItems );
      m_parent->registerIqHandler( this, ExtVersion );
      m_parent->registerStanzaExtension( new Disco::Info() );
      m_parent->registerStanzaExtension( new Disco::Items() );
      m_parent->registerStanzaExtension( new SoftwareVersion() );
    }
  }

  Disco::~Disco()
  {
//     util::clearList( m_identities );
    // FIXME
    IdentityList::iterator it = m_identities.begin();
    IdentityList::iterator it2;
    while( it != m_identities.end() )
    {
      it2 = it++;
      delete (*it2);
      m_identities.erase( it2 );
    }
    // ~

    if( m_parent )
    {
      m_parent->removeIqHandler( this, ExtDiscoInfo );
      m_parent->removeIqHandler( this, ExtDiscoItems );
      m_parent->removeIqHandler( this, ExtVersion );
      m_parent->removeStanzaExtension( ExtDiscoInfo );
      m_parent->removeStanzaExtension( ExtDiscoItems );
      m_parent->removeStanzaExtension( ExtVersion );
      m_parent->removeIDHandler( this );
    }
  }

  bool Disco::handleIq( IQ* iq )
  {
    switch( iq->subtype() )
    {
      case IQ::Get:
      {
        IQ re( IQ::Result, iq->from(), iq->id() );

        const SoftwareVersion* sv = iq->findExtension<SoftwareVersion>( ExtVersion );
        if( sv )
        {
          re.addExtension( new SoftwareVersion( m_versionName, m_versionVersion, m_versionOs ) );
          m_parent->send( re );
          return true;
        }

        const Info *info = iq->findExtension<Info>( ExtDiscoInfo );
        if( info )
        {
          Info *i = new Info();
          if( !info->node().empty() )
          {
            i->setNode( info->node() );
            IdentityList identities;
            StringList features;
            DiscoNodeHandlerMap::const_iterator it = m_nodeHandlers.find( info->node() );
            if( it == m_nodeHandlers.end() )
            {
              delete i;
              IQ re( IQ::Error, iq->from(), iq->id() );
              re.addExtension( new Error( StanzaErrorTypeCancel, StanzaErrorItemNotFound ) );
              m_parent->send( re );
              return true;
            }
            else
            {
              DiscoNodeHandlerList::const_iterator in = (*it).second.begin();
              for( ; in != (*it).second.end(); ++in )
              {
                IdentityList il = (*in)->handleDiscoNodeIdentities( iq->from(), info->node() );
                identities.merge( il );
                StringList fl = (*in)->handleDiscoNodeFeatures( iq->from(), info->node() );
                features.merge( fl );
              }
            }
            i->setIdentities( identities );
            i->setFeatures( features );
          }
          else
          {
            IdentityList il;
            IdentityList::const_iterator it = m_identities.begin();
            for( ; it != m_identities.end(); ++it )
            {
              il.push_back( new Identity( *(*it) ) );
            }
            i->setIdentities( il );
            i->setFeatures( m_features );
          }

          re.addExtension( i );
          m_parent->send( re );
          return true;
        }

        const Items *items = iq->findExtension<Items>( ExtDiscoItems );
        if( items )
        {
          Items *i = new Items( items->node() );
          if( !items->node().empty() )
          {
            DiscoNodeHandlerMap::const_iterator it = m_nodeHandlers.find( items->node() );
            if( it == m_nodeHandlers.end() )
            {
              delete i;
              IQ re( IQ::Error, iq->from(), iq->id() );
              re.addExtension( new Error( StanzaErrorTypeCancel, StanzaErrorItemNotFound ) );
              m_parent->send( re );
              return true;
            }
            else
            {
              ItemList itemlist;
              DiscoNodeHandlerList::const_iterator in = (*it).second.begin();
              for( ; in != (*it).second.end(); ++in )
              {
                ItemList il = (*in)->handleDiscoNodeItems( iq->from(), items->node() );
                itemlist.merge( il );
              }
              i->setItems( itemlist );
            }
          }

          re.addExtension( i );
          m_parent->send( re );
          return true;
        }
        break;
      }

      case IQ::Set:
      {
        bool res = false;
        DiscoHandlerList::const_iterator it = m_discoHandlers.begin();
        for( ; it != m_discoHandlers.end(); ++it )
        {
          if( (*it)->handleDiscoSet( iq ) )
            res = true;
        }
        return res;
        break;
      }

      default:
        break;
    }
    return false;
  }

  void Disco::handleIqID( IQ* iq, int context )
  {
    DiscoHandlerMap::iterator it = m_track.find( iq->id() );
    if( it != m_track.end() )
    {
      switch( iq->subtype() )
      {
        case IQ::Result:
          switch( context )
          {
            case GetDiscoInfo:
            {
              const Info* di = iq->findExtension<Info>( ExtDiscoInfo );
              if( di )
                (*it).second.dh->handleDiscoInfo( iq->from(), *di, (*it).second.context );
              (*it).second.dh->handleDiscoInfoResult( iq, (*it).second.context );
              break;
            }
            case GetDiscoItems:
            {
              const Items* di = iq->findExtension<Items>( ExtDiscoItems );
              if( di )
                (*it).second.dh->handleDiscoItems( iq->from(), *di, (*it).second.context );
              (*it).second.dh->handleDiscoItemsResult( iq, (*it).second.context );
              break;
            }
          }
          break;

        case IQ::Error:
        {
          (*it).second.dh->handleDiscoError( iq->from(), iq->error(), (*it).second.context );
          (*it).second.dh->handleDiscoError( iq, (*it).second.context );
          break;
        }

        default:
          break;
      }

      m_track.erase( it );
    }
  }

  void Disco::getDisco( const JID& to, const std::string& node, DiscoHandler* dh, int context,
                        IdType idType, const std::string& tid )
  {
    const std::string& id = tid.empty() ? m_parent->getID() : tid;

    IQ iq( IQ::Get, to, id );
    if( idType == GetDiscoInfo )
      iq.addExtension( new Info( node ) );
    else
      iq.addExtension( new Items( node ) );

    DiscoHandlerContext ct;
    ct.dh = dh;
    ct.context = context;
    m_track[id] = ct;
    m_parent->send( iq, this, idType );
  }

  void Disco::setVersion( const std::string& name, const std::string& version, const std::string& os )
  {
    m_versionName = name;
    m_versionVersion = version;
    m_versionOs = os;
  }

  void Disco::setIdentity( const std::string& category, const std::string& type,
                           const std::string& name )
  {
//     util::clearList( m_identities );
    // FIXME
    IdentityList::iterator it = m_identities.begin();
    IdentityList::iterator it2;
    while( it != m_identities.end() )
    {
      it2 = it++;
      delete (*it2);
      m_identities.erase( it2 );
    }
    // ~

    addIdentity( category, type, name );
  }

  void Disco::registerNodeHandler( DiscoNodeHandler* nh, const std::string& node )
  {
    m_nodeHandlers[node].push_back( nh );
  }

  void Disco::removeNodeHandler( DiscoNodeHandler* nh, const std::string& node )
  {
    DiscoNodeHandlerMap::iterator it = m_nodeHandlers.find( node );
    if( it != m_nodeHandlers.end() )
    {
      (*it).second.remove( nh );
      if( (*it).second.empty() )
        m_nodeHandlers.erase( it );
    }
  }

  void Disco::removeNodeHandlers( DiscoNodeHandler* nh )
  {
    DiscoNodeHandlerMap::iterator it = m_nodeHandlers.begin();
    DiscoNodeHandlerMap::iterator it2;
    while( it != m_nodeHandlers.end() )
    {
      it2 = it++;
      removeNodeHandler( nh, (*it2).first );
    }
  }

}
