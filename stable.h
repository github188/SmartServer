#ifndef STABLE_H
#define STABLE_H

#include <string>
#include <stdexcept>
#include <algorithm>
#include <list>
#include <deque>
#include <set>
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/thread/condition.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/threadpool.hpp>
#include <boost/smart_ptr/detail/atomic_count.hpp>
#include <boost/network/include/http/server.hpp>
#include <boost/network/protocol/http/response.hpp>
#include <boost/network/tags.hpp>
#include <boost/network/message_fwd.hpp>
#include <boost/network/message/wrappers.hpp>
#include <boost/network/uri/detail/uri_parts.hpp>
#include <boost/network/message/directives.hpp>
#include <boost/network/message/transformers.hpp>
#include <boost/network/protocol/http/request.hpp>
#include <boost/network/protocol/http/server/sync_server.hpp>
#include <boost/network/protocol/http/server/async_server.hpp>
#include <boost/network/utils/thread_pool.hpp>
#include "./net/config.h"
using namespace boost::asio;
#endif
