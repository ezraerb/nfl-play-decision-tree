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
#include"singlePlay.h"
#include"playIndexSet.h"
#include"playStats.h"
#include"dataStore.h"

#include<iostream>
using std::cerr;
using std::endl;

// Creates an empty data store
DataStore::DataStore()
    : _data(), _indexes()
{
    // All in the initialization list
}

/* Build indexes and derive collective play data. Indicates insertion is done. Data inserted
    after calling this method will be ignored */
void DataStore::buildIndexes()
{
    // If the method is called with the data store empty, do nothing. In practice this indicates an error
    if (_data.empty())
        return;

    // Iterate through the data and build the indexes
    CategoryIndex downIndex(SinglePlay::getCategoryCount(SinglePlay::down_number));
    CategoryIndex distanceNeededIndex(SinglePlay::getCategoryCount(SinglePlay::distance_needed));
    CategoryIndex fieldLocationIndex(SinglePlay::getCategoryCount(SinglePlay::field_location));
    CategoryIndex timeRemainingIndex(SinglePlay::getCategoryCount(SinglePlay::time_remaining));
    CategoryIndex scoreDifferentialIndex(SinglePlay::getCategoryCount(SinglePlay::score_differential));

    PlayIterator indexValue;
    for (indexValue = _data.begin(); indexValue != _data.end(); indexValue++) {
        downIndex[(unsigned short)indexValue->getDown()].push_back(indexValue);
        distanceNeededIndex[(unsigned short)indexValue->getDistanceNeeded()].push_back(indexValue);
        fieldLocationIndex[(unsigned short)indexValue->getFieldLocation()].push_back(indexValue);
        timeRemainingIndex[(unsigned short)indexValue->getTimeRemaining()].push_back(indexValue);
        scoreDifferentialIndex[(unsigned short)indexValue->getScoreDifferential()].push_back(indexValue);
    }
    // Copy into the object
    _indexes.setIndexes(downIndex, distanceNeededIndex, fieldLocationIndex, timeRemainingIndex,
                        scoreDifferentialIndex);
    // Find overall plays statistics
    PlaySummaryFactory::buildSummaryData(_indexes, _playSummaryStats);
}
