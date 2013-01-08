/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 * GSoapContextAdapter.h
 *
 * Job.h
 *
 *  Created on: Dec 20, 2012
 *      Author: Michał Simon
 */

#ifndef JOB_H_
#define JOB_H_

#include "TransferTypes.h"

#include "python/FileTransfer.h"

#include <boost/python.hpp>
#include <boost/tuple/tuple.hpp>

#include <string>
#include <map>
#include <vector>

namespace fts3 {
namespace cli {

namespace py = boost::python;

class Job {

public:
	Job();
	virtual ~Job();

	std::vector<JobElement> getJobElementsCpp();
	std::map<std::string, std::string> getJobParametersCpp();
	bool useChecksumCpp();

	void add(FileTransfer element);
	void addAll(py::list elements);

	void clear();

	py::list files();

	void setDelegationId(py::str id);
	py::object getDelegationId();

	void setGridParam(py::str param);
	py::object getGridParam();

	void setExpirationTime(long expiration);
	py::object getExpirationTime();

	void setOverwrite(bool overwrite);
	py::object overwrite();

	void setDestinationToken(py::str token);
	py::object getDestinationToken();

	void setCompareChecksum(bool cmp);
	py::object compareChecksum();

	void setCopyPinLifetime(int pin);
	py::object getCopyPinLifetime();

	void setLanConnection(bool conn);
	py::object lanConnection();

	void setFailNearline(bool fail);
	py::object failNearline();

	void setSessionReuse(bool reuse);
	py::object sessionReuse();

private:

	///
	std::vector<JobElement> elements;
	///
	std::map<std::string, std::string> parameters;
	///
	bool checksum;
	///
	long expiration;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* JOB_H_ */
