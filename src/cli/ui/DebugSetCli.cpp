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

#include "DebugSetCli.h"

#include "exception/cli_exception.h"

using namespace fts3::cli;

namespace po = boost::program_options;

DebugSetCli::DebugSetCli()
{
    // add commandline options specific for fts3-config-set
    specific.add_options()
    (
        "source", po::value<std::string>(),
        "Source SE."
        "\n(Example: --source $SE_NAME)"
    )
    (
        "destination", po::value<std::string>(),
        "Destination SE."
        "\n(Example: --destination $SE_NAME)"
    )
    ;
    // add hidden options
    hidden.add_options()
    ("debug_level", po::value<unsigned>(&level))
    ;
    // add positional (those used without an option switch) command line options
    p.add("debug_level", 1);

}

DebugSetCli::~DebugSetCli()
{

}

void DebugSetCli::validate()
{
    RestCli::validate();

    if (vm.count("source"))
        {
            source = vm["source"].as<std::string>();
        }

    if (vm.count("destination"))
        {
            destination = vm["destination"].as<std::string>();
        }

    if (source.empty() && destination.empty()) throw cli_exception("At least source or destination has to be specified!");

    if (!vm.count("debug_level")) throw bad_option("debug_level", "value missing");

    if (level > 3) throw bad_option("debug_level", "takes following values: 0, 1, 2 or 3");
}

std::string DebugSetCli::getUsageString(std::string tool) const
{
    return "Usage: " + tool + " [options] LEVEL";
}
