//
// fwd.hpp
// *******
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#pragma once

namespace aegis
{
namespace ratelimit
{
template<typename Callable, typename Result>
class ratelimit_mgr;
template<typename Callable, typename Result>
class bucket;
template<typename Callable, typename Result>
class bucket_factory;
}
namespace rest
{
class rest_controller;
class rest_reply;
}
namespace shards
{
class shard;
class shard_mgr;
}

class core;
class channel;
class guild;
class member;
class shard;

}
