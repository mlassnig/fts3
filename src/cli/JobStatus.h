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

#ifndef CLI_JOBSTATUS_H_
#define CLI_JOBSTATUS_H_

#include "exception/cli_exception.h"

#include <time.h>

#include <tuple>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>

#include <boost/optional.hpp>
#include <boost/bind/bind.hpp>

#include <boost/property_tree/json_parser.hpp>

namespace fts3
{
namespace cli
{

class FileInfo
{
public:

    FileInfo(boost::property_tree::ptree const & t) :
        src(t.get<std::string>("source_surl")), dst(t.get<std::string>("dest_surl")), fileId(t.get<uint64_t>("file_id")),
        fileIdAvail(true), state(t.get<std::string>("file_state")), reason(t.get<std::string>("reason")), duration(0),
        nbFailures(t.get<int>("retry")), stagingDuration(0)
    {
        try {
            boost::property_tree::ptree const & r = t.get_child("retries");
            setRetries(r);
        } catch(const boost::property_tree::ptree_bad_path &) {
            // retries path may not be available
        }

        std::string finish_time = t.get<std::string>("finish_time");
        std::string start_time = t.get<std::string>("start_time");

        tm time;
        memset(&time, 0, sizeof(time));

        strptime(finish_time.c_str(), "%Y-%m-%dT%H:%M:%S", &time);
        time_t finish = timegm(&time);

        strptime(start_time.c_str(), "%Y-%m-%dT%H:%M:%S", &time);
        time_t start = timegm(&time);

        duration = (long)difftime(finish, start);

        std::string staging_start = t.get<std::string>("staging_start", std::string());
        std::string staging_finished = t.get<std::string>("staging_finished", std::string());

        if (strptime(staging_start.c_str(), "%Y-%m-%dT%H:%M:%S", &time) != NULL)
            {
                time_t staging_start_time = timegm(&time);
                time_t staging_finished_time = ::time(NULL);

                if (strptime(staging_finished.c_str(), "%Y-%m-%dT%H:%M:%S", &time) != NULL)
                    staging_finished_time = timegm(&time);

                stagingDuration = staging_finished_time - staging_start_time;
            }
    }

    void setRetries(boost::property_tree::ptree const & r)
    {
        boost::property_tree::ptree::const_iterator itr;

        retries.clear();
        for (itr = r.begin(); itr != r.end(); ++itr)
            {
                std::string reason(itr->second.get<std::string>("reason"));
                retries.push_back(reason);
            }
    }

    std::string getState() const
    {
        return state;
    }

    std::string getSource() const
    {
        return src;
    }

    std::string getDestination() const
    {
        return dst;
    }

    uint64_t getFileId() const
    {
        if (!fileIdAvail) {
            throw cli_exception("The file id is not available");
        }
        return fileId;
    }

    std::string src;
    std::string dst;
    uint64_t fileId;
    bool fileIdAvail;
    std::string state;
    std::string reason;
    long duration;
    int nbFailures;
    std::vector<std::string> retries;
    long stagingDuration;
};

class DetailedFileStatus
{
public:
    DetailedFileStatus(boost::property_tree::ptree const & t) :
        jobId(t.get<std::string>("job_id")), src(t.get<std::string>("source_surl")), dst(t.get<std::string>("dest_surl")),
        fileId(t.get<uint64_t>("file_id")), state(t.get<std::string>("file_state"))
    {

    }

    std::string jobId;
    std::string src;
    std::string dst;
    uint64_t fileId;
    std::string state;
};

class JobStatus
{
public:

    typedef std::tuple<int, int, int, int, int , int, int, int, int> JobSummary;

    JobStatus(std::string const & jobId, std::string const & status, std::string const & dn, std::string const & reason,
              std::string const & vo, std::string const & submitTime, int nbFiles, int priority,
              boost::optional<JobSummary> summary = boost::optional<JobSummary>()) :
        jobId(jobId), status(status), dn(dn), reason(reason), vo (vo), submitTime(submitTime),
        nbFiles(nbFiles), priority(priority), summary(summary)
    {

    }

    virtual ~JobStatus() {}

    void addFile(FileInfo const & file)
    {
        files.push_back(file);
    }

    std::string getStatus()
    {
        return status;
    }

    std::string jobId;
    std::string status;
    std::string dn;
    std::string reason;
    std::string vo;
    std::string submitTime;
    int nbFiles;
    int priority;

    boost::optional<JobSummary> summary;

    std::vector<FileInfo> files;
};



} /* namespace cli */
} /* namespace fts3 */

#endif /* CLI_JOBSTATUS_H_ */
