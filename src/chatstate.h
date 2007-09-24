/*
  Copyright (c) 2007 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/

#ifndef CHATSTATE_H__
#define CHATSTATE_H__

#include "gloox.h"
#include "stanzaextension.h"

#include <string>

namespace gloox
{

  class Tag;

  /**
   * @brief An implementation of Chat State Notifications (XEP-0085) as a StanzaExtension.
   *
   * @author Vincent Thomasset
   * @author Jakob Schroeter <js@camaya.net>
   * @since 1.0
   */
  class GLOOX_API ChatState : public StanzaExtension
  {
    public:

      ChatState( const Tag* tag );

      ChatState( ChatStateType type )
        : StanzaExtension( ExtChatState ), m_state( type )
      {}

      virtual ~ChatState() {}

      ChatStateType state() const { return m_state; }

      // reimplemented from StanzaExtension
      virtual const std::string filterString() const
      {
        return "/message/active[@xmlns='" + XMLNS_CHAT_STATES + "']"
               "|/message/composing[@xmlns='" + XMLNS_CHAT_STATES + "']"
               "|/message/paused[@xmlns='" + XMLNS_CHAT_STATES + "']"
               "|/message/inactive[@xmlns='" + XMLNS_CHAT_STATES + "']"
               "|/message/gone[@xmlns='" + XMLNS_CHAT_STATES + "']"; }

      // reimplemented from StanzaExtension
        virtual StanzaExtension* newInstance( const Tag* tag ) const
        {
          return new ChatState( tag );
        }

      // reimplemented from StanzaExtension
      Tag* tag() const;

    private:

      ChatStateType m_state;

  };

}

#endif // CHATSTATE_H__
