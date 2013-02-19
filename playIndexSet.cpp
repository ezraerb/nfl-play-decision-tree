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
#include"singlePlay.h"
#include"playIndexSet.h"
#include"baseException.h"

#include<iostream>
using std::cerr;

using std::vector;
using std::endl;
using std::ostream;

// Default constructor.
PlayIndexSet::PlayIndexSet()
    : _indexes(),_downIndex(),_distanceNeededIndex(), _fieldLocationIndex(), _timeRemainingIndex(),
        _scoreDifferentialIndex(), _emptyCatIndex()
{
    // All  in the intialization list
}

// Copy constructor
PlayIndexSet::PlayIndexSet(const PlayIndexSet& other)
    : _indexes(other._indexes), _downIndex(other._downIndex), _distanceNeededIndex(other._distanceNeededIndex),
    _fieldLocationIndex(other._fieldLocationIndex), _timeRemainingIndex(other._timeRemainingIndex),
    _scoreDifferentialIndex(other._scoreDifferentialIndex), _emptyCatIndex()
{
    // All in the  intialization list
}

// Assignment operator
/* NOTE: This is here mostly for completeness. Using in in practice indicats some sort of problem */
PlayIndexSet& PlayIndexSet::operator=(const PlayIndexSet& other)
{
    _indexes = other._indexes;
    _downIndex = other._downIndex;
    _distanceNeededIndex = other._distanceNeededIndex;
    _fieldLocationIndex = other._fieldLocationIndex;
    _timeRemainingIndex = other._timeRemainingIndex;
    _scoreDifferentialIndex = other._scoreDifferentialIndex;
    return *this;
}

// Sets indexes in the object. Existing ones are deleted
void PlayIndexSet::setIndexes(const CategoryIndex& downIndex, const CategoryIndex& distanceNeededIndex,
                              const CategoryIndex& fieldLocationIndex, const CategoryIndex& timeRemainingIndex,
                              const CategoryIndex& scoreDifferentialIndex)
{
    //  Sanity check the data: all indexes must be non-empty
    if (downIndex.empty() || distanceNeededIndex.empty() ||
        fieldLocationIndex.empty() || timeRemainingIndex.empty() ||
        scoreDifferentialIndex.empty())
        throw BaseException(__FILE__, __LINE__, "Index create failed, some data indexes empty after build");

    _downIndex = downIndex;
    _distanceNeededIndex = distanceNeededIndex;
    _fieldLocationIndex = fieldLocationIndex;
    _timeRemainingIndex = timeRemainingIndex;
    _scoreDifferentialIndex = scoreDifferentialIndex;

    // Initialize the indexes list to all possible
    _indexes.clear();
    _indexes.insert(SinglePlay::down_number);
    _indexes.insert(SinglePlay::distance_needed);
    _indexes.insert(SinglePlay::field_location);
    _indexes.insert(SinglePlay::time_remaining);
    _indexes.insert(SinglePlay::score_differential);
}

/* Splits an index by an attribute. Indexes will be divided by each possible value. This method exists
    because spliting indexes using the characteristc used to split the underlying plays will be faster
    that either recreating the indexes from the play set, or searcing in sets of plays (which is O(logN))
    to split the plays. The characteristic for the split is deleted from the index set since it no longer
    adds any value. This class will contain the first of the split indxes; the returned values will have the rest */
vector<PlayIndexSet> PlayIndexSet::splitIndexByCharacteristic(SinglePlay::PlayCharacteristic playCharacteristic)
{
    CategoryIndex splitingIndex = getIndex(playCharacteristic);
    // Count the number of categories with indexes. If one or less, nothing to do!
    CategoryIndex::const_iterator temp;
    unsigned short splitCount = 0;
    for (temp = splitingIndex.begin(); temp != splitingIndex.end(); temp++)
        if (!temp->empty())
            splitCount++;
    /* At this point, index for characteristic to split is either about to become
        redundant or already is. Drop it in either case */
    dropIndex(playCharacteristic);

    if (splitCount <= 1) {
        // Nothing else to do!
        vector<PlayIndexSet> result;
        return result;
    }
    else {
        // Split results. Remember that one ends up in the original object
        /* The private constuctor means the starting value for the vector entries must be
            passed in */
        vector<PlayIndexSet> result(splitCount - 1);
        vector<PlayIndexSet>::iterator resultIterator;
        for (resultIterator= result.begin(); resultIterator != result.end(); resultIterator++)
            resultIterator->_indexes = _indexes;

        /* For every index, need to assemble a vector of pointers to the index
            in the results vector, and use those to do the split */
        vector<CategoryIndex*> catPointers;
        vector<PlayIndexSet>::iterator resultCounter;

        for (resultIterator = result.begin(); resultIterator != result.end(); resultIterator++)
            catPointers.push_back(&(resultIterator->_downIndex));
        splitIndex(playCharacteristic, _downIndex, catPointers);
        catPointers.clear(); // Ensure index data does not carry over

        for (resultIterator = result.begin(); resultIterator != result.end(); resultIterator++)
            catPointers.push_back(&(resultIterator->_distanceNeededIndex));
        splitIndex(playCharacteristic, _distanceNeededIndex, catPointers);
        catPointers.clear(); // Ensure index data does not carry over!

        for (resultIterator = result.begin(); resultIterator != result.end(); resultIterator++)
            catPointers.push_back(&(resultIterator->_fieldLocationIndex));
        splitIndex(playCharacteristic, _fieldLocationIndex, catPointers);
        catPointers.clear(); // Ensure index data does not carry over!

        for (resultIterator = result.begin(); resultIterator != result.end(); resultIterator++)
            catPointers.push_back(&(resultIterator->_timeRemainingIndex));
        splitIndex(playCharacteristic, _timeRemainingIndex, catPointers);
        catPointers.clear(); // Ensure index data does not carry over!

        for (resultIterator = result.begin(); resultIterator != result.end(); resultIterator++)
            catPointers.push_back(&(resultIterator->_scoreDifferentialIndex));
        splitIndex(playCharacteristic, _scoreDifferentialIndex, catPointers);
        catPointers.clear(); // Ensure index data does not carry over!
        return result;
    } // Split into at least two categories
}

// Split an index by a category characteristic, into seperate indexes for each category
void PlayIndexSet::splitIndexHelper(SinglePlay::PlayCharacteristic playCharacteristic, const PlayIndex& existIndex,
                                    vector<PlayIndex>& newIndexes)
{
    newIndexes.clear();
    newIndexes.resize(SinglePlay::getCategoryCount(playCharacteristic));
    if (newIndexes.empty()) // Wrong index type
        return;
    PlayIndex::const_iterator index;
    for (index = existIndex.begin(); index != existIndex.end(); index++)
        newIndexes[(*index)->getValue(playCharacteristic)].push_back(*index);
}

/* NOTE: This method takes a vector of pointers. They should actually be references,
    but they can't be used in vectors because they don't have default values */
// Splits a single index by the given category based characteristic
void PlayIndexSet::splitIndex(SinglePlay::PlayCharacteristic playCharacteristic, CategoryIndex& existIndex,
                              vector<CategoryIndex*> newIndexes)
{
    if (existIndex.empty())
        return; // Nothing to do!

    /* Splitting a category index by a category based criteria gets complicated. The
        actual split is easy enough. This produces data indexed first by the current
        index categories, then the split categories. Need to swap those two to get the
        wanted data structures for the return. Split categories with no results also
        get dropped, so this routine counts them as the splits proceed */
    vector<short> splitCounts(SinglePlay::getCategoryCount(playCharacteristic));
    vector<vector<PlayIndex> > results(existIndex.size());

    unsigned short index, index2;
    for (index = 0; index < existIndex.size(); index++) {
        splitIndexHelper(playCharacteristic, existIndex[index], results[index]);
        // Find number of new index entries per split category
        for (index2 = 0; index2 < splitCounts.size(); index2++)
            splitCounts[index2] += results[index][index2].size();
    } // For each category of the current index

    // Find the first category with values, swap it to the existing index
    index = 0;
    index2 = 0;
    unsigned short index3;
    while (index < splitCounts.size() && (!splitCounts[index]))
        index++;
    // If have none, have a major  problem
    if (index == splitCounts.size())
        throw BaseException(__FILE__, __LINE__, "Index split failed, generated pieces with no entries");

    /* Swap those results into the existing index. It needs to be done category
        by category. The second indexes look out of order; this is due to the indexes
        being in the 'wrong' order in the original results */
    for (index3 = 0; index3 < existIndex.size(); index3++)
        existIndex[index3].swap(results[index3][index]);

    /* Now, swap the remaining non-empty results into the other passed indexes.
        Results are guarenteed to exist because the index for the split category
        indicated the number of categories with values. A mismatch is a SERIOUS
        error */


    index++; // Move off index swapped above
    while (index < splitCounts.size()) {
        if (splitCounts[index]) {
            if (index2 >= newIndexes.size())
                // Too many results, serious problem
                throw BaseException(__FILE__, __LINE__, "Index split failed, generated too many pieces");
            for (index3 = 0; index3 < results.size(); index3++)
                (newIndexes[index2])->push_back(results[index3][index]);
            index2++;
        } // Non-empty results
        index++;
    } // While results to test
    if (index2 != newIndexes.size())
        // Too few results, serious problem
        throw BaseException(__FILE__, __LINE__, "Index split failed, generated too many pieces");
}

// Outputs a play index for debugging
ostream& operator<<(ostream& stream, const PlayIndex& data)
{
    /* Clients treat an index as a view into the data store. What they care about is
        the data as ordered by the index, not the index itself. This routine outputs it
        accordingly */
    PlayIndex::const_iterator counter;
    for (counter = data.begin(); counter != data.end(); counter++)
        stream << **counter << endl; // NOTE: Double dereferene to get the actual data
    return stream;
}

ostream& operator<<(ostream& stream, const CategoryIndex& data)
{
    /* A category index is a series of play indexes indexed by category
        output each one in order */
    CategoryIndex::const_iterator counter;
    for (counter = data.begin(); counter != data.end(); counter++)
        stream << *counter;
    return stream;
}

ostream& operator<<(ostream& stream, const PlayIndexSet& data)
{
    /* The index set contains a number of different ways of looking at
        the data store. Clients care about the data as seen through the
        indexes, not the data itself. This routine extracts and outputs
        every index. It deliberately does NOT check which characteristics
        have indexes, to show cases where these get out of sync */
    stream << "Down number:" << endl << data.getIndex(SinglePlay::down_number) << endl;
    stream << "Distance needed:" << endl << data.getIndex(SinglePlay::distance_needed) << endl;
    stream << "Field location:" << endl << data.getIndex(SinglePlay::field_location) << endl;
    stream << "Time remaining:" << endl << data.getIndex(SinglePlay::time_remaining) << endl;
    stream << "Score differential:" << endl << data.getIndex(SinglePlay::score_differential) << endl;
    return stream;
}
