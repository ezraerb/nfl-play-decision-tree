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
#include<map>
#include<cmath> // Needed for inline methods

/* These classes represent nodes within the decision tree. The class
    can represent either a decision node or a leaf depending on which
    data fields are filled in.

    The design involves major tradeoffs. Trees normally cry out for
    whole-part design patterns, where all nodes can be processed identically.
    The problem with that approach is that decision trees are pruned after
    being built, which can convert decision nodes into leaves. That part
    would require a different design pattern that can handle state. They
    exist, but combining the two would create a really heavyweight design.
    It will show up most painfully in method calls, which will require TWO
    virtual methods to handle anything.

    This code goes for the opposite approach, a single class that looks like
    one or the other to callers depending on how it is set up. Its MUCH less
    robust, since corrupting the data it contains will effectively change its
    role. The design tries to manage it by strictly limiting methods that
    change internal data

    The class is deliberately designed so that building any node automatically
    builds all nodes underneath it as well. This was done to ensure integity */

using std::vector; // Header not included, since all callers also use vectors
using std::ostream;
using std::map;

typedef map<SinglePlay::PlayType, unsigned short> PlayCountMap;

class DecisionNode {
public:
    // Lower limit of information gain ratio where a split is valuable
    static const double MinInformationGain;

    /* Constructor. Requires a set of indexes into the play store, and summary data
        about all plays (not just those in this particular index set
        WARNING: Indexes are modified thanks to the splitting proecess */
    DecisionNode(PlayIndexSet& indexes, const OverallSummaryData& summaryData);

    // Destructor
    ~DecisionNode();

    // Prune the decision tree at this node and below
    void pruneTree();

    // Get the set of plays used in the past given situation characteristics
    /* WARNING: Must match the fields, minus the play type, in
        DataStore::insertPlay() ! */
    const DetailedPlayData& findPlays(short down, short distanceNeeded, short yardLine,
                                      short minutes, short ownScore, short oppScore) const;

    // Dumps the tree to an output stream
    void debugOutputData(ostream& stream) const;

 private:
    // List of child nodes. Any node missing this is a leaf
    vector<DecisionNode*> _childNodes;
    // Atribute to use to choose a child. Applies to non-leaves only
    SinglePlay::PlayCharacteristic _decisionValue;
    /* For category based characteristics, the mapping between categories and
        children. It exists because multiple categories may point to the same
        child */
    vector<short> _categoryChildMapping;

    // Data about plays in this branch. Should be set for leaves only
    DetailedPlayData _playData;

    /* Get the set of plays used in the past given situation characteristics.
        This version takes category values */
    const DetailedPlayData& findPlays(short down, short distanceNeeded,
                                      SinglePlay::FieldLocation fieldLocation,
                                      SinglePlay::TimeRemaining timeRemaining,
                                      SinglePlay::ScoreDifferential scoreDifferential) const;

    // Converts a set of indexes into data about the plays in the data store
    void indexesToPlayCounts(const PlayIndexSet& indexes, PlayCountMap& playData);

    // Converts an index into data about the plays in the index
    void indexesToPlayCounts(const PlayIndex& index, PlayCountMap& data);

    // Returns the information gain ratio for a given split of plays
    double getInfoGainRatio(const PlayCountMap& plays, short playTotal,
                            vector<vector<short> >& splitPlayCounts, vector<short> splitPlayTotals);

    // Returns the information of a group of plays of one type
    double getInformation(short playCount, short groupCount);

    // Returns whether this node is a leaf. Deliberately private
    bool isLeaf() const;

    /* Internal output method. It takes a 'level' so child nodes are offset from
        their parents in the output */
    void debugOutputData(ostream& stream, short level, vector<bool>& lastNode) const;

    // Outputs a leader showing node relationships
    void debugOutputLeader(ostream& stream, short level, vector<bool>& lastNode) const;

    // Prohibit copying, which screws up the pointers
    DecisionNode(const DecisionNode& other);
    const DecisionNode& operator=(const DecisionNode& other);
};

// Output operator
ostream& operator<<(ostream& stream, const DecisionNode& node);

// Get the set of plays used in the past given situation characteristics
/* WARNING: Must match the fields, minus the play type, in
    DataStore::insertPlay() ! */
inline const DetailedPlayData& DecisionNode::findPlays(short down, short distanceNeeded, short yardLine,
                                                       short minutes, short ownScore, short oppScore) const
{
    // Convert values to category values and call
    return findPlays(down, distanceNeeded, SinglePlay::yardsToFieldLocation(yardLine),
                     SinglePlay::minutesToTimeRemaining(minutes),
                     SinglePlay::scoreToScoreDifferential(ownScore, oppScore));
}

inline double DecisionNode::getInformation(short playCount, short groupCount)
{
    double ratio = (double)playCount / (double)groupCount;
    return -ratio * log2(ratio);
}

// Returns whether this node is a leaf. Deliberately private
inline bool DecisionNode::isLeaf() const
{
    return _childNodes.empty();
}


inline void DecisionNode::debugOutputData(ostream& stream) const
{
    // Assume this method is called on the root only
    vector<bool> lastNode;
    debugOutputData(stream, 0, lastNode);
}
