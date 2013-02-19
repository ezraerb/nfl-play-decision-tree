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
#include<set>
using std::set;

/* NOTE: Header deliberately not included. Sources should include it for their own
    purposes, and having it here makes it too essy to forget, leading to dependency problems */
using std::vector;

// NOTE: Header deliberately not included, sources should already have it for debugging
using std::ostream;
// Data about a single NFL play
class SinglePlay {
    public:
        /* Decision trees are built by testing attributes one by one. The code is
            simplest to implement when all attribute data can be handled identically.
            This object represents all data as things that can be cast to shorts.

            The data has both individual getters and a generic getter that takes an
            atribute type and returns a short value. The latter method will be called
            much more than the former. This brings up the question of whether a vector
            of shorts or individual data fields are more efficient. 'case' statements
            are highly optimized, and the later has less potential for data corruption,
            so I went with individual fields

            Certain data is derived, and can only be set at the end of the data read
            process. These fields have their own setter instead of using the constructor.
            The possibility of corruption is obvious, but can't be avoided in this case */

        // Play types are derived from those listed in the original data set
        enum PlayType { run_left, run_middle, run_right, pass_short_right, pass_short_middle,
                        pass_short_left, pass_deep_right, pass_deep_middle, pass_deep_left,
                        field_goal, punt };

        // Characteristics shown to affect play selection
        enum PlayCharacteristic { down_number, distance_needed, field_location,
                                    time_remaining, score_differential };

        /* Distance needed to make a first down. Grouping distance by category leads to
            a cleaner tree than trying to select on it directly, which is heavily affected
            by outlyers */
        enum DistanceNeeded { over_twenty, twenty_to_ten, ten_to_four, four_to_one, one_or_less };

        /* Location on field at time of play. The categories are based on reseach on
            where it affects play selection. NOTE: 'red zone' here means the 10 yards
            closest to the goal line; most commentators define it as 20 yards */
        enum FieldLocation { own_red_zone, middle, opp_red_zone };

        /* Time remaining in half. The categories are based on research on when it
            becomes important enough to affect play selection */
        enum TimeRemaining { outside_two_minutes, inside_two_minutes };

        /* Score differential. The categories are based on research on when it will
            affect play calling */
        enum ScoreDifferential { down_over_fourteen, down_over_seven, down_seven_less,
                                 even, up_seven_less, up_over_seven, up_over_fourteen };

        // Constructor, supply all specified data
        SinglePlay(unsigned int refId, PlayType playType, short down, short distanceNeeded,
                   short yardLine, short minutes, short ownScore, short oppScore,
                   short distanceGained, bool turnedOver);

        // Returns the number of categories for a category based characeristic (0 for others)
        static unsigned short getCategoryCount(PlayCharacteristic playCharacteristic);

        // Returns the number of different types of plays processed
        static unsigned short getPlayTypeCount();

        // Convert a distance needed into a distance category
        static DistanceNeeded distanceToDistanceNeeded(short distanceNeeded);

         // Convert a field yardage into location
        static FieldLocation yardsToFieldLocation(short yardLine);

        // Convert a minute count to time remaining category
        static TimeRemaining minutesToTimeRemaining(short minutes);

        static ScoreDifferential scoreToScoreDifferential(short ownScore, short oppScore);

        /* Gets the reference ID for this play. The ID is used to trace it through the
            system for debugging purposes */
        unsigned int getRefId() const;

        // Getters
        PlayType getPlayType() const;
        short getDistanceGained() const;
        bool getTurnedOver() const;

        short getDown() const;
        DistanceNeeded getDistanceNeeded() const;
        FieldLocation getFieldLocation() const;
        TimeRemaining getTimeRemaining() const;
        ScoreDifferential getScoreDifferential() const;

        // Getter by characteristic
        short getValue(PlayCharacteristic characteristic) const;

    private:
        /* Reference ID, used to trace a play through the system for debugging. Clients must
            set these and ensure the level of integrity needed */
        unsigned int _refId;
        PlayType _playType;
        unsigned char _down;
        DistanceNeeded _distanceNeeded;
        FieldLocation _fieldLocation;
        TimeRemaining _timeRemaining;
        ScoreDifferential _scoreDifferential;
        short _distanceGained;
        bool _turnedOver;
};

typedef vector<SinglePlay> PlayVector;
typedef PlayVector::const_iterator PlayIterator;

typedef set<SinglePlay::PlayCharacteristic> PlayCharacteristicSet;

// Method to output play types
ostream& operator<<(ostream& stream, SinglePlay::PlayType playType);

// Method to output play characteristics
ostream& operator<<(ostream& stream, SinglePlay::PlayCharacteristic playCharacteristic);
ostream& operator<<(ostream& stream, SinglePlay::DistanceNeeded distanceNeeded);
ostream& operator<<(ostream& stream, SinglePlay::FieldLocation fieldLocation);
ostream& operator<<(ostream& stream, SinglePlay::TimeRemaining timeRemaining);
ostream& operator<<(ostream& stream, SinglePlay::ScoreDifferential scoreDifferential);

// Method to output a play for debugging purposes
ostream& operator<<(ostream& stream, const SinglePlay& singlePlay);

// Returns the number of different types of plays processed
inline unsigned short SinglePlay::getPlayTypeCount()
{
    // WARNING: This MUST be kept in sync with the list of valid plays!
    // Remember to add 1 since enums are indexed from zero
    return (unsigned short)punt + 1;
}

// Returns the number of categories for a category based characeristic (0 for others)
inline unsigned short SinglePlay::getCategoryCount(PlayCharacteristic playCharacteristic)
{
    switch(playCharacteristic) {
        case down_number:
            /* Officially continuous, but has so few valid values its better processed
                as a category per value
                SEMI-HACK: Downs range from 1 to 4, but enums always start at zero. To
                make downs match up, pretend down 0 exists, giving it a size of five
                (The alternative is to subtract or add 1 everywhere for downs, which gets
                 even clunkier) */
            return 5;
            break;
        case distance_needed:
            return (unsigned short)one_or_less + 1;
            break;

        case field_location:
            // SUBTLE NOTE: Remember that enums are indexed from 0
            return (unsigned short)opp_red_zone + 1;
            break;

        case time_remaining:
            return (unsigned short)inside_two_minutes + 1;
            break;

        case score_differential:
            return (unsigned short)up_over_fourteen + 1;
            break;

        default:
            return 0; // Not a category characteristic
            break;
    }
}

// Convert a distance needed into a distance category
inline SinglePlay::DistanceNeeded SinglePlay::distanceToDistanceNeeded(short distanceNeeded)
{
    /* NOTE: distance is done in increasing order, so most likely
        values occur first */
    if (distanceNeeded <= 1)
        return one_or_less;
    else if (distanceNeeded <= 4)
        return four_to_one;
    else if (distanceNeeded <= 10)
        return ten_to_four;
    else if (distanceNeeded < 20)
        return twenty_to_ten;
    else
        return over_twenty;
}



// Convert a field yardage into location
inline SinglePlay::FieldLocation SinglePlay::yardsToFieldLocation(short yardLine)
{
    // In the data, yardage is always given in terms of offence yards to go
    if (yardLine >= 90)
        return own_red_zone;
    else if (yardLine > 10)
        return middle;
    else
        return opp_red_zone;
}

// Convert a minute count to time remaining category
inline SinglePlay::TimeRemaining SinglePlay::minutesToTimeRemaining(short minutes)
{
    // Game time in data is specifed in as time remaining in the overall game
    if ((minutes < 2) || ((minutes >= 30) && (minutes < 32)))
        return inside_two_minutes;
    else
        return outside_two_minutes;
}

// Convert two scores into a score differential category
inline SinglePlay::ScoreDifferential SinglePlay::scoreToScoreDifferential(short ownScore, short oppScore)
{
    short scoreDiff = ownScore - oppScore;
    if (scoreDiff < -14)
        return down_over_fourteen;
    else if (scoreDiff < -7)
        return down_over_seven;
    else if (scoreDiff < 0)
        return down_seven_less;
    else if (!scoreDiff)
        return even;
    else if (scoreDiff <= 7)
        return up_seven_less;
    else if (scoreDiff <= 14)
        return up_over_seven;
    else
        return up_over_fourteen;
}

inline unsigned int SinglePlay::getRefId() const
{ return _refId; }

inline SinglePlay::PlayType SinglePlay::getPlayType() const
{ return _playType; }

inline short SinglePlay::getDown() const
{ return (short)_down; }

inline SinglePlay::DistanceNeeded SinglePlay::getDistanceNeeded() const
{ return _distanceNeeded; }

inline SinglePlay::FieldLocation SinglePlay::getFieldLocation() const
{ return _fieldLocation; }

inline SinglePlay::TimeRemaining SinglePlay::getTimeRemaining() const
{ return _timeRemaining; }

inline SinglePlay::ScoreDifferential SinglePlay::getScoreDifferential() const
{ return _scoreDifferential; }

inline short SinglePlay::getValue(SinglePlay::PlayCharacteristic characteristic) const
{
    switch (characteristic) {
        // SUBTLE NOTE: Don't need break statements due to the returns, but people expect them
    case down_number:
        return (short)_down;
        break;
    case distance_needed:
        return (short)_distanceNeeded;
        break;
    case field_location:
        return (short)_fieldLocation;
        break;
    case time_remaining:
        return (short)_timeRemaining;
        break;
    case score_differential:
        return (short)_scoreDifferential;
        break;
    default: // Keep the compiler happy
        return 0;
    };
}

inline short SinglePlay::getDistanceGained() const
{
    return _distanceGained;
}

inline bool SinglePlay::getTurnedOver() const
{
    return _turnedOver;
}
