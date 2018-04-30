//
// message_delete.hpp
// ******************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/message.hpp"
#include "base_event.hpp"
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct message_delete : public base_event
{
    snowflake message_id; /**<\todo Needs documentation */
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    message_delete(snowflake m, channel * c) : message_id(m), _channel(c) {};
    channel * const _channel; /**<\todo Needs documentation */
#else
    message_delete(snowflake m) : message_id(m) {}
#endif
};

}

}

}
