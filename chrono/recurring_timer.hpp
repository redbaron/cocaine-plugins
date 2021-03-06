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

#ifndef COCAINE_RECURRING_TIMER_DRIVER_HPP
#define COCAINE_RECURRING_TIMER_DRIVER_HPP

#include <cocaine/api/driver.hpp>

#include <cocaine/asio/reactor.hpp>

namespace cocaine { namespace driver {

using io::reactor_t;

class recurring_timer_t:
    public api::driver_t
{
    public:
        typedef api::driver_t category_type;

    public:
        recurring_timer_t(context_t& context,
                          reactor_t& reactor,
                          app_t& app,
                          const std::string& name,
                          const Json::Value& args);

        virtual
        ~recurring_timer_t();

        virtual
        Json::Value
        info() const;

        void
        enqueue(const api::event_t& event,
                const std::shared_ptr<api::stream_t>& stream);

        virtual
        void
        reschedule();

    private:
        void
        on_event(ev::timer&, int);

    protected:
        context_t& m_context;
        std::unique_ptr<logging::log_t> m_log;

        app_t& m_app;

        const std::string m_event;
        const double m_interval;

        ev::timer m_watcher;
};

}}

#endif
