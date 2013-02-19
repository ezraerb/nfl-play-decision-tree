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
#include<ostream>
#include<vector>
#include"singlePlay.h"

using std::ostream;

// Constructor, supply all specified data
SinglePlay::SinglePlay(unsigned int refId, SinglePlay::PlayType playType, short down, short distanceNeeded,
                       short yardLine, short minutes, short ownScore, short oppScore, short distanceGained,
                       bool turnedOver)
{
    _refId = refId;
    _playType = playType;
    _down = down;
    _distanceNeeded = distanceToDistanceNeeded(distanceNeeded);
    _fieldLocation = yardsToFieldLocation(yardLine);
    _timeRemaining = minutesToTimeRemaining(minutes);
    _scoreDifferential = scoreToScoreDifferential(ownScore, oppScore);
    _distanceGained = distanceGained;
    _turnedOver = turnedOver;
}

// Output play type
ostream& operator<<(ostream& stream, SinglePlay::PlayType playType)
{
    switch (playType) {
    case SinglePlay::run_left:
        stream << "Run Left";
        break;

    case SinglePlay::run_middle:
        stream << "Run Up Middle";
        break;

    case SinglePlay::run_right:
        stream << "Run Right";
        break;

    case SinglePlay::pass_short_right:
        stream << "Short Pass Right";
        break;

    case SinglePlay::pass_short_middle:
        stream << "Short Pass Middle";
        break;

    case SinglePlay::pass_short_left:
        stream << "Short Pass Left";
        break;

    case SinglePlay::pass_deep_right:
        stream << "Deep Pass Right";
        break;

    case SinglePlay::pass_deep_middle:
        stream << "Deep Pass Middle";
        break;

   case SinglePlay::pass_deep_left:
        stream << "Deep Pass Left";
        break;

    case SinglePlay::field_goal:
        stream << "Field Goal Attempt";
        break;

    case SinglePlay::punt:
        stream << "Punt";
        break;

    default:
        stream << "UNKNOWN";
        break;
    }
    return stream;
}

// Output play classification characteristic
ostream& operator<<(ostream& stream, SinglePlay::PlayCharacteristic playCharacteristic)
{
    switch(playCharacteristic) {
    case SinglePlay::down_number:
        stream << "down_number";
        break;

    case SinglePlay::distance_needed:
        stream << "distance_needed";
        break;

    case SinglePlay::field_location:
        stream << "field_location";
        break;

    case SinglePlay::time_remaining:
        stream << "time_remaining";
        break;

    case SinglePlay::score_differential:
        stream << "score_differential";
        break;

    default:
        stream << "UNKNOWN";
        break;
    }
    return stream;
}

ostream& operator<<(ostream& stream, SinglePlay::DistanceNeeded distanceNeeded)
{
    switch (distanceNeeded) {
        case SinglePlay::over_twenty:
            stream << "over twenty yards";
            break;
        case SinglePlay::twenty_to_ten:
            stream << "ten to twenty yards";
            break;
        case SinglePlay::ten_to_four:
            stream << "four to ten yards";
            break;
        case SinglePlay::four_to_one:
            stream << "one to four yards";
            break;
        case SinglePlay::one_or_less:
            stream << "less than one yard";
            break;
        default:
            stream << "UNKNOWN";
            break;
    }
    return stream;
}

ostream& operator<<(ostream& stream, SinglePlay::FieldLocation fieldLocation)
{
    switch (fieldLocation) {
        case SinglePlay::own_red_zone:
            stream << "backed up, own red zone";
            break;
        case SinglePlay::middle:
            stream << "between red zones";
            break;
        case SinglePlay::opp_red_zone:
            stream << "scoring range, opponent red zone";
            break;
        default:
            stream << "UNKNOWN";
            break;
        }
    return stream;
}

ostream& operator<<(ostream& stream, SinglePlay::TimeRemaining timeRemaining)
{
    switch (timeRemaining) {
        case SinglePlay::outside_two_minutes:
            stream << "Outside two minute warning";
            break;
        case SinglePlay::inside_two_minutes:
            stream << "Inside two minute warning";
            break;
        default:
            stream << "UNKNOWN";
            break;
        }
    return stream;
}

ostream& operator<<(ostream& stream, SinglePlay::ScoreDifferential scoreDifferential)
{
    switch (scoreDifferential) {
        case SinglePlay::down_over_fourteen:
            stream << "Down over 14 points";
            break;
        case SinglePlay::down_over_seven:
            stream << "Down between 7 and 14 points";
            break;
        case SinglePlay::down_seven_less: stream << "Down 7 or less points";
            break;
        case SinglePlay::even:
            stream << "Tied";
            break;
        case SinglePlay::up_seven_less:
            stream << "Up 7 or less points";
            break;
        case SinglePlay::up_over_seven:
            stream << "Up between 7 and 14 points";
            break;
        case SinglePlay::up_over_fourteen:
            stream << "Up over 14 points";
            break;
        default:
            stream << "UNKNOWN";
            break;
        }
    return stream;
}

// Method to output a play for debugging purposes
ostream& operator<<(ostream& stream, const SinglePlay& singlePlay)
{
    stream << "RefId:" << singlePlay.getRefId() << " Play:" << singlePlay.getPlayType();

    /* This methid outputs the characteristics by value reads. It results in
        numeric values instead of category namess. This is quite deliberate, so
        the debug output has the same values used by the decision tree algorithm */
    unsigned short index;
    // NOTE: Enums can't be incremented, leading to this clunky implementation
    for (index = 0; index <= (unsigned short)SinglePlay::score_differential; index++)
        stream << " " << (SinglePlay::PlayCharacteristic)index << ":"
               << singlePlay.getValue((SinglePlay::PlayCharacteristic)index);
    stream << " Distance Gained:" << singlePlay.getDistanceGained() << " Turned Over:"
           << (short)singlePlay.getTurnedOver();
    return stream;
}
