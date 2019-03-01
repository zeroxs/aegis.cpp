//
// message_create.hpp
// ******************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/message.hpp"
#include "aegis/error.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct message_create
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    std::optional<std::reference_wrapper<aegis::user>> user; /**<\todo Reference to object of user that sent this message */
    aegis::channel & channel; /**<\todo Reference to channel object this message was sent in */
    //std::optional<std::reference_wrapper<aegis::guild>> guild; /**<\todo Reference to guild object this message was sent in if it exists */
    objects::message msg; /**<\todo Message object */

    bool has_user() const noexcept
    {
        return user.has_value();
    }
    aegis::user & get_user() const
    {
        if (has_user())
            return user.value().get();
        throw std::bad_optional_access();
    }
};

}

}

}
