/* Copyright @ Members of the EMI Collaboration, 2013.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once

#include "active_object.h"
#include <utility>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include <boost/filesystem/operations.hpp>
#include <ctime>

#define foreach BOOST_FOREACH
namespace fs = boost::filesystem;

extern bool stopThreads;

FTS3_SERVER_NAMESPACE_START

template <typename TRAITS>
class CleanLogsHandlerActive: public TRAITS::ActiveObjectType
{
protected:
    using TRAITS::ActiveObjectType::_enqueue;

public:

    CleanLogsHandlerActive(const std::string& desc = ""):
        TRAITS::ActiveObjectType("CleanLogsHandlerActive", desc) {}

    void beat()
    {

        boost::function<void() > op = boost::bind(&CleanLogsHandlerActive::beat_impl, this);
        this->_enqueue(op);
    }

private:

    void beat_impl(void)
    {
        while (!stopThreads)
            {
                try
                    {
                        for ( boost::filesystem::recursive_directory_iterator end, dir("/var/lib/fts3/");
                                dir != end; ++dir )
                            {
                                if(!boost::filesystem::is_directory(*dir))
                                    {
                                        std::time_t t = boost::filesystem::last_write_time( *dir ) ;
                                        std::time_t now = time(NULL);

                                        double x =  difftime (now, t);
                                        if(x > 1296000)  //clean files 15 days old
                                            {
                                                FTS3_COMMON_LOGGER_NEWLOG(INFO) << " Deleting file " << *dir << " because it was created " << std::ctime( &t ) <<  commit;
                                                boost::filesystem::remove(*dir);
                                            }
                                    }
                            }

                        sleep(86400); //once a day
                    }
                catch(...)
                    {
                        sleep(86400); //once a day
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot delete old files" <<  commit;
                    }
            }
    }
};


//----------------------------
class CleanLogsActive;
struct CleanLogsTraitsActive
{
    typedef ActiveObject <ThreadPool::ThreadPool, Traced<CleanLogsActive> > ActiveObjectType;
};

/* -------------------------------------------------------------------------- */

class CleanLogsActive : public CleanLogsHandlerActive <CleanLogsTraitsActive>
{
public:
    explicit CleanLogsActive() {}
    virtual ~CleanLogsActive() {};
};

FTS3_SERVER_NAMESPACE_END