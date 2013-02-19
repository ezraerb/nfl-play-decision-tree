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

/* These classes defines statistics about plays. Two main ones
    exist, because two types of statistics are used. The short
    smmary is used for overall data about a play type, while more
    detailed data applies to the plays that meet wanted conditions */

using std::vector; // Header NOT included, since clients make extensive use of them and should include it
using std::ostream; // Ditto
using std::map;

typedef vector<short> DistanceVector;

class OverallPlaySummary {
public:
    OverallPlaySummary(const DistanceVector& playDistances, short turnoverCount);
    // Use default copy constructor, assignment operator, and destructor

    // Getters
    short getAverageDistance() const;
    short getDistanceVariance() const;
    short getTurnoverPercentage() const;
    short getTotalCount() const;

private:
    short _averageDistance; // Distance gained on play
    short _distanceVariance;
    short _turnoverPercentage;
    short _totalCount;
};

/* A full set of play data with one missing will be very rare, so use a vector
    indexed by play type to represent a collection of overall summary records */
typedef vector<OverallPlaySummary> OverallSummaryData;

// Detailed statistics about a group of plays of some type
class DetailedPlaySummary {
public:
    DetailedPlaySummary(const DistanceVector& playDistances, short turnoverCount,
                        short conditionPlayCount,
                        const OverallPlaySummary& overallTypeStatistics);

    // Use default copy constructor, assignment operator, and destructor

    /* Merge one summary into another. Used when combining statistics from
        similiar conditions */
    void merge(const DetailedPlaySummary& other, short totalMergedPlays);

    /* Changes the percentages for a given condition, to account for merges
        where the other set has no plays of this type */
    void updateConditionStats(short totalMergedPlays);

    // Getters
    const DistanceVector& getPlayDistances() const;
    short getAverageDistance() const; // Statistics on the above
    short getDistanceVariance() const;

    short getPlayCount() const; // Number of plays of this type in these conditions
    short getTurnoverCount() const; // Number of these plays turned over
    short getTurnoverPercentage() const; // In 0.1%

    // Play type as a percentage of plays for the given set of conditions
    short getPercentOfConditionPlays() const; // In %0.1%
    // Plays for this set of conditions as percent of all plays of this type
    short getPercentOfTypePlays() const; // In 0.1%

    // Overall statistics for the play type
    short getOverallAverageDistance() const;
    short getOverallDistanceVariance() const;
    short getOverallTurnoverPercentage() const;

private:
    DistanceVector _playDistances;
    short _turnoverCount; // Number of these plays turned over
    // Statistics for plays in this group
    OverallPlaySummary _groupStats;

    // Statistics for all plays of the current type
    OverallPlaySummary _overallStats;

    // Play type as a percentage of plays for the given set of conditions
    short _percentOfConditionPlays; // In %0.1%
    // Plays for this set of conditions as percent of all plays of this type
    short _percentOfTypePlays; // In 0.1%
};

/* Not every play will occur for every possible combination of conditions, so the
    data structure to use for a collection of them requires tradeoffs. Functionally,
    it needs to act like a sparsely populated array indexed by play type. The two
    structures usually used for this purpose are vectors and maps. Vectors take up
    little space but are painful to use thanks to the constant searching for the
    play type. Maps are easy to use but eat up memory. Most play collections will
    contain less than half the available types, so this design goes with a map */
typedef map<SinglePlay::PlayType, DetailedPlaySummary> DetailedPlayData;

// Build summaries from index data
class PlaySummaryFactory {
public:
    static void buildSummaryData(const PlayIndexSet& indexes, OverallSummaryData& data);

    static void buildDetailedData(const PlayIndexSet& indexes, const OverallSummaryData& overallData,
                                  DetailedPlayData& detailedData);

    // Merges two sets of play summaries together
    static void mergeData(DetailedPlayData& result, const DetailedPlayData& other);

private:
    // Assemble data about plays within an index, indexed by play type
    // WARNING: Vectors must be properly sized to number of play types beforehand
    static void indexesToCounts(const PlayIndexSet& indexes, vector<DistanceVector>& distances,
                                vector<short>& turnoverCounts);
};

// Output operator
ostream& operator<<(ostream& stream, const DetailedPlaySummary& data);

inline short OverallPlaySummary::getAverageDistance() const
{ return _averageDistance; }

inline short OverallPlaySummary::getDistanceVariance() const
{ return _distanceVariance; }

inline short OverallPlaySummary::getTurnoverPercentage() const
{ return _turnoverPercentage; }

inline short OverallPlaySummary::getTotalCount() const
{ return _totalCount; }

/* Changes the percentages for a given condition, to account for merges
    where the other set has no plays of this type */
inline void DetailedPlaySummary::updateConditionStats(short totalMergedPlays)
{
    _percentOfConditionPlays = ((short)_playDistances.size() * 1000) / totalMergedPlays;
}

inline const DistanceVector& DetailedPlaySummary::getPlayDistances() const
{ return _playDistances ; }

inline short DetailedPlaySummary::getAverageDistance() const
{ return _groupStats.getAverageDistance() ; }

inline short DetailedPlaySummary::getDistanceVariance() const
{ return _groupStats.getDistanceVariance() ; }

inline short DetailedPlaySummary::getPlayCount() const
{ return _playDistances.size(); }

inline short DetailedPlaySummary::getTurnoverCount() const
{ return _turnoverCount; }

inline short DetailedPlaySummary::getTurnoverPercentage() const
{ return _groupStats.getTurnoverPercentage(); }

// Play type as a percentage of plays for the given set of conditions
inline short DetailedPlaySummary::getPercentOfConditionPlays() const
{ return _percentOfConditionPlays; }

// Plays for this set of conditions as percent of all plays of this type
inline short DetailedPlaySummary::getPercentOfTypePlays() const
{ return _percentOfTypePlays; }

// Overall statistics for the play type
inline short DetailedPlaySummary::getOverallAverageDistance() const
{ return _overallStats.getAverageDistance(); }

inline short DetailedPlaySummary::getOverallDistanceVariance() const
{ return _overallStats.getDistanceVariance(); }

inline short DetailedPlaySummary::getOverallTurnoverPercentage() const
{ return _overallStats.getTurnoverPercentage(); }
