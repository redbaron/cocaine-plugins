/*
    Copyright (c) 2011-2012 Andrey Sibiryov <me@kobology.ru>
    Copyright (c) 2011-2012 Other contributors as noted in the AUTHORS file.

    This file is part of Cocaine.

    Cocaine is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Cocaine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COCAINE_DEALER_DRIVER_HPP
#define COCAINE_DEALER_DRIVER_HPP

#include <cocaine/common.hpp>

#include <cocaine/api/driver.hpp>
#include <cocaine/asio/reactor.hpp>

#include "io.hpp"
#include "messaging.hpp"

namespace cocaine { namespace driver {

using io::reactor_t;

class dealer_t:
    public api::driver_t
{
    public:
        typedef api::driver_t category_type;

    public:
        dealer_t(context_t& context,
                 reactor_t& reactor,
                 app_t& app,
                 const std::string& name,
                 const Json::Value& args);

        virtual
        ~dealer_t();

        virtual
        Json::Value
        info() const;

        template<class Event, typename... Args>
        bool
        send(const std::string& route,
             Args&&... args)
        {
            on_check(m_checker, ev::PREPARE);

            return m_channel.send(route, ZMQ_SNDMORE) &&
                   m_channel.send(io::codec::pack<Event>(std::forward<Args>(args)...));
        }

    private:
        void
        on_event(ev::io&, int);

        void
        on_check(ev::prepare&, int);

        void
        process_events();

    private:
        context_t& m_context;
        std::unique_ptr<logging::log_t> m_log;

        reactor_t& m_reactor;

        app_t& m_app;

        // Configuration

        const std::string m_event;
        const std::string m_identity;

        // Dealer RPC

        io::socket_t m_channel;

        // Event loop

        ev::io m_watcher;
        ev::prepare m_checker;
};

}}

#endif
