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

#include "TransferFileHandler.h"

namespace fts3
{
namespace server
{

std::map< std::string, std::set<std::string> >& TransferFileHandler::getMapFromCache(std::map< std::string, std::list<TransferFile> >& files, GET_MAP_OPTS opt)
{
    if (init_cache.empty())
        {
            init_cache.resize(4); // there are four maps

            std::map<std::string, std::list<TransferFile> >::iterator it_v;

            std::map< std::string, std::set<FileIndex> > unique;

            // iterate over all VOs
            for (it_v = files.begin(); it_v != files.end(); ++it_v)
                {
                    // the vo name
                    std::string vo = it_v->first;
                    // unique vo names
                    vos.insert(vo);
                    // ref to the list of files (for the given VO)
                    std::list<TransferFile>& tfs = it_v->second;
                    std::list<TransferFile>::iterator it_tf;
                    // iterate over all files in a given VO
                    for (it_tf = tfs.begin(); it_tf != tfs.end(); ++it_tf)
                        {

                            TransferFile tmp = *it_tf;

                            init_cache[SOURCE_TO_DESTINATIONS][tmp.sourceSe].insert(tmp.destSe);
                            init_cache[SOURCE_TO_VOS][tmp.sourceSe].insert(tmp.voName);
                            init_cache[DESTINATION_TO_SOURCES][tmp.destSe].insert(tmp.sourceSe);
                            init_cache[DESTINATION_TO_VOS][tmp.destSe].insert(tmp.voName);

                            // create index (job ID + file index)
                            FileIndex index((*it_tf).jobId, tmp.fileIndex);
                            // file index to files mapping
                            fileIndexToFiles[index].push_back(tmp);
                            // check if the mapping for the VO already exists
                            if (!unique[vo].count(index))
                                {
                                    // if not creat it
                                    unique[vo].insert(index);
                                    voToFileIndexes[it_v->first][make_pair(tmp.sourceSe, tmp.destSe)].push_back(index);
                                }
                        }

                    nextPairForVo[it_v->first] = voToFileIndexes[it_v->first].begin();
                }
        }

    return init_cache[opt];
}


TransferFileHandler::TransferFileHandler(std::map< std::string, std::list<TransferFile> >& files) :
    sourceToDestinations(getMapFromCache(files, SOURCE_TO_DESTINATIONS)),
    sourceToVos(getMapFromCache(files, SOURCE_TO_VOS)),
    destinationToSources(getMapFromCache(files, DESTINATION_TO_SOURCES)),
    destinationToVos(getMapFromCache(files, DESTINATION_TO_VOS))
{
    init_cache.clear();
}

TransferFileHandler::~TransferFileHandler()
{
    std::map< FileIndex, std::list<TransferFile> >::iterator it;
    for (it = fileIndexToFiles.begin(); it != fileIndexToFiles.end(); ++it)
        {
            freeList(it->second);
        }
}

boost::optional<TransferFile> TransferFileHandler::get(std::string vo)
{
    // get the index of the next File in turn for the VO
    boost::optional<FileIndex> index = getIndex(vo);
    // if the index exists return the file
    if (index) return getFile(*index);

    return boost::optional<TransferFile>();
}

boost::optional< std::pair<std::string, std::string> > TransferFileHandler::getNextPair(std::string vo)
{
    // if there are no pairs for the given VO ...
    if (voToFileIndexes[vo].empty()) return boost::optional< std::pair<std::string, std::string> >();

    // if it is the end wrap around
    if (nextPairForVo[vo] == voToFileIndexes[vo].end()) nextPairForVo[vo] = voToFileIndexes[vo].begin();

    // get the iterator
    std::map< std::pair<std::string, std::string>, std::list<FileIndex> >::iterator ret = nextPairForVo[vo];

    // set the next pair
    nextPairForVo[vo]++;

    return ret->first;
}

boost::optional<FileIndex> TransferFileHandler::getIndex(std::string vo)
{
    // find the item
    std::map<std::string, std::map< std::pair<std::string, std::string>, std::list<FileIndex> > >::iterator it =
        voToFileIndexes.find(vo);

    // if the VO has no mapping or no files are assigned to the VO ...
    if (it == voToFileIndexes.end() || it->second.empty()) return boost::optional<FileIndex>();

    boost::optional< std::pair<std::string, std::string> > src_dst = getNextPair(vo);

    if (!src_dst.is_initialized()) return boost::optional<FileIndex>();

    // get the index value
    FileIndex index = it->second[*src_dst].front();
    it->second[*src_dst].pop_front();

    // if there are no more values assigned to either VO or pair remove it from respective mapping
    if (it->second[*src_dst].empty())
        {
            it->second.erase(*src_dst);

            if (it->second.empty())
                {
                    voToFileIndexes.erase(it);
                }
        }

    return index;
}

boost::optional<TransferFile> TransferFileHandler::getFile(FileIndex index)
{
    boost::optional<TransferFile> ret;

    // if there's no mapping for this index ..
    if (fileIndexToFiles.find(index) == fileIndexToFiles.end()) return ret;

    if (!fileIndexToFiles[index].empty())
        {
            // get the first in the list
            ret = fileIndexToFiles[index].front();
            // remove it from the list
            fileIndexToFiles[index].pop_front();
        }

    return ret;
}

void TransferFileHandler::freeList(std::list<TransferFile>& l)
{
    l.clear();
}

std::set<std::string>::iterator TransferFileHandler::begin()
{
    return vos.begin();
}

std::set<std::string>::iterator TransferFileHandler::end()
{
    return vos.end();
}

bool TransferFileHandler::empty()
{
    return voToFileIndexes.empty();
}

const std::set<std::string> TransferFileHandler::getSources(std::string se) const
{
    std::map< std::string, std::set<std::string> >::const_iterator it = destinationToSources.find(se);
    if (it != destinationToSources.end())
        {
            return it->second;
        }

    return std::set<std::string>();
}

const std::set<std::string> TransferFileHandler::getDestinations(std::string se) const
{
    std::map< std::string, std::set<std::string> >::const_iterator it = sourceToDestinations.find(se);
    if (it != sourceToDestinations.end())
        {
            return it->second;
        }

    return std::set<std::string>();
}


const std::set<std::string> TransferFileHandler::getSourcesVos(std::string se) const
{
    std::map< std::string, std::set<std::string> >::const_iterator it = destinationToVos.find(se);
    if (it != destinationToVos.end())
        {
            return it->second;
        }

    return std::set<std::string>();
}

const std::set<std::string> TransferFileHandler::getDestinationsVos(std::string se) const
{
    std::map< std::string, std::set<std::string> >::const_iterator it = sourceToVos.find(se);
    if (it != sourceToVos.end())
        {
            return it->second;
        }

    return std::set<std::string>();
}

int TransferFileHandler::size()
{
    int sum = 0;

    std::map< std::string, std::map< std::pair<std::string, std::string>, std::list<FileIndex> > >::iterator iout;
    std::map< std::pair<std::string, std::string>, std::list<FileIndex> >::iterator iin;

    for (iout = voToFileIndexes.begin(); iout != voToFileIndexes.end(); iout++)
        for (iin = iout->second.begin(); iin != iout->second.end(); iin++)
            sum += (unsigned int) iin->second.size();

    return sum;
}

} /* namespace cli */
} /* namespace fts3 */
