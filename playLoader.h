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
#include<fstream>
#include<cstdlib>

using std::ifstream;
using std::string; // Clients will use lots of strings, so they should include the header

/* This class loads plays from .csv files into the in-memory database.
    Statistical techniques are hard to apply to football, because the
    number of variables a team faces are so large. Modern NFL teams deal
    with the issue through their Quality Control departments. Any NFL team
    can request film on any game ever played back to the early 1960s (when
    NFL Films started). This department selects games that will be the most
    useful to study and breaks down the film into individual plays.

    Selection of games to study is one of the blackest arts of football
    coaching. Most select previous games against a given opponent, teams
    similiar to their their team against that opponent, and their team
    against teams similiar to their opponent. The class builds the database
    using this approach. The caller specifies the similiar teams.

    WARNING: This class is still pretty simplistic compared to real Quality
    Control, which will lead to skewed results. Many go much futher that
    just teams, getting film containing key players or teams where a key
    choach has worked previously. They also select games from different
    team/season combinations while this routine gets all seasons for a given
    matchup */

class PlayLoader {
public:
    // Constructor. Takes the path to the directory with data files
    explicit PlayLoader(const string& filePath);

    // Destructor
    ~PlayLoader();

    // Loads plays into a data store
    void loadPlays(const string& thisTeam, const string& otherTeam,
                   const vector<string>& thisSimiliar, const vector<string>& otherSimiliar,
                   unsigned short yearRange, DataStore& dataStore);

private:
    // File to load plays from. Inside class to ensure always released
    ifstream _playFile;
    string _directory; // Where to file data files

    // Loads plays for the wanted teams for one season into the data store
    /* WARNING: Does NOT generate index data! */
    void loadSingleSeason(const string& thisTeam, const string& otherTeam,
                          const vector<string>& thisSimiliar,
                          const vector<string>& otherSimiliar,
                          unsigned short seasonYear,
                          DataStore& dataStore);

    // Process a single play from a data file
    void processPlay(const string& playString, const string& thisTeam, const string& otherTeam,
                     const vector<string>& thisSimiliar, const vector<string>& otherSimiliar,
                     unsigned short& sackCount, DataStore& dataStore);

    // Extracts numeric data from the passed position of the input string. Range is [start...end)
    short extractNumeric(const string& playString, unsigned int startPos, unsigned int endPos);

    // Finds the yardage achieved from a play, and whether the ball was fumbled
    void extractPlayYardageTurnover(const string& description, unsigned int pos,
                                    short& distanceGained, bool& turnedOver);

    // Prohibit copying, which screws up the file buffer
    PlayLoader(const PlayLoader& other);
    PlayLoader& operator=(const PlayLoader& other);
};

inline void PlayLoader::loadPlays(const string& thisTeam, const string& otherTeam,
                                  const vector<string>& thisSimiliar, const vector<string>& otherSimiliar,
                                  unsigned short yearRange, DataStore& dataStore)
{
    /* FUTURE DEVELOPMENT: Should use file system calls to find the range of years with play
        data. This routine hardcodes it */
    unsigned short firstYear = 2008;
    unsigned short lastYear = 2011;
    if (lastYear - yearRange + 1 > firstYear)
        firstYear = lastYear - yearRange + 1;
    unsigned short yearCounter;
    for (yearCounter = lastYear; yearCounter >= firstYear; yearCounter--)
        loadSingleSeason(thisTeam, otherTeam, thisSimiliar, otherSimiliar, yearCounter, dataStore);
    dataStore.buildIndexes();
}

// Extracts numeric data from the passed position of the input string. Range is [start...end)
inline short PlayLoader::extractNumeric(const string& playString, unsigned int startPos,
                                        unsigned int endPos)
{
    // This is not the cleanest way to do this, but it is the fastest
    return atoi(string(playString, startPos, endPos - startPos).c_str());
}
