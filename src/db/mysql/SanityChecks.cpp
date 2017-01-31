/*
 * Copyright (c) CERN 2013-2016
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

#include "MySqlAPI.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include "db/generic/DbUtils.h"
#include "monitoring/msg-ifce.h"
#include "sociConversions.h"


using namespace fts3::common;
using namespace db;


static void logInconsistency(const std::string &jobId, const std::string &message)
{
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Found inconsistency for " << jobId << ": " << message << commit;
}


void MySqlAPI::fixEmptyJob(soci::session &sql, const std::string &jobId)
{
    sql << "UPDATE t_job SET "
    " job_finished = UTC_TIMESTAMP(), "
    " job_state = 'CANCELED', "
    " reason = 'The job was empty'"
    " WHERE job_id = :jobId",
    soci::use(jobId);
    logInconsistency(jobId, "The job was empty");
}


void MySqlAPI::fixNonTerminalJob(soci::session &sql, const std::string &jobId,
    uint64_t filesInJob, uint64_t cancelCount, uint64_t finishedCount, uint64_t failedCount)
{
    const std::string failed = "One or more files failed. Please have a look at the details for more information";
    const std::string canceledMessage = "Transfer canceled by the user";

    if (cancelCount > 0) {
        sql << "UPDATE t_job SET "
            "    job_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), "
            "    reason = :canceledMessage "
            "    WHERE job_id = :jobId and  job_state <> 'CANCELED' ",
            soci::use(canceledMessage), soci::use(jobId);
    }
    else if (filesInJob == finishedCount)  // All files finished
    {
        sql << "UPDATE t_job SET "
            "    job_state = 'FINISHED', job_finished = UTC_TIMESTAMP() "
            "    WHERE job_id = :jobId and  job_state <> 'FINISHED'  ",
            soci::use(jobId);
    }
    else if (filesInJob == failedCount)  // All files failed
    {
        sql << "UPDATE t_job SET "
            "    job_state = 'FAILED', job_finished = UTC_TIMESTAMP(),"
            "    reason = :failed "
            "    WHERE job_id = :jobId and  job_state <> 'FAILED' ",
            soci::use(failed), soci::use(jobId);
    }
    else   // Otherwise it is FINISHEDDIRTY
    {
        sql << "UPDATE t_job SET "
            "    job_state = 'FINISHEDDIRTY', job_finished = UTC_TIMESTAMP(), "
            "    reason = :failed "
            "    WHERE job_id = :jobId and  job_state <> 'FINISHEDDIRTY'",
            soci::use(failed), soci::use(jobId);
    }

    logInconsistency(jobId, "Non terminal job with all its files terminal");
}


/// Search for jobs in non terminal state for which all transfers are in terminal
void MySqlAPI::fixJobNonTerminallAllFilesTerminal(soci::session &sql)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity check non terminal jobs with all transfers terminal" << commit;

    sql.begin();

    soci::rowset<soci::row> notFinishedJobIds = (
        sql.prepare <<
            "SELECT SQL_BUFFER_RESULT job_id, job_type, job_state FROM t_job WHERE job_finished IS NULL"
    );

    for (auto i = notFinishedJobIds.begin(); i != notFinishedJobIds.end(); ++i) {
        const std::string jobId = i->get<std::string>("job_id");
        const std::string jobState = i->get<std::string>("job_state");
        const Job::JobType jobType = i->get<Job::JobType>("job_type");

        // Get file state count
        long long filesInJob = 0;
        std::map<std::string, long long> stateCount;

        soci::rowset<soci::row> fileStates = (sql.prepare <<
            "SELECT file_state, COUNT(file_state) AS cnt "
            "FROM t_file "
            "WHERE job_id = :job_id "
            "GROUP BY file_state "
            "ORDER BY NULL",
            soci::use(jobId)
        );

        for (auto i = fileStates.begin(); i != fileStates.end(); ++i) {
            const std::string fileState = i->get<std::string>("file_state");
            long long count = i->get<long long>("cnt");
            stateCount[fileState] = count;
            filesInJob += count;
        }

        // Fix empty jobs
        if (filesInJob == 0) {
            fixEmptyJob(sql, jobId);
            continue;
        }

        // For non-multiple replica, a job is terminal if *all* files are terminal
        if (jobType != Job::kTypeMultipleReplica) {
            long long terminalCount = stateCount["FINISHED"] + stateCount["FAILED"] + stateCount["CANCEL"];
            if (filesInJob == terminalCount) {
                fixNonTerminalJob(sql, jobId, filesInJob,
                    stateCount["CANCEL"], stateCount["FINISHED"], stateCount["FAILED"]);
            }
        }
        // For multiple replica jobs, a job is terminal if there is one FINISHED, or if all are FAILED/CANCEL
        else {
            if (stateCount["FINISHED"] >= 1) {
                sql << "UPDATE t_file SET "
                    "    file_state = 'NOT_USED', finish_time = NULL, "
                    "    reason = '' "
                    "    WHERE file_state in ('ACTIVE','SUBMITTED') AND job_id = :jobId",
                    soci::use(jobId);
                sql << "UPDATE t_job SET "
                    "    job_state = 'FINISHED', job_finished = UTC_TIMESTAMP()"
                    "    WHERE job_id = :jobId",
                    soci::use(jobId);
                logInconsistency(jobId, "Multireplica job with a finished replica not marked as terminal");
            }
            else if (stateCount["FAILED"] + stateCount["CANCEL"] >= filesInJob) {
                sql << "UPDATE t_job SET "
                    "    job_state = 'FAILED', job_finished = UTC_TIMESTAMP(), reason='Inconsistent state found'"
                    "    WHERE job_id = :jobId",
                    soci::use(jobId);
                logInconsistency(jobId, "Multireplica job with no available replicas not marked as terminal");
            }
            else if ((jobState == "ACTIVE" || jobState == "READY") && (stateCount["ACTIVE"] + stateCount["SUBMITTED"]) == 0) {
                sql << "UPDATE t_job SET "
                    "     job_state = 'FAILED', job_finished = UTC_TIMESTAMP(), reason='Inconsistent state found' "
                    "     WHERE job_id = :jobId",
                    soci::use(jobId);
                logInconsistency(jobId, "Multireplica job marked as active, but has no queued transfers");
            }
        }
    }
    sql.commit();
}

/// Search for jobs in terminal state with files still in non terminal
void MySqlAPI::fixJobTerminalFileNonTerminal(soci::session &sql)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity check job terminal with transfers not terminal" << commit;

    soci::rowset<soci::row> rs = (
        sql.prepare <<
            "SELECT j.job_id "
            "FROM t_job j INNER JOIN t_file f ON (j.job_id = f.job_id) "
            "WHERE j.job_finished IS NOT NULL "
            "AND f.file_state IN ('SUBMITTED','ACTIVE','STAGING','STARTED') "
    );

    sql.begin();
    for (auto i = rs.begin(); i != rs.end(); ++i) {
        const std::string jobId = i->get<std::string>("job_id");

        sql << "UPDATE t_file SET "
            "    file_state = 'FAILED', finish_time = UTC_TIMESTAMP(), "
            "    reason = 'Force failure due to file state inconsistency' "
            "    WHERE file_state in ('ACTIVE','SUBMITTED','STAGING','STARTED') and job_id = :jobId ",
            soci::use(jobId);

        logInconsistency(jobId, "The job is in terminal state, but there are transfers still in non terminal state");
    }
    sql.commit();
}

/// Search for DELETE tasks that are still running, but belong to a job marked as terminal
void MySqlAPI::fixDeleteInconsistencies(soci::session &sql)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity check delete jobs" << commit;

    soci::rowset<std::string> rs = (
        sql.prepare <<
            "SELECT j.job_id FROM t_job j INNER JOIN t_dm f ON (j.job_id = f.job_id) "
            "WHERE j.job_finished >= (UTC_TIMESTAMP() - INTERVAL '24' HOUR ) "
            "   AND f.file_state IN ('STARTED','DELETE')"
    );

    sql.begin();
    for (auto i = rs.begin(); i != rs.end(); ++i) {
        std::string jobId = (*i);
        sql << "UPDATE t_dm SET "
            "    file_state = 'FAILED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
            "    reason = 'Force failure due to file state inconsistency' "
            "    WHERE file_state in ('DELETE','STARTED') and job_id = :jobId",
            soci::use(jobId);

        logInconsistency(jobId,
            "The job is in terminal state, but there are still deletion tasks in non terminal state");
    }
    sql.commit();
}


/// Search for hosts that haven't updated their status for more than two hours.
/// For those matches, mark assigned transfers as CANCELED
void MySqlAPI::recoverFromDeadHosts(soci::session &sql)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity check from dead hosts" << commit;
    Producer producer(ServerConfig::instance().get<std::string>("MessagingDirectory"));

    soci::rowset<std::string> deadHosts = (
        sql.prepare <<
            " SELECT hostname "
            " FROM t_hosts "
            " WHERE beat < DATE_SUB(UTC_TIMESTAMP(), INTERVAL 120 MINUTE) AND service_name = 'fts_server' "
    );

    for (auto i = deadHosts.begin(); i != deadHosts.end(); ++i) {
        const std::string deadHost = (*i);

        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Found host offline for too long: " << deadHost << commit;

        //now check and collect if there are any active/ready in these hosts
        soci::rowset<soci::row> transfersActiveInHost = (
            sql.prepare <<
                "SELECT file_id, job_id FROM t_file "
                "WHERE file_state  = 'ACTIVE' "
                "   AND transfer_host = :transferHost ",
                soci::use(deadHost)
        );
        for (auto active = transfersActiveInHost.begin(); active != transfersActiveInHost.end(); ++active) {
            int fileId = active->get<int>("file_id");
            const std::string jobId = active->get<std::string>("job_id");
            const std::string errorMessage = "Transfer has been forced-canceled because host " + deadHost +
                                             " is offline and the transfer is still assigned to it";

            updateFileTransferStatusInternal(sql, 0.0, jobId, fileId, "CANCELED", errorMessage, 0, 0, 0, false);
            updateJobTransferStatusInternal(sql, jobId, "CANCELED", 0);

            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Canceling assigned transfer " << jobId << " / " << fileId
               << commit;

            //send state monitoring message for the state transition
            const std::vector<TransferState> files = getStateOfTransferInternal(sql, jobId, fileId);
            for (auto it = files.begin(); it != files.end(); ++it) {
                TransferState tmp = (*it);
                MsgIfce::getInstance()->SendTransferStatusChange(producer, tmp);
            }
        }
    }
}

/// Search for staging operations in STARTED, for which their bring online timeout
/// has expired.
void MySqlAPI::recoverStalledStaging(soci::session &sql)
{
    const std::string errorMessage = "Transfer has been forced-canceled because is has been in staging state beyond its bringonline timeout ";

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity check stalled staging" << commit;

    soci::rowset<soci::row> rsStagingStarted = (
        sql.prepare <<
            "SELECT f.file_id, f.staging_start, j.bring_online, j.job_id "
            "FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
            "WHERE file_state = 'STARTED'"
    );

    sql.begin();
    for (auto iStaging = rsStagingStarted.begin(); iStaging != rsStagingStarted.end(); ++iStaging) {
        int fileId = iStaging->get<unsigned long long>("file_id");
        const std::string jobId = iStaging->get<std::string>("job_id");
        int bringOnline = iStaging->get<int>("bring_online", 0);

        time_t startTimeT = 0;
        if (iStaging->get_indicator("staging_start") == soci::i_ok) {
            struct tm startTime = iStaging->get<struct tm>("staging_start");
            startTimeT = timegm(&startTime);
        }

        time_t now = getUTC(0);
        double diff = difftime(now, startTimeT);
        int diffInt = boost::lexical_cast<int>(diff);

        if (diffInt > (bringOnline + 800)) {
            updateFileTransferStatusInternal(sql, 0.0, jobId, fileId, "FAILED", errorMessage, 0, 0, 0, false);
            updateJobTransferStatusInternal(sql, jobId, "FAILED", 0);

            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Canceling staging operation " << jobId << " / " << fileId << commit;

            sql << " UPDATE t_file set staging_finished=UTC_TIMESTAMP() where file_id=:file_id", soci::use(fileId);
        }
    }
    sql.commit();
}


void MySqlAPI::checkSanityState()
{
    if (hashSegment.start != 0) {
        return;
    }
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity states check thread started " << commit;

    soci::session sql(*connectionPool);

    try {
        fixJobNonTerminallAllFilesTerminal(sql);
        fixJobTerminalFileNonTerminal(sql);
        fixDeleteInconsistencies(sql);
        recoverFromDeadHosts(sql);
        recoverStalledStaging(sql);
    }
    catch (std::exception &e) {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity states check thread ended " << commit;
}
