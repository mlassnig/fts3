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
 * PythonCliWrapper.cpp
 *
 *  Created on: Sep 11, 2012
 *      Author: Michał Simon
 */

#include "PythonApi.h"
#include "TransferTypes.h"

#include <boost/optional/optional.hpp>

#include <string>
#include <vector>
#include <map>


namespace fts3 { namespace cli {

using namespace boost;
using namespace std;

PythonApi::PythonApi(py::str endpoint) : ctx(py::extract<string>(endpoint)) {
	ctx.init();
}

PythonApi::~PythonApi() {
}

py::str PythonApi::submit(Job job) {
	return ctx.transferSubmit(job.getJobElementsCpp(), job.getJobParametersCpp(), job.useChecksumCpp()).c_str();
}

void PythonApi::cancel(py::list ids) {

	vector<string> c_ids;

	int size = len(ids);
	for (int i = 0; i < size; i++) {
		c_ids.push_back(py::extract<string>(ids[i]));
	}

	ctx.cancel(c_ids);
}

py::str PythonApi::getStatus(py::str id) {
	JobStatus s = ctx.getTransferJobStatus(py::extract<string>(id));
	return s.jobStatus.c_str();
}

}
}

