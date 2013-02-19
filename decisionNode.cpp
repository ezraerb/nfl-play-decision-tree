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
#include<iostream>
#include<cstdlib>
#include<bitset>
#include"singlePlay.h"
#include"playIndexSet.h"
#include"playStats.h"
#include"decisionNode.h"
#include"baseException.h"

using std::vector;
using std::map;
using std::ostream;
using std::make_pair;
using std::bitset;

using std::cerr;
using std::endl;

// Lower limit of information gain ratio where a split is valuable
const double DecisionNode::MinInformationGain = 0.02;

/* Constructor. Requires a set of indexes into the play store.
    WARNING: Indexes are modified thanks to the splitting proecess */
DecisionNode::DecisionNode(PlayIndexSet& indexes, const OverallSummaryData& summaryData)
    : _childNodes(), _categoryChildMapping(), _playData()
{
    /* First, assemble data about the plays in the index. Need the number of play types and
        the number of plays per type. A map handles this nicely */
    PlayCountMap playTypeCounts;
    indexesToPlayCounts(indexes, playTypeCounts);

    // If the play data is empty, so are the indexes. This indicates a serious problem
    if (playTypeCounts.empty())
        throw BaseException(__FILE__, __LINE__, "DecisionNode create failed, passed play store empty");

    /* At this point, need to find the characteristic split that will best divide
        the plays in the index. Its determined through a value called the
        information gain ratio. When the highest ratio possible falls below the
        stopping value, this node should be created as a leaf. Otherwise it becomes
        a decision node */
    double maxInfoRatio = 0.0;

    /* If the play data has only one play type, no further splitting is possible.
        Information gain ratio is zero */
    if (playTypeCounts.size() > 1) {
        PlayCharacteristicSet::const_iterator testIndex;

        // Default vector of counts per play type
        vector<short> defaultPlayCounts(SinglePlay::getPlayTypeCount(), 0);

        /* Indexes found to be redundant are dropped. Make a copy of the available set
            here, to ensure iterators are stable */
        PlayCharacteristicSet testCharacteristics(indexes.getIndexesAvailable());
        for (testIndex = testCharacteristics.begin(); testIndex != testCharacteristics.end();
             testIndex++) {
            double infoRatio = 0.0; // Informaion gain ratio for this index

            /* Using constant references is normally bad, but the routine
                has considerably logic and copying an index is expensive */
            const CategoryIndex& splitIndex = indexes.getIndex(*testIndex);

            /* Category indexes split by category type. Find the play counts for each
                and use them to find the information gain */
            vector<vector<short> > splitPlayCounts;
            vector<short> splitPlayTotals;
            short playTotals = 0;

            CategoryIndex::const_iterator catIndex;
            PlayIndex::const_iterator playIndex;
            for (catIndex = splitIndex.begin(); catIndex != splitIndex.end(); catIndex++)
                if (!catIndex->empty()) {
                    splitPlayCounts.push_back(defaultPlayCounts);
                    for (playIndex = catIndex->begin(); playIndex != catIndex->end(); playIndex++)
                        splitPlayCounts.back().at((unsigned short)((*playIndex)->getPlayType()))++;
                    splitPlayTotals.push_back(catIndex->size());
                    playTotals += catIndex->size();
                } // Category has plays

            // If all plays are in one category, the information gain by definition is zero
            if (splitPlayTotals.size() <= 1)
                infoRatio = 0.0;
            else
                infoRatio = getInfoGainRatio(playTypeCounts, playTotals, splitPlayCounts, splitPlayTotals);

            if (infoRatio < MinInformationGain)
            // Index can't be used for splitting, so its redundant
                indexes.dropIndex(*testIndex);

            else {
                _decisionValue = *testIndex;
                maxInfoRatio = infoRatio;
            } // Characteristic is best split found so far
        } // For each characteristic with an index defined
    } // Multiple play types within the indexes

    /* If information gain is greater than the minimum for a split, create a decision
        node, otherwise create a leaf */
    if (maxInfoRatio >= MinInformationGain) {
        /* Children will only be created for values with plays. The order will be the same
            as the order of the categories. Use this to create the mapping from categories
            to children. It needs to be done here because the split below will change the index */
        const CategoryIndex& splitIndex = indexes.getIndex(_decisionValue);
        _categoryChildMapping.assign(splitIndex.size(), -1); // 0 is a valid value!
        short valueCount = 0;
        unsigned short catIndex;
        for (catIndex = 0; catIndex < splitIndex.size(); catIndex++)
            if (splitIndex[catIndex].size() > 0) {
                _categoryChildMapping.at(catIndex) = valueCount;
                valueCount++;
            } // Category with values

        vector<PlayIndexSet> newIndexes(indexes.splitIndexByCharacteristic(_decisionValue));

        /* If the new indexes are empty, something went seriously wrong. About to start an infinite loop
            (because the indxes just attempt to be split will be pasesed to a new node, which won't be
            able to split them either) so throw an exception */
        if (newIndexes.empty())
            throw BaseException(__FILE__, __LINE__, "DecisionNode create failed, split of play store data failed");

        // Partially constructed objects are NOT deallocated on exception. Need to handle explictly
        try {
            _childNodes.push_back(new DecisionNode(indexes, summaryData));
            vector<PlayIndexSet>::iterator newIndexesPtr;
            for (newIndexesPtr = newIndexes.begin(); newIndexesPtr != newIndexes.end();
                 newIndexesPtr++)
                _childNodes.push_back(new DecisionNode(*newIndexesPtr, summaryData));
        } // Try block
        catch (...) {
            vector<DecisionNode*>::iterator index;
            for (index = _childNodes.begin(); index != _childNodes.end(); index++)
                delete *index;
            throw;
        } // Catch any exception
    } // High enough information gain for a decision node
    else
        // Convert the indexes into statistics
        PlaySummaryFactory::buildDetailedData(indexes, summaryData, _playData);
}

// Destructor
DecisionNode::~DecisionNode()
{
    vector<DecisionNode*>::iterator index;
    for (index = _childNodes.begin(); index != _childNodes.end(); index++)
        delete *index;
}

// Converts an indexes into data about the plays in the data store
void DecisionNode::indexesToPlayCounts(const PlayIndexSet& indexes, PlayCountMap& playData)
{
    playData.clear();
    // Extract the characteristics with indexes. If empty, have nothing to do
    if (indexes.getIndexesAvailable().empty())
        return;

    SinglePlay::PlayCharacteristic indexType = *(indexes.getIndexesAvailable().begin());
    /* NOTE: Using constant references is normally bad, but in this case the routine
        does complex logic and copying the index will be expensive */
    const CategoryIndex& catIndex = indexes.getIndex(indexType);
    CategoryIndex::const_iterator tempPtr;
    for (tempPtr = catIndex.begin(); tempPtr != catIndex.end(); tempPtr++)
        indexesToPlayCounts(*tempPtr, playData);
}

// Prune the decision tree at this node and below
void DecisionNode::pruneTree()
{
    /* NFL play calling is probablity based, not exact. That causes big problems for
        information gain based splitting, because it will split plays long after
        the point coaches would make actual decisions. Information gain algorirhms
        are also incredibly sensitive to noisy data and outlyers. The pruning phase
        attempts to correct for these situations. */
    if (isLeaf())
        // Leaf node, nothing to do!
        return;

    /* Scan through the child nodes. For each one that is not already a leaf, try to prune the nodes
        below that node. If the pruning fails and it remains a decision node, can stop afterwards
        TRICKY NOTE: Still need to prune nodes blow even if this node will not be pruned, so can't
        just stop after knowing this node can't be pruned */
    bool haveLeaves = true;
    vector<DecisionNode*>::iterator nodeIndex;
    for (nodeIndex = _childNodes.begin(); nodeIndex != _childNodes.end(); nodeIndex++) {
        if (!((*nodeIndex)->isLeaf())) {
            (*nodeIndex)->pruneTree();
            haveLeaves = haveLeaves && (*nodeIndex)->isLeaf();
        } // Not a leaf node
    } // While nodes to test and reason to do so

    if (!haveLeaves)
        // Have decison nodes below this one, can't prune
        return;

    /* Every NFL play occurs often enough that a leaf containing a single play almost
        certainly the result of splitting a probability based entry. If all leaves, or
        all but one leaf, fall in this category, assume this is the case and prune them */
    unsigned short singlePlayLeafCount = 0;
    for (nodeIndex = _childNodes.begin(); nodeIndex != _childNodes.end(); nodeIndex++)
        if ((*nodeIndex)->_playData.size() == 1) // Only one type of play in the summary
            if ((*nodeIndex)->_playData.begin()->second.getPlayCount() == 1) // Play type has only one play
                singlePlayLeafCount++;
    bool pruneTree = (singlePlayLeafCount >= _childNodes.size() - 1);
    if (!pruneTree) {
        /* Error rate algorithms treat the ideal tree layout as every node having only one
            type of play. This normally doesn't happen in practice. Consider the play type
            with the highest count the 'intended' play type for the node, and the remainder as
            'contaminents'. The percentage of the plays that are contaminents is the error rate.
            Children should be pruned if their combined error rate is less than the error rate for
            the plays in all children combined.

            NFL plays are called by probability, so nodes can contain many play types. Strict error
            rate algorithms don't work. Applying the algorithm shows that nodes are merged when either
            the error percentage for all children is low AND they contain the same intended play type, OR
            one node has low error rate and many more plays than other children. This algorithm
            generalizes that result by finding which plays have the highest percentage and comparing them.

            WARNING: Overfitting means that many nodes may have few plays, so a single play can have a
            percentage high enough to consider. Single plays found in only one node are ignored provided
            that they form less than half the combined play data

            NASTY HACK: <set> does not handle proper set operations, so bit arrays indexed by play type
            are used instead. Having a bit set indicates the play type is in the set. The C++ STL does
            not currently support a dynamically sized bit array (Boost does) so the size is hardcoded. If
            the number of play types changes, this must be updated to match or bad things will happen! */
        typedef bitset<11> PlayTypeBitSet; // Use typedef so size is in only one place

        unsigned short totalPlayCount;
        PlayTypeBitSet singlePlays; // Only one play in at least one child
        PlayTypeBitSet multiPlays; // More than one play in at least one child

        PlayTypeBitSet significantPlays; // Plays considered significant for this child
        PlayTypeBitSet anySignificant; // Significant plays in ANY child
        PlayTypeBitSet allSignificant; // Significant plays in ALL children, initialized to all TRUE
        allSignificant.flip();

        DetailedPlayData::iterator playIndex;
        for (nodeIndex = _childNodes.begin(); nodeIndex != _childNodes.end(); nodeIndex++) {
            // Accumulate single plays and multiple plays, and find the most frequent play
            short mostFrequentPlay = 0;
            for (playIndex = (*nodeIndex)->_playData.begin(); playIndex != (*nodeIndex)->_playData.end();
                 playIndex++) {
                // Accumulate to combined play total for nodes, needed below
                totalPlayCount += playIndex->second.getPlayCount();
                if (playIndex->second.getPlayCount() > 1)
                    multiPlays.set((unsigned short)playIndex->first);
                else
                    singlePlays.set((unsigned short)playIndex->first);
                if (playIndex->second.getPlayCount() > mostFrequentPlay)
                    mostFrequentPlay = playIndex->second.getPlayCount();
            } // For loop through the plays

            // Find significant plays
            /* SUBTLE NOTE: Remember that this is based on their percentage within the existing node,
                not their percentage if the nodes were combined */
            unsigned short maxPercentage = 0;
            for (playIndex = (*nodeIndex)->_playData.begin(); playIndex != (*nodeIndex)->_playData.end(); playIndex++)
                if (playIndex->second.getPercentOfConditionPlays() > maxPercentage)
                    maxPercentage = playIndex->second.getPercentOfConditionPlays();

            // Set the threshold for 'significant' to 3/4 of the most frequent play
            maxPercentage *= 3;
            maxPercentage /= 4;

            significantPlays.reset();
            for (playIndex = (*nodeIndex)->_playData.begin(); playIndex != (*nodeIndex)->_playData.end(); playIndex++)
                /* SEMI-HACK: If the most frequent play does not appear often (so a single play greatly
                    changes the percentage) this test does not work well. To handle it, declare everything
                    to be significant in this case. Note that the single entry test should handle any
                    genuine outlyers picked up by accident */
                if ((playIndex->second.getPercentOfConditionPlays() >= maxPercentage) ||
                    (mostFrequentPlay <= 5))
                    significantPlays.set((unsigned short)playIndex->first);

            /* OR with the ANY list and AND with the ALL list. The former sets bits for plays
                found in this child not already set, and the latter resets bits for plays
                NOT found in this child */
            anySignificant |= significantPlays;
            allSignificant &= significantPlays;
        } // Loop through children

        /* If the list of significant play types in any child equals the list for all of them,
            the significant play types in all children are the same and should prune */
        if (anySignificant == allSignificant)
            pruneTree = true;
        else {
            /* Find the list of play types with ONLY single plays in child nodes.
                This is (play types with single plays) and NOT (play types with
                multiple plays) */
            multiPlays.flip();
            singlePlays &= multiPlays;

            /* Find the list of play types in only some children. This is
                (play types in any child) and NOT (play types in all children) */
            allSignificant.flip();
            anySignificant &= allSignificant;

            /* Find the play types in the above set that are ALSO single plays. If
                this equals the list of significant plays in only some children, AND
                their total is less than half the total plays, prune the node */
            singlePlays &= anySignificant;
            if ((singlePlays == anySignificant) &&
                (anySignificant.size() <= (totalPlayCount / 2)))
                pruneTree = true;
        } // Some significant play types are not in all children
    } // Tests so far did not result in a prune

    if (pruneTree) {
        // Combine their statistics
        _playData.swap(_childNodes.front()->_playData);
        for (nodeIndex = _childNodes.begin() + 1; nodeIndex != _childNodes.end(); nodeIndex++)
            PlaySummaryFactory::mergeData(_playData, (*nodeIndex)->_playData);

        // Remove children
        for (nodeIndex = _childNodes.begin(); nodeIndex != _childNodes.end(); nodeIndex++)
            delete *nodeIndex;
        _childNodes.clear();
    } // Children should be merged
    return;
}

// Converts an index into data about the plays in the index
void DecisionNode::indexesToPlayCounts(const PlayIndex& index, PlayCountMap& data)
{
    PlayIndex::const_iterator indexPtr;
    PlayCountMap::iterator dataPtr;
    for (indexPtr = index.begin(); indexPtr != index.end(); indexPtr++) {
        dataPtr = data.find((*indexPtr)->getPlayType());
        if (dataPtr != data.end())
            dataPtr->second++;
        else
            data.insert(make_pair((*indexPtr)->getPlayType(), 1));
    } // For every play in the index
}

// Returns the information gain ratio for a given split of plays
double DecisionNode::getInfoGainRatio(const PlayCountMap& plays, short playTotal,
                                      vector<vector<short> >& splitPlayCounts,
                                      vector<short> splitPlayTotals)
{
    /* Partition tests are based on information gain theory. The test is
        derived as follows:
            The system starts out as a collection of plays. The goal is to
        divide them into categories using their attributes. Ideally, the splitting
        should minimize the number of characteristics needed to find the final
        category. The full solution is NP hard, so this algorithm uses huerestics.
        A given collection of plays requires a certain amount of future splits,
        approximated by a value called its Information. The divisions should be
        done such that the amount of information needed afterward to finish the
        division is as small as possible. A quantity called the "Information
        Gain Ratio" states which split does this best.

        Some definitions:
        D: plays to classify
        d: size of the above
        C: play types within D
        C[i]: a single play type, indexed by i
        c: number of different play types in D
        P[i]: plays within D that have C[i] play type
        p[i]: numbber of plays in P[i]

        The information content of D is defined as
        I(D) = sum[1..c](-(p[i]/d)log2(p[i]/d))

        Note that p[i] <= d, so the log is negative, requiring the minus sign to flip it

        Also note that sum[1..c]p[i] = d, so the more evenly distributed
        the plays between the types, the higher the information content will
        be. The more even the distribution, the more splits needed to seperate
        out the plays.

        A given characteristic will divide D into multiple non-overlapping
        subsets. Each of these is also a collection of plays, meaning every
        one has its own information content. If the split is chosen well,
        the information contents of the subsets will be significantly smaller
        than the original set, because the play types will be less evenly
        distributed within the subsets than the original set. The difference
        is the Information Gain

        More definitions:
        D[k]: a subset of D, indexed by k
        d[k]: number of elements in D[k]
        Information Gain IG(D,k) = I(D) - sum[1..k]((d[k]/d)I(D(k))

        Information gain has a nasty bias: it prefers characteristics that
        generate lots of little subsets over ones that generate a few larger
        subsets. In other words, the split by itself appears to reducee the information
        needed in the future, regardless of the content of the resulting subsets.
        Information gain ratio attempt to correct this, by calculating the maximum
        information gain possible from each split. The actual split closest to its
        maximum gets chosen. The theoretical maximim is called the Intrinsic
        Information Value.

        The theoretical maximim gain happens when the split divides the set into subsets
        that only contain a single play type each. The inforation needed for future
        splits would be zero. The information gain for his case would thus be equal
        to the information of the original set, which is:

        IIV(D,k) = sum[1..k]((d[k]/d)log2(d[k]/d))

        Note carefully that the values are the sizes of the SUBSETS, since every
        subset in this case contains plays of a single type.

        The information gain ratio is IG(D,k) / IIV(D,k).
    */
    unsigned short counter, counter2;

    // First, find the information of the entire group of plays
    double groupInformation = 0.0;
    PlayCountMap::const_iterator mapCounter;
    for (mapCounter = plays.begin(); mapCounter != plays.end(); mapCounter++)
        groupInformation += getInformation(mapCounter->second, playTotal);

    // Now, SUBTRACT the information for each split subset
    for (counter = 0; counter < splitPlayCounts.size(); counter++) {
        double splitInformation = 0.0;
        for (counter2 = 0; counter2 < splitPlayCounts[counter].size(); counter2++)
            if (splitPlayCounts[counter][counter2] != 0) // This is indexed by play type, so some may have no count
                splitInformation += getInformation(splitPlayCounts[counter][counter2], splitPlayTotals[counter]);
        groupInformation -= ((splitInformation * (double)splitPlayTotals[counter]) / (double)playTotal);
    } // Loop through splits

    // Find the intrinsic information value of the original group
    double intrinsicValue = 0.0;
    for (counter = 0; counter < splitPlayTotals.size(); counter++)
        intrinsicValue += getInformation(splitPlayTotals[counter], playTotal);
    return groupInformation / intrinsicValue;
}

/* Get the set of plays used in the past given situation characteristics.
    This version takes category values */
const DetailedPlayData& DecisionNode::findPlays(short down, short distanceNeeded,
                                                SinglePlay::FieldLocation fieldLocation,
                                                SinglePlay::TimeRemaining timeRemaining,
                                                SinglePlay::ScoreDifferential scoreDifferential) const
{
    // If this is a leaf, return the plays it contains
    if (isLeaf())
        return _playData;

    // Extract a test value from the input based on the characteristic used to split
    short testValue;
    switch (_decisionValue) {
    case SinglePlay::down_number:
        testValue = down;
        break;
    case SinglePlay::distance_needed:
        testValue = distanceNeeded;
        break;
    case SinglePlay::field_location:
        testValue = (short)fieldLocation;
        break;
    case SinglePlay::time_remaining:
        testValue = (short)timeRemaining;
        break;
    case SinglePlay::score_differential:
        testValue = (short)scoreDifferential;
        break;
    default: // Keep the compiler happy
        testValue = 0;
    } // Switch statement

    /* Look up the value in the mapping to children, and then
        call the appropriate child. Note that if the training set
        did not have anything for the given category value, no plays
        will be found, and an empty set will be returned */
    short childIndex = _categoryChildMapping.at(testValue);
    if (childIndex >= 0)
        return _childNodes.at(childIndex)->findPlays(down, distanceNeeded, fieldLocation,
                                                     timeRemaining, scoreDifferential);
    else
        return _playData; // Set empty at construction
}

    /* Internal output method. It takes a 'level' so child nodes are offset from
        their parents in the output, plus data to format it properly  */
void DecisionNode::debugOutputData(ostream& stream, short level, vector<bool>& lastNode) const
{
    /* Children are linked to their parents by vertical lines. For every level but
        the last, this node is not related to those nodes, so output '| ' if this node
        is not a child of a final node from a split. For the last level, this node is a
        child, so output '|-' followed by the value from the parent's split */
    debugOutputLeader(stream, level, lastNode);
    if (!isLeaf()) {
        lastNode.push_back(false); // Extend list for children about to process
        stream << "Split: " << _decisionValue << endl;
        short index;
        for (index = 0; index < (short)_categoryChildMapping.size(); index++)
            if (_categoryChildMapping[index] >= 0) {
                // Flag the last child
                if (_categoryChildMapping[index] == (short)_childNodes.size() - 1)
                    lastNode.back() = true;
                debugOutputLeader(stream, level, lastNode);
                stream << "Value:";
                // Output the value based on the split characteristic
                switch (_decisionValue) {
                    case SinglePlay::distance_needed:
                        stream << (SinglePlay::DistanceNeeded)index;
                        break;

                    case SinglePlay::field_location:
                        stream << (SinglePlay::FieldLocation)index;
                        break;

                    case SinglePlay::time_remaining:
                        stream << (SinglePlay::TimeRemaining)index;
                        break;

                    case SinglePlay::score_differential:
                        stream << (SinglePlay::ScoreDifferential)index;
                        break;

                    default:
                        stream << index;
                } // Switch on split characteristic
                stream << endl;
                _childNodes[_categoryChildMapping[index]]->debugOutputData(stream, level + 1, lastNode);
            } // This category has a child node
        lastNode.pop_back(); // Remove bit inserted above so doesn't carry over
    } // Decision node
    else {
        // Leaf, output play summary data on one line per play type
        DetailedPlayData::const_iterator playPtr;
        for (playPtr = _playData.begin(); playPtr != _playData.end(); playPtr++) {
            if (playPtr != _playData.begin())
                debugOutputLeader(stream, level, lastNode);
            stream << playPtr->first << ": " << playPtr->second << endl;
        } // For each play in the list
    } // Leaf node
}

// Outputs a leader showing node relationships
void DecisionNode::debugOutputLeader(ostream& stream, short level, vector<bool>& lastNode) const
{
    short index;
    for (index = 0; index < level; index++) {
        if (!lastNode[index])
            stream << "|";
        else
            stream << " ";
        stream << " ";
    }
}

// Output operator
ostream& operator<<(ostream& stream, const DecisionNode& node)
{
    node.debugOutputData(stream);
    return stream;
}
