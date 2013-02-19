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

/* This file defines the data store for the NFL play decision tree.
    The items to be stored are plays, indexed by the conditions under
    which a play was chosen. The conditions can be processed in any order,
    so data is stored using a star arrangement of a single data array snd
    multiple index arrays. Indexes for atrributes with continuous values are sorted.

    Indexes can be split based on any attribute. Attrbutes with continuous values
    will require a splitting value. This ability exists because with a large data set
    it will be faster to split the index by comparing attribute values than searching
    for data indexes in a set. The latter is O(logN) while the former is constant.

    This class, like many of its type, contains both original and derived data.
    The plays inserted in the object are original; everything else is derived.
    The original and derived data can get out of sync, in which case really bad
    things will happen. There are four main ways of dealing with the problem:
    1. Derive data as new original data is inserted. This is very expensive.
    2. Derive data at the point its read. This is also expensive, since every
    read needs to be checked. Caching can reduce the compute, but now need to
    check when the cache gets out of date
    3. Implied state transitions. An object should have two distinct phases in
    its life cycle, with no overlap: one inserting data, and one reading it back
    out. The code can detect when this transition has occurred and do the derivation
    then. Its effectively a cheaper version of the cache in option 2, and still
    pretty expensive
    4. Explicit state transitions. The calling code determines when the state
    transitions take place and calls methods to flag them. This is by far the
    cheapest, but puts a logic burden on calling classes.

    All data should be inserted by a single class, so burdening it with state
    management is an acceptable design tradeoff. Method 4 is used by this class.

    State transitions have another consistency problem, that of enforcing state.
    Methods for one state called when the object is in another should raise errors.
    Testing for them is expensive. This design doesn't bother based on two
    observations: 1. All methods for states other than the final state should only
    be called in one class, so finding misuse is easy. 2. All data read out of the
    object will be done by index, which is derived. As long as the internal data stays
    consistent on insert, index methods will still be valid even after more inserts.
    The inserted data will be ignored, but this is acceptable */
using std::vector;

class DataStore {
    public:
        // Creates an empty data store
        DataStore();

        // Use the default destructor

        // Inserts a single play. It takes the data to avoid excess object copies
        void insertPlay(SinglePlay::PlayType playType, short down, short distanceNeeded,
                        short yardLine, short minutes, short ownScore, short oppScore,
                        short distanceGained, bool turnedOver);

        /* Build indexes and derive collective play data. Indicates insertion is done. Data inserted
            after calling this method will be ignored */
        void buildIndexes();

        /* Returns index data for this data store. A COPY is returned so the client can manipulate
            it as plays are divided up */
        PlayIndexSet getIndexes() const;

        // Summary data of the entire set of plays
        const OverallSummaryData& getPlaySummaryStats() const;

    private:
        PlayVector _data;

        // Indexes, as of the last time they were built
        PlayIndexSet _indexes;

        // Summary data of the entire set of plays
        OverallSummaryData _playSummaryStats;

        // Prohibit copying, which breaks index data
        DataStore(const DataStore& other);
        DataStore& operator=(const DataStore& other);
};

// Inserts a single play. It takes the data to avoid excess object copies
inline void DataStore::insertPlay(SinglePlay::PlayType playType, short down, short distanceNeeded,
                                  short yardLine, short minutes, short ownScore, short oppScore,
                                  short distanceGained, bool turnedOver)
{
    // Play data goes into the data list, accumulation data goes into total for play type
    // Use the current size of the store as the reference ID, should ensure uniqueness
    _data.push_back(SinglePlay(_data.size(), playType, down, distanceNeeded, yardLine,
                               minutes, ownScore, oppScore, distanceGained, turnedOver));
}


/* Returns index data for this data store. A COPY is returned so the client can manipulate
    it as plays are divided up */
inline PlayIndexSet DataStore::getIndexes() const
{
    return _indexes;
}

// Summary data of the entire set of plays. Reflects the same plays as indexed
inline const OverallSummaryData& DataStore::getPlaySummaryStats() const
{
    return _playSummaryStats;
}
