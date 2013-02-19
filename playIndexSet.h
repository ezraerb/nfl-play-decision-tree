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

/* This class manages the indexes for a data store of plays. It exists to ensure
   integrity as indexes are manipulated due to splitting plays into nodes
   WARNING: Index references remain valid only as long as the underlying data store exists */

/* Only quantities that affect play selection are indexed. Distance made, variance of distance
    made and turnover percentage are not indexed because they are the result of the play, and
    coaches use these values from previous plays to select within a given situation. They are
    also derived, which cause a technical limitation: all plays of the same type will have the
    same value, making them appear to be the perfect play selector! */
using std::vector; // Clients make extensive use of vectors, so they should include header

// Index of plays. Double dereference iterators to get the play
typedef vector<PlayIterator> PlayIndex;

// Play indexes split by category. Individual indexes have no sort order
typedef vector<PlayIndex> CategoryIndex;

class PlayIndexSet {
    public:
        // Set indexes into object. Existing ones will be dropped
        void setIndexes(const CategoryIndex& downIndex, const CategoryIndex& distanceNeededIndex,
                        const CategoryIndex& fieldLocationIndex, const CategoryIndex& timeRemainingIndex,
                        const CategoryIndex& scoreDifferentialIndex);

        // Copy constructor
        PlayIndexSet(const PlayIndexSet& other);

        // Constructor, creates an empty object
        PlayIndexSet();

        // Assignment operator
        /* NOTE: This is here mostly for completeness. Using in in practice indicats some sort of problem */
        PlayIndexSet& operator=(const PlayIndexSet& other);

        // Returns a reference to a category based index. Other types return an empty index
        const CategoryIndex& getIndex(SinglePlay::PlayCharacteristic playCharacteristic) const;

        /* Splits an index by an attribute. Continuous attributes must supply a value to use for
            the split; category indexes will be divided by each possible value. This method exists because
            spliting indexes using the characteristc used to split the underlying plays will be faster
            that either recreating the indexes from the play set, or searcing in sets of plays (which is O(logN))
            to split the plays. For a split on a category based characteristic, the index for that
            characteristic is deleted from the index set since it no longer adds any value. This
            class will contain the first of the split indxes; the returned values will have the rest */
        vector<PlayIndexSet> splitIndexByCharacteristic(SinglePlay::PlayCharacteristic playCharacteristic);

        // Drops an index. This usually happens because it is redundant for splitting
        void dropIndex(SinglePlay::PlayCharacteristic playCharacteristic);

        // Returns characteristics with plays defined
        const PlayCharacteristicSet& getIndexesAvailable() const;

    private:
        /* Set of characteristics which have indexes. The set can either be derived on every
            call for the data or tracked seperately and updated with each drop. This clas does \
            the latter, since only one routine should update it, and it is read often, */
        PlayCharacteristicSet _indexes;

        // Indexes
        CategoryIndex _downIndex;
        CategoryIndex _distanceNeededIndex;
        CategoryIndex _fieldLocationIndex;
        CategoryIndex _timeRemainingIndex;
        CategoryIndex _scoreDifferentialIndex;

        /* Index return is by reference, need empty and stable indexes for
            characteristics not indexed */
        CategoryIndex _emptyCatIndex;

        // Split an index by a category characteristic
        /* NOTE: This method takes a vector of pointers. They should actually be references,
            but they can't be used in vectors because they don't have default values */
        void splitIndex(SinglePlay::PlayCharacteristic playCharacteristic, CategoryIndex& existIndex,
                        vector<CategoryIndex*> newIndexes);
        void splitIndexHelper(SinglePlay::PlayCharacteristic playCharacteristic, const PlayIndex& existIndex,
                              vector<PlayIndex>& newIndexes);
};

// Outputs a play index for debugging
ostream& operator<<(ostream& stream, const PlayIndex& data);
ostream& operator<<(ostream& stream, const CategoryIndex& data);
ostream& operator<<(ostream& stream, const PlayIndexSet& data);

// Returns a reference to a category based index. Other types return an empty index
inline const CategoryIndex& PlayIndexSet::getIndex(SinglePlay::PlayCharacteristic playCharacteristic) const
{
    switch (playCharacteristic) {
    case SinglePlay::down_number:
        return _downIndex;
        break;
    case SinglePlay::distance_needed:
        return _distanceNeededIndex;
        break;
    case SinglePlay::field_location:
        return _fieldLocationIndex;
        break;
    case SinglePlay::time_remaining:
        return _timeRemainingIndex;
        break;
    case SinglePlay::score_differential:
        return _scoreDifferentialIndex;
        break;
    default:
        return _emptyCatIndex;
        break;
    }
}

// Drops an index. This usually happens because it is redundant for splitting
inline void PlayIndexSet::dropIndex(SinglePlay::PlayCharacteristic playCharacteristic)
{
    // If the index to drop is the last one in the set, leave it alone
    if (_indexes.size() == 1)
        return;

    // Clearing an already cleared index does no damage, so don't check for it
    switch (playCharacteristic) {
    case SinglePlay::down_number:
        _downIndex.clear();
        break;
    case SinglePlay::distance_needed:
        _distanceNeededIndex.clear();
        break;
    case SinglePlay::field_location:
        _fieldLocationIndex.clear();
        break;
    case SinglePlay::time_remaining:
        _timeRemainingIndex.clear();
        break;
    case SinglePlay::score_differential:
        _scoreDifferentialIndex.clear();
        break;
    default:
        // Do nothing
        break;
    } // Case statement

    _indexes.erase(playCharacteristic);
}

// Returns characteristics with plays defined
inline const PlayCharacteristicSet& PlayIndexSet::getIndexesAvailable() const
{
    return _indexes;
}


