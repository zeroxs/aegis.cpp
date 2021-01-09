//
// message_create.hpp
// ******************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/message.hpp"
#include "aegis/error.hpp"
#include <optional>

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
    std::optional<std::reference_wrapper<aegis::user>> user; /**< Reference to object of user that sent this message */
    aegis::channel & channel; /**< Reference to channel object this message was sent in */
    objects::message msg; /**< Message object */

    bool has_user() const noexcept
    {
        return user.has_value();
    }
    aegis::user & get_user() const
    {
        if (has_user())
            return user.value().get();
#if defined(AEGIS_HAS_BUILTIN_OPTIONAL)
		throw std::bad_optional_access("bad optional access");
#else
		throw std::bad_optional_access();
#endif
    }
};

}

}

}
