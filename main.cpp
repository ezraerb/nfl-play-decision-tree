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

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include"baseException.h"
#include"singlePlay.h"
#include"playIndexSet.h"
#include"playStats.h"
#include"dataStore.h"
#include"playLoader.h"
#include"decisionNode.h"

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::ofstream;

int main(int argc, char **argv)
{
    ofstream resultFile;
    try {
        PlayLoader loader("..\\Data");
        DataStore data;

        /* Extract the teams from the input. Order is given below */
        bool validInput = false;
        bool usSimiliar = false;
        if (argc == 3)
            validInput = true;
        else if (argc >= 5) {
            /* Third argument must be either -u for teams similiar to us or
                -o for teams similiar to opponent */
            string flag(argv[3]); // Argv[0] contains the program name
            if (flag == string("-u")) {
                validInput = true;
                usSimiliar = true;
            }
            else if (flag == string("-o"))
                validInput = true;
        } // Four or more arguments

        if (!validInput) {
            cout << "Invalid arguments. US OPPONENT [-u] [SIMILIAR US TEAMS] [-o] [SIMILIAR OTHER TEAMS]" << endl;
            exit(1);
        } // Invalid input

        string thisTeam(argv[1]);
        string otherTeam(argv[2]);
        vector <string> thisSimiliar;
        vector <string> otherSimiliar;

        if (argc >= 5) {
            int argIndex;
            for (argIndex = 4; argIndex < argc; argIndex++) {
                string newTeam(argv[argIndex]);
                if (usSimiliar) {
                    if (newTeam == string("-o"))
                        usSimiliar = false;
                    else
                        thisSimiliar.push_back(newTeam);
                } // Teams similiar to us
                else {
                    if (newTeam == string("-u"))
                        usSimiliar = true;
                    else
                        otherSimiliar.push_back(newTeam);
                } // Teams similiar to opponent
            } // Loop through arguments
        } // More than two teams specified

        loader.loadPlays(thisTeam, otherTeam, thisSimiliar, otherSimiliar, 3, data);
        PlayIndexSet dataView(data.getIndexes());
        DecisionNode tree(dataView, data.getPlaySummaryStats());
        tree.pruneTree();

        // Output the final decision tree
        resultFile.open("result.txt");
        if (resultFile.is_open()) {
            // Generate header
            resultFile << "Us:" << thisTeam << " Opponent: " << otherTeam << " ";
            if (!thisSimiliar.empty()) {
                resultFile << "Similiar to Us:";
                vector<string>::iterator index;
                for (index = thisSimiliar.begin(); index != thisSimiliar.end(); index++)
                    resultFile << *index << " ";
            }
            if (!otherSimiliar.empty()) {
                resultFile << "Similiar to Other:";
                vector<string>::iterator index;
                for (index = otherSimiliar.begin(); index != otherSimiliar.end(); index++)
                    resultFile << *index << " ";
            }
            resultFile << endl;

            resultFile << tree << endl;
            resultFile.close();
        }
    } // Try block
    catch (exception& e) { // Catch by reference so virtual methods work properly
        cout << "Exception: " << e.what() << " thrown" << endl;
        if (resultFile.is_open())
            resultFile.close();
        exit(1);
    } // Catch block
}
