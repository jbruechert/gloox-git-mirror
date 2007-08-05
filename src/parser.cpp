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

#include "util.h"
#include "parser.h"

namespace gloox
{

  Parser::Parser( TagHandler *ph )
    : m_tagHandler( ph ), m_current( 0 ), m_root( 0 ), m_state( Initial ),
      m_preamble( 0 ), m_quote( false )
  {
  }

  Parser::~Parser()
  {
    delete m_root;
  }

  int Parser::feed( const std::string& data )
  {
    int i = 0;
    std::string::const_iterator it = data.begin();
    for( ; it != data.end(); ++it, ++i )
    {
      const unsigned char c = (*it);
//       printf( "found char:   %c, ", c );

      if( !isValid( c ) )
      {
        cleanup();
        return i;
      }

      switch( m_state )
      {
        case Initial:
//           printf( "Initial: %c\n", c );
          m_tag = "";
          if( isWhitespace( c ) )
            break;

          switch( c )
          {
            case '<':
              m_state = TagOpening;
              break;
            case '>':
            default:
              if( m_current )
              {
                m_cdata += c;
                m_state = TagInside;
              }
//               cleanup();
//               return i;
              break;
          }
          break;
        case TagOpening:               // opening '<' has been found before
//           printf( "TagOpening: %c\n", c );
          if( isWhitespace( c ) )
            break;

          switch( c )
          {
            case '<':
            case '>':
              cleanup();
              return i;
              break;
            case '/':
              m_state = TagClosingSlash;
              break;
            case '?':
              m_state = TagNameCollect;
              m_preamble = 1;
              break;
            case '!':
              if( m_tag.empty() && it + 7 != data.end() && data.substr( i, 8 ) == "![CDATA[" )
              {
                it += 7;
                i += 7;
                m_state = TagCDATASection;
              }
              else
              {
                cleanup();
                return i;
              }
              break;
            default:
              m_tag += c;
              m_state = TagNameCollect;
              break;
          }
          break;
        case TagCDATASection:
          switch( c )
          {
            case ']':
              if( it + 2 != data.end() && data.substr( i, 3 ) == "]]>" )
              {
                it += 2;
                i += 2;
                addCData();
                m_state = TagInside;
                break;
              } // fall-through intended
            default:
            m_cdata += c;
            break;
          }
          break;
        case TagNameCollect:          // we're collecting the tag's name, we have at least one octet already
//           printf( "TagNameCollect: %c\n", c );
          if( isWhitespace( c ) )
          {
            m_state = TagNameComplete;
            break;
          }

          switch( c )
          {
            case '<':
            case '?':
              cleanup();
              return i;
              break;
            case '/':
              m_state = TagOpeningSlash;
              break;
            case '>':
              addTag();
              m_state = TagInside;
              break;
            default:
              m_tag += c;
              break;
          }
          break;
        case TagInside:                // we're inside a tag, expecting a child tag or cdata
//           printf( "TagInside: %c\n", c );
          m_tag = "";
          switch( c )
          {
            case '<':
              addCData();
              m_state = TagOpening;
              break;
            default:
              m_cdata += c;
              break;
          }
          break;
        case TagOpeningSlash:         // a slash in an opening tag has been found, initing close of the tag
//           printf( "TagOpeningSlash: %c\n", c );
          if( isWhitespace( c ) )
            break;

          if( c == '>' )
          {
            addTag();
            if( !closeTag() )
            {
              cleanup();
              return i;
            }

            m_state = Initial;
          }
          else
          {
            cleanup();
            return i;
          }
          break;
        case TagClosingSlash:         // we have found the '/' of a closing tag
//           printf( "TagClosingSlash: %c\n", c );
          if( isWhitespace( c ) )
            break;

          switch( c )
          {
            case '>':
            case '<':
            case '/':
              cleanup();
              return i;
              break;
            default:
              m_tag += c;
              m_state = TagClosing;
              break;
          }
          break;
        case TagClosing:               // we're collecting the name of a closing tag
//           printf( "TagClosing: %c\n", c );
          switch( c )
          {
            case '<':
            case '/':
              cleanup();
              return i;
              break;
            case '>':
              if( !closeTag() )
              {
                cleanup();
                return i;
              }

              m_state = Initial;
              break;
            default:
              m_tag += c;
              break;
          }
          break;
        case TagNameComplete:        // a tag name is complete, expect tag close or attribs
//           printf( "TagNameComplete: %c\n", c );
          if( isWhitespace( c ) )
            break;

          switch( c )
          {
            case '<':
              cleanup();
              return i;
              break;
            case '/':
              m_state = TagOpeningSlash;
              break;
            case '>':
              if( m_preamble == 1 )
              {
                cleanup();
                return i;
              }
              m_state = TagInside;
              addTag();
              break;
            case '?':
              if( m_preamble == 1 )
                m_preamble = 2;
              else
              {
                cleanup();
                return i;
              }
              break;
            default:
              m_attrib += c;
              m_state = TagAttribute;
              break;
          }
          break;
        case TagAttribute:                  // we're collecting the name of an attribute, we have at least 1 octet
//           printf( "TagAttribute: %c\n", c );
          if( isWhitespace( c ) )
          {
            m_state = TagAttributeComplete;
            break;
          }

          switch( c )
          {
            case '<':
            case '/':
            case '>':
              cleanup();
              return i;
              break;
            case '=':
              m_state = TagAttributeEqual;
              break;
            default:
              m_attrib += c;
          }
          break;
        case TagAttributeComplete:         // we're expecting an equals sign or ws or the attrib value
//           printf( "TagAttributeComplete: %c\n", c );
          if( isWhitespace( c ) )
            break;

          switch( c )
          {
            case '=':
              m_state = TagAttributeEqual;
              break;
            case '<':
            case '/':
            case '>':
            default:
              cleanup();
              return i;
              break;
          }
          break;
        case TagAttributeEqual:            // we have found an equals sign
//           printf( "TagAttributeEqual: %c\n", c );
          if( isWhitespace( c ) )
            break;

          switch( c )
          {
            case '"':
              m_quote = true;
            case '\'':
              m_state = TagValue;
              break;
            case '=':
            case '<':
            case '>':
            default:
              cleanup();
              return i;
              break;
          }
          break;
        case TagValue:                 // we're expecting value data
//           printf( "TagValue: %c\n", c );
          switch( c )
          {
            case '<':
              cleanup();
              return i;
              break;
            case '\'':
              if( m_quote )
              {
                m_value += c;
                break;
              }
            case '"':
              addAttribute();
              m_state = TagNameComplete;
              m_quote = false;
              break;
            case '>':
            default:
              m_value += c;
          }
          break;
        default:
//           printf( "default action!?\n" );
          break;
      }
//       printf( "parser state: %d\n", m_state );
    }

    return -1;
  }

  void Parser::addTag()
  {
    if( !m_root )
    {
//       printf( "created Tag named %s, ", m_tag.c_str() );
      m_root = new Tag( 0, relax( m_tag ) );
      m_current = m_root;
    }
    else
    {
//       printf( "created Tag named %s, ", m_tag.c_str() );
      m_current = new Tag( m_current, relax( m_tag ) );
    }

    if( m_attribs.size() )
    {
      m_current->setAttributes( m_attribs );
//       printf( "added %d attributes, ", m_attribs.size() );
      m_attribs.clear();
    }

    if( m_tag == "stream:stream" )
    {
      streamEvent( m_root );
      cleanup();
    }
//     else
//       printf( "%s, ", m_root->xml().c_str() );

    if( m_tag == "xml" && m_preamble == 2 )
      cleanup();
  }

  void Parser::addAttribute()
  {
//     printf( "adding attribute: %s='%s', ", m_attrib.c_str(), m_value.c_str() );
    m_attribs.push_back( new Tag::Attribute( relax( m_attrib ), relax( m_value ) ) );
    m_attrib = "";
    m_value = "";
//     printf( "added, " );
  }

  void Parser::addCData()
  {
    if( m_current && !m_cdata.empty() )
    {
      m_current->addCData( m_state == TagCDATASection ? m_cdata : relax( m_cdata ) );
//       printf( "added cdata %s to %s: %s\n",
//               m_cdata.c_str(), m_current->name().c_str(), m_current->xml().c_str() );
      m_cdata = "";
    }
  }

  bool Parser::closeTag()
  {
//     printf( "about to close, " );

    if( m_tag == "stream:stream" )
      return true;

    if( !m_current || m_current->name() != m_tag )
      return false;

//       printf( "m_current: %s, ", m_current->name().c_str() );
//       printf( "m_tag: %s, ", m_tag.c_str() );

    if( m_current->parent() )
      m_current = m_current->parent();
    else
    {
//       printf( "pushing upstream\n" );
      streamEvent( m_root );
      cleanup();
    }

    return true;
  }

  void Parser::cleanup()
  {
    delete m_root;
    m_root = 0;
    m_current = 0;
    m_cdata = "";
    m_tag = "";
    m_attrib = "";
    m_value = "";
    util::clear( m_attribs );
    m_attribs.clear();
    m_state = Initial;
    m_preamble = 0;
  }

  bool Parser::isValid( unsigned char c )
  {
    return ( c != 0xc0 || c != 0xc1 || c < 0xf5 );
  }

  bool Parser::isWhitespace( unsigned char c )
  {
    return ( c == 0x09 || c == 0x0a || c == 0x0d || c == 0x20 );
  }

  void Parser::streamEvent( Tag *tag )
  {
    if( m_tagHandler )
      m_tagHandler->handleTag( tag );
  }

  static const char escape_chars[] = { '&', '<', '>', '\'', '"', '<', '>',
    '\'', '"', '<', '>', '<', '>', '\'', '"', '<', '>', '<', '>', '\'', '"' };

  static const std::string escape_seqs[] = { "amp;", "lt;", "gt;", "apos;",
    "quot;", "#60;", "#62;", "#39;", "#34;", "#x3c;", "#x3e;", "#x3C;",
    "#x3E;", "#x27;", "#x22;", "#X3c;", "#X3e;", "#X3C;", "#X3E;", "#X27;",
    "#X22;" };

  static const unsigned nb_escape = sizeof(escape_chars)/sizeof(char);
  static const unsigned escape_size = 5;

  /*
  * When a sequence is found, do not repack the string directly, just set
  * the new symbol and mark the rest for deletation (0).
  */
  const std::string Parser::relax( std::string esc )
  {
    const unsigned int l = esc.length();
    unsigned int p = 0;
    unsigned int i = 0;

    for( unsigned int val; i < l; ++i )
    {
      if( esc[i] != '&' )
        continue;

      for( val = 0; val < nb_escape; ++val )
      {
        if( ( i + escape_seqs[val].length() <= l )
              && !strncmp( esc.data()+i+1, escape_seqs[val].data(),
                           escape_seqs[val].length() ) )
        {
          esc[i] = escape_chars[val];
          for( p=1; p <= escape_seqs[val].length(); ++p )
            esc[i+p] = 0;
          i += p-1;
          break;
        }
      }
    }
    if( p )
    {
      for( p = 0, i = 0; i < l; ++i )
      {
        if( esc[i] != 0 )
        {
          if( esc[p] == 0 )
          {
            esc[p] = esc[i];
            esc[p+1] = 0;
          }
          ++p;
        }
      }
      esc.resize( p );
    }
    return esc;
  }

}
