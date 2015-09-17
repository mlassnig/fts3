/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <boost/type_traits/is_base_of.hpp>
#include <boost/static_assert.hpp>
#include <queue>

#include <stdsoap2.h>

#include "common/error.h"
#include "common/MonitorObject.h"

namespace fts3 {
namespace server {

class GSoapRequestHandler;


class GSoapAcceptor: public fts3::common::MonitorObject
{

public:
    GSoapAcceptor(const unsigned int port, const std::string& ip);
    GSoapAcceptor(const fts3::server::GSoapAcceptor&);
    virtual ~GSoapAcceptor();

    soap* getSoapContext();
    void recycleSoapContext(soap* ctx);

    std::unique_ptr<GSoapRequestHandler> accept();

protected:

    soap* ctx;
    std::queue<soap*> recycle;

public:
    mutable boost::recursive_mutex _mutex;
};

} // end namespace server
} // end namespace fts3
