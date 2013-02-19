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
#include<string>
#include<vector>
#include<fstream>
#include<iostream>
#include<sstream>
#include<algorithm>

#include"singlePlay.h" // Needed by playIndexSet.h
#include"playIndexSet.h" // Needed by dataStore.h
#include"playStats.h" // Needed by dataStore.h
#include"dataStore.h"
#include"playLoader.h"
#include"baseException.h"

using std::string;
using std::ifstream;
using std::cerr;
using std::endl;
using std::stringstream;
using std::find;

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
PlayLoader::PlayLoader(const string& directory)
    : _playFile(), _directory(directory)
{
    // All in the initialization list
}

// Destructor
PlayLoader::~PlayLoader()
{
    // Ensure the file buffer is closed and released
    if (_playFile.is_open())
        _playFile.close();
}

// Loads plays for the wanted teams for one season into the data store
/* WARNING: Does NOT generate index data! */
void PlayLoader::loadSingleSeason(const string& thisTeam, const string& otherTeam,
                                  const vector<string>& thisSimiliar,
                                  const vector<string>& otherSimiliar,
                                  unsigned short seasonYear,
                                  DataStore& dataStore)
{
    // First, open the file
    if (_playFile.is_open())
        _playFile.close();

    // Assemble the file name. Format is XXXX_nfl_pbp_data.csv, where XXXX is the year
    /* The passed directory does not include the backslash needed before the filename,
        so add it
        TRICKY NOTE: Notice the double backslash below. C++ uses '\' as an
        escape character. The first is the esacpe character needed to insert a
        litteral '\' in the string! */
    stringstream fullFileName;
    fullFileName << _directory << "\\";
    fullFileName << seasonYear << "_nfl_pbp_data.csv";
    // Trace the full file path, to catch the error where the directory is wrong
    _playFile.open(fullFileName.str().c_str());
    if (!_playFile.is_open()) {
        stringstream errorMessage;
        errorMessage << "Error, could not open data file " << fullFileName.str();
        throw BaseException(__FILE__, __LINE__, errorMessage.str().c_str());
    }

    /* Some texts recommend always putting file reads in a try...catch block, to clean up
        properly. This code doesn't bother because the destructor will handle the file,
        and the lack of an index creation call on the data store ensures consistent (and empty)
        results on an exception */
    string playText;
    // First line is a header. Read it to burn it
    getline(_playFile, playText);
    unsigned short sackCount = 0; // Number of sacks processed
    while (!_playFile.eof()) {
        // Read a play from the data file and process it
        getline(_playFile, playText);
        processPlay(playText, thisTeam, otherTeam, thisSimiliar, otherSimiliar, sackCount, dataStore);
    }
    _playFile.close();
}

// Process a single play from a data file
void PlayLoader::processPlay(const string& playString, const string& thisTeam, const string& otherTeam,
                             const vector<string>& thisSimiliar, const vector<string>& otherSimiliar,
                             unsigned short& sackCount, DataStore& dataStore)
{
    /* Play data is organized in the following fields:
        gameid,qtr,min,sec,off,def,down,togo,ydline,description,offscore,defscore,season
        They are extracted by searching for the commas */
    unsigned int pos;
    unsigned int prevPos;
    // First category is a game ID, burn it
    pos = playString.find_first_of(',');
    // Second category is quarter, burn it
    pos = playString.find_first_of(',', pos + 1);

    // Third cagetory is minues. Extract it
    prevPos = pos + 1; // Move off  the comma
    pos = playString.find_first_of(',', prevPos);
    if (pos == string::npos) { // Indicates badly formed input
        cerr << "Improperly formatted input: " << playString << endl;
        return;
    }
    unsigned short minutes = (unsigned short)extractNumeric(playString, prevPos, pos);

    // Fourth category is seconds. Burn it
    pos = playString.find_first_of(',', pos + 1);

    // Fifth category is offence, extract it
    prevPos = pos + 1; // Move off  the comma
    pos = playString.find_first_of(',', prevPos);
    if (pos == string::npos) { // Indicates badly formed input
        cerr << "Improperly formatted input: " << playString << endl;
        return;
    }
    string offense(playString, prevPos, pos - prevPos);

    // Sixth category is defence, extract it
    prevPos = pos + 1; // Move off  the comma
    pos = playString.find_first_of(',', prevPos);
    if (pos == string::npos) { // Indicates badly formed input
        cerr << "Improperly formatted input: " << playString << endl;
        return;
    }
    string defense(playString, prevPos, pos - prevPos);

    // Seventh category is down, extract it
    prevPos = pos + 1; // Move off  the comma
    pos = playString.find_first_of(',', prevPos);
    if (pos == string::npos) { // Indicates badly formed input
        cerr << "Improperly formatted input: " << playString << endl;
        return;
    }
    /* If the down is missing, this play is a non-down play like kickoffs
        and extra point attempts. The database deliberately ignores these */
    if (pos == prevPos)
        return;
    unsigned short down = (unsigned short)extractNumeric(playString, prevPos, pos);

    /* At this point, have enough information to determine whether this play is wanted
        or not. It must fall within one of three categories
        1. Offense matches wanted offense and defense matches wanted offense
        2. Offense matches wanted offense and defense falls in list of similiar
            defenses to wanted defense
        3. Offense falls in list similiar offenses to wanted offense and defense
            matches wanted defense
        WARNING: This process is sensitive to both whitespace and capitalization, because
        its more efficient for the caller to get this right that for this code to deal with
        matching it. The current set of data files requires ALL CAPS and no whitespace */
    bool haveMatch = false;
    if (offense == thisTeam) {
        if (defense == otherTeam)
            haveMatch = true;
        else
            haveMatch = (find(otherSimiliar.begin(), otherSimiliar.end(), defense) != otherSimiliar.end());
    } // Offense matches the wanted team
    else if (defense == otherTeam)
        haveMatch = (find(thisSimiliar.begin(), thisSimiliar.end(), offense) != thisSimiliar.end());
    else
        haveMatch = false;
    if (!haveMatch)
        return; // Not a wanted play

    /* If get to here, want the play. Extract remaining data snd insert into the data store.
        Some of it requires a tricky search of the description field */

    // Eighth category is distance needed, extract it.
    /* NOTE: If this is a non-down play, the distance won't be set either. They
        are rejected above, so missing the distance here is an error */
    prevPos = pos + 1; // Move off  the comma
    pos = playString.find_first_of(',', prevPos);
    if ((pos == string::npos) || (pos == prevPos)) {
        // Indicates badly formed input
        cerr << "Improperly formatted input: " << playString << endl;
        return;
    }
    unsigned short distanceNeeded = (unsigned short)extractNumeric(playString, prevPos, pos);

    // Ninth category is position on the field, extract it.
    prevPos = pos + 1; // Move off  the comma
    pos = playString.find_first_of(',', prevPos);
    if (pos == string::npos) { // Indicates badly formed input
        cerr << "Improperly formatted input: " << playString << endl;
        return;
    }
    unsigned short yardLine = (unsigned short)extractNumeric(playString, prevPos, pos);

    // Tenth category is play description. This needs further processing. Extract it here
    prevPos = pos + 1; // Move off  the comma
    pos = playString.find_first_of(',', prevPos);
    if (pos == string::npos) { // Indicates badly formed input
        cerr << "Improperly formatted input: " << playString << endl;
        return;
    }
    string description(playString, prevPos, pos - prevPos);

     // Eleventh category is offence current score, extract it.
    prevPos = pos + 1; // Move off  the comma
    pos = playString.find_first_of(',', prevPos);
    if (pos == string::npos) { // Indicates badly formed input
        cerr << "Improperly formatted input: " << playString << endl;
        return;
    }
    unsigned short ownScore = (unsigned short)extractNumeric(playString, prevPos, pos);

    // Twelveth category is defense current score, extract it.
    prevPos = pos + 1; // Move off  the comma
    pos = playString.find_first_of(',', prevPos);
    if (pos == string::npos) { // Indicates badly formed input
        cerr << "Improperly formatted input: " << playString << endl;
        return;
    }
    unsigned short oppScore = (unsigned short)extractNumeric(playString, prevPos, pos);

    /* To get play type, yardage gained, and turnover, need to parse the description.
        Thankfully, it has a standard format */
    SinglePlay::PlayType playType = SinglePlay::punt;
    short distanceGained = 0;
    bool turnedOver = false;
    unsigned int wordLoc;
    bool havePlay = false;

    // ' pass ' or ' passed ' indicates a pass play
    wordLoc = description.find(string(" pass "));
    if (wordLoc == string::npos)
        wordLoc = description.find(string(" passed "));

    if (wordLoc != string::npos) {
        wordLoc += 5; // Move to next word
        if (description[wordLoc] != ' ')
            wordLoc += 2; // Are sitting on 'ed '
        wordLoc++; // Move off the space

        // If next word is 'incomplete ', no yards and no turnover
        // SUBTLE NOTE: compare() returns 0 if they match
        bool haveIncomplete = false;
        if (description.compare(wordLoc, 11, string("incomplete ")) == 0) {
            haveIncomplete = true;
            wordLoc += 11;
        } // Incomplete pass
        // Next word should be either 'short ' or 'deep '.
        bool haveDeep = (description.compare(wordLoc, 5, string("deep ")) == 0);
        if (haveDeep)
        // Move off the word
            wordLoc += 5;
        else {
            /* Unfortunately, not all pass descriptions include the distance. These are
                treated as short. The effect here is that 'short' should only be skipped
                if it is there */
            if (description.compare(wordLoc, 6, string("short ")) == 0)
                wordLoc+=6;
        }
        // Next word will be 'left', 'right', or 'middle '
        if (description.compare(wordLoc, 5, string("left ")) == 0) {
            if (haveDeep)
                playType = SinglePlay::pass_deep_left;
            else
                playType = SinglePlay::pass_short_left;
        } // Left pass
        else if (description.compare(wordLoc, 6, string("right ")) == 0) {
            if (haveDeep)
                playType = SinglePlay::pass_deep_right;
            else
                playType = SinglePlay::pass_short_right;
        } // Left pass
        else {
            /* Unfortunately, not all pass descriptions include the direction either.
                They are treated as passes to the middle, so they will end up here
                whether the word 'middle' exists or not */
            if (haveDeep)
                playType = SinglePlay::pass_deep_middle;
            else
                playType = SinglePlay::pass_short_middle;
        } // Right pass
        if (haveIncomplete) {
            distanceGained = 0;
            turnedOver = false;
        }
        else {
            /* If the description has 'INTERCEPTION', the play is listed as an interception for
                no yards */
            if (description.find(string("INTERCEPT"), wordLoc) != string::npos) {
                distanceGained = 0;
                turnedOver = true;
            }
            else
                extractPlayYardageTurnover(description, wordLoc, distanceGained, turnedOver);
        } // Not an incomplete pass
        havePlay = true;
    } // Pass play

    /* Running plays are listed by specifying a player and a direction. Thankfully, the directions
        are unique to running plays! Need to find each one individually */
    if (!havePlay) {
        wordLoc = description.find(string(" left end "));
        if (wordLoc == string::npos)
            wordLoc = description.find(string(" left guard "));
        if (wordLoc == string::npos)
            wordLoc = description.find(string(" left tackle "));
        if (wordLoc != string::npos) {
            playType = SinglePlay::run_left;
            extractPlayYardageTurnover(description, wordLoc, distanceGained, turnedOver);
            havePlay = true;
        } // Run left
    } // No play yet

    if (!havePlay) {
        wordLoc = description.find(string(" right end "));
        if (wordLoc == string::npos)
            wordLoc = description.find(string(" right guard "));
        if (wordLoc == string::npos)
            wordLoc = description.find(string(" right tackle "));
        if (wordLoc != string::npos) {
            playType = SinglePlay::run_right;
            extractPlayYardageTurnover(description, wordLoc, distanceGained, turnedOver);
            havePlay = true;
        } // Run left
    } // No play yet

    if (!havePlay) {
        /* Some rush plays have ' rushed ' with no direction. A few more have 'scrambled'
            for quarterback scrambles. Treat these as up the middle */
        wordLoc = description.find(string(" up the middle "));
        if (wordLoc == string::npos)
            wordLoc = description.find(string(" rushed "));
        if (wordLoc == string::npos)
            wordLoc = description.find(string(" scrambles "));
        if (wordLoc != string::npos) {
            playType = SinglePlay::run_middle;
            extractPlayYardageTurnover(description, wordLoc, distanceGained, turnedOver);
            havePlay = true;
        } // Run left
    } // No play yet

    if (!havePlay) {
        /* Quarter back sacks appear as ' sacked '. In practice they are almost always
            busted pass plays. The type of pass play is unknown, so they are evenly divided
            between the pass play types. Note that a sack has yardage and can fumble as well */
        wordLoc = description.find(string(" sacked "));
        if (wordLoc != string::npos) {
            wordLoc += 8; // Move to next word
            switch (sackCount % 6) {
            case 0:
                playType = SinglePlay::pass_short_left;
                break;
            case 1:
                playType = SinglePlay::pass_short_middle;
                break;
            case 2:
                playType = SinglePlay::pass_short_right;
                break;
            case 3:
                playType = SinglePlay::pass_deep_left;
                break;
            case 4:
                playType = SinglePlay::pass_deep_middle;
                break;
            case 5:
                playType = SinglePlay::pass_deep_right;
                break;
            } // Switch on number of processed sacks
            sackCount++;
            extractPlayYardageTurnover(description, wordLoc, distanceGained, turnedOver);
            havePlay = true;
        } // Quarterback sack
    } // Play not found so far

    if (!havePlay) {
        // ' punts ' or ' punted ' indicates a successful punt, followed by the yardage
        wordLoc = description.find(string(" punts "));
        if (wordLoc == string::npos)
            wordLoc = description.find(string(" punted "));
        if (wordLoc != string::npos) {
            playType = SinglePlay::punt;
            wordLoc += 6; // Skip word
            if (description[wordLoc] != ' ')
                wordLoc++; // On 'd '
            wordLoc++; // Move off space
            unsigned int nextWordLoc = description.find(' ', wordLoc);
            distanceGained = extractNumeric(description, wordLoc, nextWordLoc);
            turnedOver = false; // Successful punts are not considered turnovers
            havePlay = true;
        } // Punt
    } // Play not found so far

    if (!havePlay) {
        // ' field goal ' indicates a field goal attempt.
        wordLoc = description.find(string(" field goal "));
        if (wordLoc != string::npos) {
            /* If the next words are 'is GOOD', it succeeded. The yardage is found BEFORE
                the word 'yard' before the signal words */
            playType = SinglePlay::field_goal;
            if (description.compare(wordLoc + 12, 7, string("is GOOD")) == 0) {
                wordLoc = description.rfind(' ', wordLoc - 1); // Skip over 'yard'
                unsigned int prevWordLoc = description.rfind(' ', wordLoc - 1);
                distanceGained = extractNumeric(description, prevWordLoc + 1, wordLoc);
            }
            else
                distanceGained = 0;
            /* Field goal attempts that result in turnovers appear differently, so none
                of these resulted in a turnover (turnover on downs doesn't count) */
            turnedOver = false;
            havePlay = true;
        } // Field goal attempt
    } // No play so far

    if (!havePlay) {
        /* The phrase 'FUMBLES (Aborted)' means the quaterback fumbled the ball without
            being sacked. In practice they are almost always busted pass plays.
            The type of pass play is unknown, so they are evenly divided between the pass
            play types. */
        wordLoc = description.find(string(" FUMBLES (Aborted) "));
        if (wordLoc != string::npos) {
            switch (sackCount % 6) {
            case 0:
                playType = SinglePlay::pass_short_left;
                break;
            case 1:
                playType = SinglePlay::pass_short_middle;
                break;
            case 2:
                playType = SinglePlay::pass_short_right;
                break;
            case 3:
                playType = SinglePlay::pass_deep_left;
                break;
            case 4:
                playType = SinglePlay::pass_deep_middle;
                break;
            case 5:
                playType = SinglePlay::pass_deep_right;
                break;
            } // Switch on number of processed sacks
            sackCount++;
            distanceGained = 0;
            turnedOver = true;
            havePlay = true;
        } // Quarterback sack
    } // Play not found so far

    if (!havePlay) {
        /* The phrase ' Aborted. ' indicates a few types of busted plays. All three involve
            turning the ball over */
        if (description.find(string(" Aborted. ")) != string::npos) {
            if (description.find(string("Punt")) != string::npos)
                playType = SinglePlay::punt;
            else if (description.find(string("Field Goal")) != string::npos)
                playType = SinglePlay::field_goal;
            else
                // Bad handoff on a running play
                playType = SinglePlay::run_middle;
            distanceGained = 0;
            turnedOver = true;
            havePlay = true;
        } // Busted play
    } // Play not found so far

    if (!havePlay) {
        // ' punt is BLOCKED ' indicates an unsuccessful punt. This is treated as a turnover with no yardage
        wordLoc = description.find(string(" punt is BLOCKED ")); // Note carefully the spaces on either end
        if (wordLoc != string::npos) {
            playType = SinglePlay::punt;
            distanceGained = 0;
            turnedOver = true; // Unsuccessful punts are treated as turnovers
            havePlay = true;
        } // Punt
    } // Play not found so far

    /* If get to here without a play, have yet another possibility. Some run plays do not have a
        direction specified, so they are listed [name] to [team] [location] for [yards]. Assume anything
        with this pattern is a running play, unless it contains 'kneels', indicating a kneel down */
    if (!havePlay) {
        if (description.find(string(" kneels ")) == string::npos) {
            wordLoc = description.find(string(" to "));
            wordLoc += 4; // Skip 'to'
            wordLoc = description.find(' ', wordLoc + 1); // Skip team name in location
            if (wordLoc != string::npos)
                wordLoc = description.find(' ', wordLoc + 1); // Skip yardage in location
            if (wordLoc != string::npos)
                if (description.compare(wordLoc, 5, string(" for ")) == 0) {
                    playType = SinglePlay::run_middle;
                    extractPlayYardageTurnover(description, wordLoc, distanceGained, turnedOver);
                    havePlay = true;
                } // Word pattern indicates a running play
        } // Does NOT indicate a kneel down
    } // Haven't found a play yet

    /* If get to here, some running plays that fail to gain yardage are listed as ' lost ' by
        itself followed by yardage */
    if (!havePlay) {
        wordLoc = description.find(string(" lost "));
        if (wordLoc != string::npos) {
            playType = SinglePlay::run_middle;
            wordLoc += 6;
            unsigned int nextWordLoc = description.find(' ', wordLoc);
            distanceGained = extractNumeric(description, wordLoc, nextWordLoc);
            distanceGained *= -1; // Play had negative yardage
            turnedOver = false;
            havePlay = true;
        } // Have negative running play
    } // No play yet

    // If found a play at this point, insert it in the data store
    if (havePlay)
        dataStore.insertPlay(playType, down, distanceNeeded, yardLine, minutes,
                             ownScore, oppScore, distanceGained, turnedOver);
    else {
        /* Certain things indicate non-plays. Check for them. If the descrition
            does not match any of them, output it as an unknown play type
            1. Penalties after a play get their own play line
            2. Kneel downs at the end of the game are ignored; their use is obvious
            3. Spikes to stop the clock are ignored; their use is  pretty obvious
            4. Video reviews get their own line
            5. Some kickoffs mistakenly have a down listed
        */

        if ((description.find(string("PENALTY")) == string::npos) &&
            (description.find(string("penalized")) == string::npos) &&
            (description.find(string("kneels")) == string::npos) &&
            (description.find(string("spiked")) == string::npos) &&
            (description.find(string("kicked")) == string::npos) &&
            (description.find(string(" play under review ")) == string::npos))
            cerr << "UNKNOWN PLAY TYPE: " << playString << endl;
    } // Not a known play type
}

// Finds the yardage achieved from a play, and whether the ball was fumbled
void PlayLoader::extractPlayYardageTurnover(const string& description, unsigned int pos,
                                            short& distanceGained, bool& turnedOver)
{
    // Yards gained always appears as ' for XXX yards'. Search for the ' for ' to find it
    unsigned int wordLoc = description.find(string(" for "), pos);
    wordLoc += 5; // Skip over string
    unsigned int nextWordLoc = description.find(' ', wordLoc);
    // Plays of zero yards are listed as 'no gain'. Need to check for 'no '
    if (description.compare(wordLoc, nextWordLoc - wordLoc, string("no ")) == 0)
        distanceGained = 0;
    else {
        /* Just to confuse things, some descriptions use 'a loss of' to indicate
            negative yardage */
        bool flipSign = false;
        if (description.compare(wordLoc, nextWordLoc - wordLoc, string("a ")) == 0) {
            flipSign = true;
            wordLoc += 10;
            nextWordLoc = description.find(' ', wordLoc);
        } // Loss flagged in words
        distanceGained = extractNumeric(description, wordLoc, nextWordLoc);
        if (flipSign)
            distanceGained *= -1;
    }
    // If the description contains 'FUMBLE', the ball carrier turned it over
    turnedOver = (description.find(string("FUMBLE"), nextWordLoc) != string::npos);
}

