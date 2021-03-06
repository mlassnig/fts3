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

#include "SetCfgCli.h"

#include "exception/bad_option.h"

#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


using namespace fts3::cli;

SetCfgCli::SetCfgCli(bool spec)
{
    type = CfgParser::NOT_A_CFG;

    if (spec)
        {
            // add commandline options specific for fts3-config-set
            specific.add_options()
            (
                "bring-online", po::value< std::vector<std::string> >()->multitoken(),
                "If this switch is used the user should provide SE_NAME VALUE and boost::optionally VO_NAME in order to set the maximum number of files that are staged concurrently for a given SE."
                "\n(Example: --bring-online $SE_NAME $VALUE [$VO_NAME])"
            )
            (
                "delete", po::value< std::vector<std::string> >()->multitoken(),
                "If this switch is used the user should provide SE_NAME VALUE and boost::optionally VO_NAME in order to set the maximum number of files that are deleted concurrently for a given SE."
                "\n(Example: --delete $SE_NAME $VALUE [$VO_NAME])"
            )
            (
                "drain", po::value<std::string>(),
                "If set to 'on' drains the given endpoint."
                "\n(Example: --drain on|off)"
            )
            (
                "retry", po::value< std::vector<std::string> >()->multitoken(),
                "Sets the number of retries of each individual file transfer for the given VO (the value should be greater or equal to -1)."
                "\n(Example: --retry $VO $NB_RETRY)"
            )
            (
                "optimizer-mode", po::value<int>(),
                "Sets the optimizer mode (allowed values: 1, 2 or 3)"
                "\n(Example: --optimizer-mode 1|2|3)"
                "\nPlan 1: use file size to calculate the number of streams "
                "\nPlan 2: take samples for 15min from 1-16 streams and then every 12h "
                "\nPlan 3: set TCP buffer size to 8MB "
            )
            (
                "queue-timeout", po::value<int>(),
                "Sets the maximum time (in hours) transfer job is allowed to be in the queue (the value should be greater or equal to 0)."
                "\n(Example: --queue-timeout $TIMEOUT)"
            )
            (   "source", po::value<std::string>(),
                "The source SE"
                "\n(Example: --source $SE_NAME)"
            )
            (
                "destination", po::value<std::string>(),
                "The destination SE"
                "\n(Example: --destination $SE_NAME)"
            )
            (
                "max-bandwidth", po::value<int>(),
                "The maximum bandwidth that can be used (in MB/s) for the given source or destination (see --source & --destination)"
                "\n(Example: --max-bandwidth $LIMIT)"
            )
            (   "protocol", po::value< std::vector<std::string> >()->multitoken(),
                "Set protocol (UDT | IPv6) for given SE"
                "\n(Example: --protocol udt $SE_NAME on|off)"
                "\n(Example: --protocol ipv6 $SE_NAME on|off)"
            )
            (
                "max-se-source-active", po::value< std::vector<std::string> >()->multitoken(),
                "Maximum number of active transfers for given source SE (-1 means no limit)"
                "\n(Example: --max-se-source-active $NB_ACT $SE_NAME)"
            )
            (
                "max-se-dest-active", po::value< std::vector<std::string> >()->multitoken(),
                "Maximum number of active transfers for given destination SE (-1 means no limit)"
                "\n(Example: --max-se-dest-active $NB_ACT $SE_NAME)"
            )
            (
                "global-timeout", po::value<int>(),
                "Global transfer timeout"
                "\n(Example: --global-timeout $TIMEOUT)"
            )
            (
                "max-per-link", po::value<int>(),
                "Global limit per link"
                "\n(Example: --max-per-link $MAXACTIVE)"
            )
            (
                "max-per-se", po::value<int>(),
                "Global active limit per storage element"
                "\n(Example: --max-per-se $MAXACTIVE)"
            )
            (
                "sec-per-mb", po::value<int>(),
                "number of seconds per MB"
                "\n(Example: --sec-per-mb $SEC_PER_MB)"
            )
            (
                "active-fixed", po::value<int>(),
                "Fixed number of active transfer for a given pair (-1 means reset to optimizer)"
                "\n(Example: --source $SE --destination $SE --active-fixed $NB_ACT"
            )
            (
                "show-user-dn", po::value<std::string>(),
                "If set to 'on' user DNs will included in monitoring messages"
                "\n(Example: --show-user-dn on|off)"
            )
            (
                "s3", po::value< std::vector<std::string> >()->multitoken(),
                "Set the S3 credentials, requires: access-key, secret-key, VO name and storage name"
                "\n(Example: --s3 $ACCESS_KEY $SECRET_KEY $VO_NAME $STORAGE_NAME)"
            )
            (
                "dropbox", po::value< std::vector<std::string> >()->multitoken(),
                "Set the dropbox credentials, requires: app-key, app-secret and service API URL"
                "\n(Example: --s3 $APP_KEY $APP_SECRET $API_URL)"
            )
            (
                "authorize",  po::value< std::vector<std::string> >()->multitoken(),
                "Authorize a user to perform an action without needing VO extensions"
                "\n(Example: --authorize config \"/DC=ch/DC=cern/...\""
            )
            (
                "revoke",  po::value< std::vector<std::string> >()->multitoken(),
                "Revoke the permissions given to an user to perform an action without needing VO extensions"
                "\n(Example: --revoke config \"/DC=ch/DC=cern/...\""
            )
            ;
        }

    // add hidden options (not printed in help)
    hidden.add_options()
    ("cfg", po::value< std::vector<std::string> >(), "Specify SE configuration.")
    ;

    // all positional parameters go to jobid
    p.add("cfg", -1);
}

SetCfgCli::~SetCfgCli()
{
}

void SetCfgCli::parse(int ac, char* av[])
{

    // do the basic initialisation
    CliBase::parse(ac, av);

    if (vm.count("cfg"))
        {
            cfgs = vm["cfg"].as< std::vector<std::string> >();
        }
    else if(vm.count("max-bandwidth"))
        {
            parseMaxBandwidth();
        }

    if (vm.count("bring-online"))
        parseMaxOpt("bring-online");

    if (vm.count("delete"))
        parseMaxOpt("delete");

    if (vm.count("active-fixed"))
        parseActiveFixed();

    // check JSON configurations
    std::vector<std::string>::iterator it;
    for (it = cfgs.begin(); it < cfgs.end(); it++)
        {
            boost::trim(*it);
            // check if the configuration is started with an opening brace and ended with a closing brace
            if (*it->begin() != '{' || *(it->end() - 1) != '}')
                {
                    // most likely the user didn't used single quotation marks and bash did some pre-parsing
                    throw bad_option("cfg", "Configuration error: most likely you didn't use single quotation marks (') around a configuration!");
                }

            // parse the configuration, check if its valid JSON format, and valid configuration
            CfgParser c(*it);

            type = c.getCfgType();
            if (type == CfgParser::NOT_A_CFG) throw bad_option("cfg", "The specified configuration doesn't follow any of the valid formats!");
        }
}

void SetCfgCli::validate()
{
    RestCli::validate();
    std::vector<std::string> singleOpts;

    if (vm.count("s3"))
        {
            singleOpts.push_back("s3");
        }

    if (vm.count("dropbox"))
        {
            singleOpts.push_back("dropbox");
        }

    if (vm.count("authorize") || vm.count("revoke"))
        {
            singleOpts.push_back("authorize/revoke");
        }

    bool hasNoOtherOpts = 
        getConfigurations().empty()
            && !vm.count("drain")
            && !vm.count("retry")
            && !vm.count("queue-timeout")
            && !vm.count("bring-online")
            && !vm.count("delete")
            && !vm.count("optimizer-mode")
            && !vm.count("max-bandwidth")
            && !vm.count("protocol")
            && !vm.count("max-se-source-active")
            && !vm.count("max-se-dest-active")
            && !vm.count("global-timeout")
            && !vm.count("max-per-link")
            && !vm.count("max-per-se")
            && !vm.count("sec-per-mb")
            && !vm.count("active-fixed")
            && !vm.count("show-user-dn");

    if (singleOpts.size()>0)
        {
            if (singleOpts.size()>1 || !hasNoOtherOpts)
                {
                    throw bad_option(singleOpts[0], "should be used only as a single option");
                }
            return;
        }

    if (hasNoOtherOpts)
        {
            throw cli_exception("No parameters have been specified.");
        }

    boost::optional<std::pair<std::string, int>> src = getMaxSeActive("max-se-source-active");
    boost::optional<std::pair<std::string, int>> dst = getMaxSeActive("max-se-dest-active");

    if (src.is_initialized() && dst.is_initialized())
        {
            if (src.get().second != dst.get().second)
                throw bad_option(
                    "max-se-source-active, max-se-dest-active",
                    "the number of active for source and destination has to be equal"
                );
        }

    if ((vm.count("active-fixed") || vm.count("sec-per-mb")) && (!vm.count("source") || !vm.count("destination")))
        throw bad_option("source, destination", "missing source and destination pair");
}

std::string SetCfgCli::getUsageString(std::string tool) const
{
    return "Usage: " + tool + " [options] CONFIG [CONFIG...]";
}

std::vector<std::string> SetCfgCli::getConfigurations()
{
    return cfgs;
}

boost::optional<bool> SetCfgCli::drain()
{
    if (vm.count("drain"))
        {
            std::string const & value = vm["drain"].as<std::string>();

            if (value != "on" && value != "off")
                {
                    throw bad_option("drain", "drain may only take on/off values!");
                }

            return value == "on";
        }

    return boost::optional<bool>();
}

boost::optional<bool> SetCfgCli::showUserDn()
{
    if (vm.count("show-user-dn"))
        {
            std::string const & value = vm["show-user-dn"].as<std::string>();

            if (value != "on" && value != "off")
                {
                    throw bad_option("show-user-dn", "may only take on/off values!");
                }

            return value == "on";
        }

    return boost::none;
}

boost::optional< std::pair<std::string, int> > SetCfgCli::retry()
{
    if (vm.count("retry"))
        {
            // get a reference to the options set by the user
            std::vector<std::string> const & v = vm["retry"].as< std::vector<std::string> >();
            // make sure the number of parameters is correct
            if (v.size() != 2) throw bad_option("retry", "following parameters were expected: VO nb_of_retries");

            try
                {
                    int retries = boost::lexical_cast<int>(v[1]);
                    if (retries < -1)
                        throw bad_option("retry", "incorrect value: the number of retries has to greater or equal to -1.");
                    return make_pair(v[0], retries);
                }
            catch(boost::bad_lexical_cast& ex)
                {
                    throw bad_option("retry", "incorrect value: " + v[1] + " (the number of retries has be an integer).");
                }
        }

    return boost::optional< std::pair<std::string, int> >();
}

boost::optional<int> SetCfgCli::optimizer_mode()
{
    if (vm.count("optimizer-mode"))
        {
            int mode = vm["optimizer-mode"].as<int>();

            if (mode < 1 || mode > 3)
                {
                    throw bad_option("optimizer-mode", "only following values are accepted: 1, 2 or 3");
                }

            return mode;
        }

    return boost::optional<int>();
}

boost::optional<unsigned> SetCfgCli::queueTimeout()
{
    if (vm.count("queue-timeout"))
        {
            int timeout = vm["queue-timeout"].as<int>();
            if (timeout < 0)
                throw bad_option("queue-timeout", "the queue-timeout value has to be greater or equal to 0.");
            return timeout;
        }

    return boost::optional<unsigned>();
}

boost::optional< std::tuple<std::string, std::string, std::string> > SetCfgCli::getProtocol()
{
    // check if the option was used
    if (!vm.count("protocol")) return boost::optional< std::tuple<std::string, std::string, std::string> >();
    // make sure it was used corretly
    const std::vector<std::string>& v = vm["protocol"].as< std::vector<std::string> >();
    if (v.size() != 3) throw bad_option("protocol", "'--protocol' takes following parameters: udt/ipv6 SE on/off");
    if (v[2] != "on" && v[2] != "off") throw bad_option("protocol", "'--protocol' can only be switched 'on' or 'off'");

    return std::make_tuple(v[0], v[1], v[2]);
}

void SetCfgCli::parseMaxOpt(std::string const & operation)
{
    std::vector<std::string> const & vec = vm[operation].as< std::vector<std::string> >();
    // check the number of arguments
    if (vec.size() != 2 && vec.size() != 3) throw bad_option(operation, "wrong number of arguments!");
    // check arguments one after another
    std::vector<std::string>::const_iterator it = vec.begin();
    // get SE name
    std::string const & se = *it;
    // than get the value
    ++it;
    int value;
    try
        {
            value = boost::lexical_cast<int>(*it);
        }
    catch(boost::bad_lexical_cast const & ex)
        {
            throw bad_option("bring-online", "value: " + *it + " is not a correct integer (int) value!");
        }
    // and than check if VO name was provided
    ++it;
    std::string vo;
    if (it != vec.end()) vo = *it;
    // set the parsed values for the given oepration
    max_opt[operation] = std::make_tuple(se, value, vo);
}


void SetCfgCli::parseMaxBandwidth()
{
    std::string source_se, dest_se;

    if (!vm["source"].empty())
        source_se = vm["source"].as<std::string>();
    if (!vm["destination"].empty())
        dest_se = vm["destination"].as<std::string>();

    int limit = vm["max-bandwidth"].as<int>();

    bandwidth_limitation = boost::make_optional(std::tuple<std::string, std::string, int>(source_se, dest_se, limit));
}

void SetCfgCli::parseActiveFixed()
{
    std::string source_se, dest_se;

    if (!vm["source"].empty())
        source_se = vm["source"].as<std::string>();
    if (!vm["destination"].empty())
        dest_se = vm["destination"].as<std::string>();

    int active = vm["active-fixed"].as<int>();

    active_fixed = boost::make_optional(std::tuple<std::string, std::string, int>(source_se, dest_se, active));
}

boost::optional< std::tuple<std::string, int, std::string> > SetCfgCli::getBringOnline()
{
    if (max_opt.find("bring-online") == max_opt.end()) return boost::none;
    return max_opt["bring-online"];
}

boost::optional< std::tuple<std::string, int, std::string> > SetCfgCli::getDelete()
{
    if (max_opt.find("delete") == max_opt.end()) return boost::none;
    return max_opt["delete"];
}

boost::optional<std::tuple<std::string, std::string, int> > SetCfgCli::getBandwidthLimitation()
{
    return bandwidth_limitation;
}

boost::optional<std::tuple<std::string, std::string, int> > SetCfgCli::getActiveFixed()
{
    return active_fixed;
}

boost::optional< std::pair<std::string, int> > SetCfgCli::getMaxSeActive(std::string option)
{
    if (!vm.count(option))
        {
            return boost::optional< std::pair<std::string, int> >();
        }

    // make sure it was used corretly
    const std::vector<std::string>& v = vm[option].as< std::vector<std::string> >();

    if (v.size() != 2) throw bad_option(option, "'--" + option + "' takes following parameters: number_of_active SE");

    std::string se = v[1];

    try
        {
            int active = boost::lexical_cast<int>(v[0]);

            if (active < -1) throw bad_option("option", "values lower than -1 are not valid");

            return std::make_pair(se, active);
        }
    catch(boost::bad_lexical_cast& ex)
        {
            throw bad_option(option, "the number of active has to be an integer");
        }
}

boost::optional< std::pair<std::string, int> > SetCfgCli::getMaxSrcSeActive()
{
    return getMaxSeActive("max-se-source-active");
}

boost::optional< std::pair<std::string, int> > SetCfgCli::getMaxDstSeActive()
{
    return getMaxSeActive("max-se-dest-active");
}

boost:: optional<int> SetCfgCli::getGlobalTimeout()
{
    if (!vm.count("global-timeout")) return boost::optional<int>();

    int timeout = vm["global-timeout"].as<int>();

    if (timeout < -1) throw bad_option("global-timeout", "values lower than -1 are not valid");

    if (timeout == -1) timeout = 0;

    return timeout;
}

boost::tuple<boost::optional<int>, boost::optional<int>> SetCfgCli::getGlobalLimits()
{
    boost::optional<int> maxPerLink, maxPerSe;

    if (vm.count("max-per-link"))
        maxPerLink = vm["max-per-link"].as<int>();
    if (vm.count("max-per-se"))
        maxPerSe = vm["max-per-se"].as<int>();

    return boost::tuple<boost::optional<int>, boost::optional<int> > (maxPerLink, maxPerSe);
}

boost::optional<int> SetCfgCli::getSecPerMb()
{
    if (!vm.count("sec-per-mb")) return boost::optional<int>();

    int sec = vm["sec-per-mb"].as<int>();

    if (sec < -1) throw bad_option("sec-per-mb", "values lower than -1 are not valid");

    if (sec == -1) sec = 0;

    return sec;
}

boost::optional< std::tuple<std::string, std::string, std::string, std::string> > SetCfgCli::s3()
{
    if (!vm.count("s3")) return boost::none;

    std::vector<std::string> const & v = vm["s3"].as< std::vector<std::string> >();

    if (v.size() != 4) throw bad_option("s3", "4 parameters were expected: access-key, secret-key, VO name and storage name");

    return std::make_tuple(v[0], v[1], v[2], v[3]);
}

boost::optional< std::tuple<std::string, std::string, std::string> > SetCfgCli::dropbox()
{
    if (!vm.count("dropbox")) return boost::none;

    std::vector<std::string> const & v = vm["dropbox"].as< std::vector<std::string> >();

    if (v.size() != 3) throw bad_option("dropbox", "3 parameters were expected: app-key, app-secret and service API URL");

    return std::make_tuple(v[0], v[1], v[2]);
}

boost::optional< std::tuple<std::string, std::string> > SetCfgCli::getAddAuthorization()
{
    if (!vm.count("authorize")) return boost::none;

    std::vector<std::string> const & v = vm["authorize"].as< std::vector<std::string> >();

    if (v.size() != 2) throw bad_option("authorize", "2 parameters were expected: operation and dn");

    return std::make_tuple(v[0], v[1]);
}

boost::optional< std::tuple<std::string, std::string> > SetCfgCli::getRevokeAuthorization()
{
    if (!vm.count("revoke")) return boost::none;

    std::vector<std::string> const & v = vm["revoke"].as< std::vector<std::string> >();

    if (v.size() != 2) throw bad_option("revoke", "2 parameters were expected: operation and dn");

    return std::make_tuple(v[0], v[1]);
}
