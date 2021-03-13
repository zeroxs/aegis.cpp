//
// defines.hpp
// ***********
//
// Copyright (c) 2021 Sharon Fox (sharon at sharonfox dot dev)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include <cstdint>

/// Gateway intents for masking out events on the websocket.
/// Use as a bitfield with create_bot_t::intents().
// https://github.com/discordapp/discord-api-docs/pull/1307
enum intent : uint32_t {
    IntentsDisabled = 0xffffffff,	/* Special case, disables intents if none have been defined */
    Guilds = (1 << 0),
    GuildMembers = (1 << 1),
    GuildBans = (1 << 2),
    GuildEmojis = (1 << 3),
    GuildIntegrations = (1 << 4),
    GuildWebhooks = (1 << 5),
    GuildInvites = (1 << 6),
    GuildVoiceStates = (1 << 7),
    GuildPresences = (1 << 8),
    GuildMessages = (1 << 9),
    GuildMessageReactions = (1 << 10),
    GuildMessageTyping = (1 << 11),
    DirectMessages = (1 << 12),
    DirectMessageReactions = (1 << 13),
    DirectMessageTyping = (1 << 14)
};
