/* This file is part of NFLdecisionTree. It creates decision trees to classify
    situations within NFL football games, and displays the plays historically
    called in those situations given a set of opponents.

    Copyright (C) 2013   Ezra Erb

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    I'd appreciate a note if you find this program useful or make
    updates. Please contact me through LinkedIn (my profile also has
    a link to the code depository)
*/
#include<vector>
#include<ostream>
#include<cmath>
#include<algorithm>
#include<utility>

#include"singlePlay.h"
#include"playIndexSet.h"
#include"playStats.h"

using std::make_pair;

OverallPlaySummary::OverallPlaySummary(const DistanceVector& playDistances, short turnoverCount)
{
    // Calculate the stats from the data about the play
    _totalCount = playDistances.size();
    if (_totalCount > 0) {
        // Turnover percentage is in tenth of percent
        _turnoverPercentage = (turnoverCount * 1000) / _totalCount;

        // Sum the distance gained on all plays
        short totalDistance = 0;
        DistanceVector::const_iterator index;
        for (index = playDistances.begin(); index != playDistances.end(); index++)
            totalDistance += *index;

        _averageDistance = totalDistance / _totalCount;

        /* To calculate the variance, need the square of the distance between
            the average and each value. Calculate using ints to avoid overflow */
        int totalVariance = 0;
        for (index = playDistances.begin(); index != playDistances.end(); index++)
            totalVariance += ((int)(*index - _averageDistance) *
                              (int)(*index - _averageDistance));

        totalVariance /= _totalCount;

        _distanceVariance = (short)sqrt(totalVariance);
    }
    else {
        _averageDistance = 0;
        _distanceVariance = 0;
        _turnoverPercentage = 0;
    }
}

// Constructor
DetailedPlaySummary::DetailedPlaySummary(const DistanceVector& playDistances, short turnoverCount,
                                         short conditionPlayCount,
                                         const OverallPlaySummary& overallTypeStatistics)
    : _playDistances(playDistances), _turnoverCount(turnoverCount),
      _groupStats(playDistances, turnoverCount), _overallStats(overallTypeStatistics)
{
    sort(_playDistances.begin(), _playDistances.end());

    // Calculate percentages
    _percentOfConditionPlays = ((short)_playDistances.size() * 1000) / conditionPlayCount;
    _percentOfTypePlays = ((short)_playDistances.size() * 1000) / overallTypeStatistics.getTotalCount();
}

/* Merge one summary into another. Used when combining statistics from
    similiar conditions */
void DetailedPlaySummary::merge(const DetailedPlaySummary& other, short totalMergedPlays)
{
    /* WARNING: If statistics are merged for different types of plays, the results will
        be meaningless */
    // Overall data stays the same

    _playDistances.insert(_playDistances.end(), other._playDistances.begin(),
                          other._playDistances.end());
    sort(_playDistances.begin(), _playDistances.end());
    _turnoverCount += other._turnoverCount;
    // Calculate new statistics based on the combined play data
    _groupStats = OverallPlaySummary(_playDistances, _turnoverCount);

    // Calculate new percentages
    _percentOfTypePlays = ((short)_playDistances.size() * 1000) / _overallStats.getTotalCount();
    updateConditionStats(totalMergedPlays);
}

// Create overall summary data from a snapshot of the data store
void PlaySummaryFactory::buildSummaryData(const PlayIndexSet& indexes, OverallSummaryData& data)
{
    data.clear();

    // Assemble statistics about plays
    vector<DistanceVector> distances(SinglePlay::getPlayTypeCount());
    vector<short> turnoverCounts(SinglePlay::getPlayTypeCount());
    indexesToCounts(indexes, distances, turnoverCounts);

    // Convert to summary by play type
    unsigned short index3;
    for (index3 = 0; index3 < distances.size(); index3++)
        data.push_back(OverallPlaySummary(distances[index3], turnoverCounts[index3]));
}

// Create detailed data from a snapshot of the data store, and the summary of the overall data
void PlaySummaryFactory::buildDetailedData(const PlayIndexSet& indexes, const OverallSummaryData& overallData,
                                           DetailedPlayData& detailedData)
{
    detailedData.clear();
    // Assemble statistics about plays
    vector<DistanceVector> distances(SinglePlay::getPlayTypeCount());
    vector<short> turnoverCounts(SinglePlay::getPlayTypeCount());
    indexesToCounts(indexes, distances, turnoverCounts);

    /* If distances were found, convert the play data to a summary and insert */
    short totalPlayCount = 0;
    unsigned short index3;
    // Find total number of plays in index, which is the number of distances returned
    for (index3 = 0; index3 < distances.size(); index3++)
        totalPlayCount += distances[index3].size();

    for (index3 = 0; index3 < distances.size(); index3++)
        if (!distances[index3].empty())
            detailedData.insert(make_pair((SinglePlay::PlayType)index3,
                                          DetailedPlaySummary(distances[index3], turnoverCounts[index3],
                                                              totalPlayCount, overallData.at(index3))));
}

void PlaySummaryFactory::indexesToCounts(const PlayIndexSet& indexes, vector<DistanceVector>& distances,
                                         vector<short>& turnoverCounts)
{
    // If indexes have no data, routine has nothing to do
    if (indexes.getIndexesAvailable().empty())
        return;

    // Find characteristic to use. Any one will do
    SinglePlay::PlayCharacteristic testIndex = *(indexes.getIndexesAvailable().begin());

    CategoryIndex::const_iterator index1;
    PlayIndex::const_iterator index2;

    // Assemble statistics using index
    for (index1 = indexes.getIndex(testIndex).begin();
         index1 != indexes.getIndex(testIndex).end(); index1++)
        for (index2 = index1->begin(); index2 != index1->end(); index2++) {
            distances.at((unsigned short)((*index2)->getPlayType())).push_back((*index2)->getDistanceGained());
            if ((*index2)->getTurnedOver())
                turnoverCounts.at((unsigned short)((*index2)->getPlayType()))++;
        }
}

// Merges two sets of play summaries together
void PlaySummaryFactory::mergeData(DetailedPlayData& result, const DetailedPlayData& other)
{
    // Find the total number of plays in the combined summaries, needed below
    DetailedPlayData::iterator resultPtr;
    DetailedPlayData::const_iterator otherPtr;
    short playCount = 0;
    for (resultPtr = result.begin(); resultPtr != result.end(); resultPtr++)
        playCount += resultPtr->second.getPlayCount();
    for (otherPtr = other.begin(); otherPtr != other.end(); otherPtr++)
        playCount += otherPtr->second.getPlayCount();

    /* The most straightforward method is to search for types in the one to merge
        in the existing results. If found, merge the summaries, otherwise insert. That
        gets expensive. Since both are sorted in the same order, iterate through both.
        Note that insert will invalidate an existing iterator, but the operation itself
        returns an iterator to the new entry, just what is needed */
    resultPtr = result.begin();
    for (otherPtr = other.begin(); otherPtr != other.end(); otherPtr++) {
        while ((resultPtr != result.end()) &&
               (resultPtr->first < otherPtr->first)) {
            // Update play to account for change in total plays
            resultPtr->second.updateConditionStats(playCount);
            resultPtr++;
        }
        if ((resultPtr != result.end()) &&
            (resultPtr->first == otherPtr->first)) // Data for identical types
            resultPtr->second.merge(otherPtr->second, playCount);
        else if (resultPtr != result.end()) {
            /* Get a fast insert by passing the position BEFORE
               the wanted insert position.
               NOTE: Existing iterator is invalidated, reset from insert results */
            DetailedPlayData::iterator tempIter = resultPtr;
            tempIter--;
            resultPtr = result.insert(tempIter, *otherPtr);
            // Update new play to account for changes in total plays
            resultPtr->second.updateConditionStats(playCount);
        }
        else {
            // Decrementing the iterator is undefined, need to do this the hard way
            resultPtr = result.insert(*otherPtr).first;
            // Update new play to account for changes in total plays
            resultPtr->second.updateConditionStats(playCount);
        }
    } // Loop through entries to merge
}




// Output operator
ostream& operator<<(ostream& stream, const DetailedPlaySummary& data)
{
    /* Summary output requires tradeoffs. Putting out everything will make it hard
        to read. Using multiple lines can fix that problem, except that summaries will
        often be output in formatted trees, and multiple lines will screw up the
        alignment. This code only outputs the most commonly used stats */
    stream << "pct of category:" << data.getPercentOfConditionPlays()
           << " pct of all type plays:" << data.getPercentOfTypePlays()
           << " avg dist:" << data.getAverageDistance()
           << " dist var:" << data.getDistanceVariance()
           << " Turnover pct:" << data.getTurnoverPercentage();
    return stream;
}
