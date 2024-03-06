#ifndef GCODE_H
#define GCODE_H

#include <sstream>
#include <cmath>
#include <string>
#include <stdexcept>

// Define the Command class
class GCodeMove
{
public:
    
    float X, Y, Z, E;
    GCodeMove()
        : X(0)
        , Y(0)
        , Z(0)
        , E(0)
    {
    }

    GCodeMove(float x, float y, float z, float e)
        : X(x)
        , Y(y)
        , Z(z)
        , E(e)
    {
    }

    bool isExtrusionMove()
    {
        float tolerance = 1e-8;
        if (std::fabs(E) < tolerance)
            return false;
        else
            return true;
    }

};

// Parses a gcode instruction and extracts the X, Y, Z, E values
// // Only works for G0 and G1 commands that contain at least one element
// - input:   std::string that contains the gcode instruction
// - output:  a Command object that contains the parsed values of the gcode instruction
GCodeMove get_g_move(const std::string& line)
{
    if (line.substr(0, 2) != "G0" && line.substr(0, 2) != "G1")
    {
        throw std::invalid_argument("Supplied gcode command must be starting with G0 or G1");
    }

    float x = 0.0f, y = 0.0f, z = 0.0f, e = 0.0f;
    std::istringstream iss(line);
    std::string commandType;
    iss >> commandType;
    char ch;
    bool atLeastOneParameter = false;
    while (iss >> ch)
    {
        if (ch == 'X')
        {
            iss >> x;
            atLeastOneParameter = true;
        }
        else if (ch == 'Y')
        {
            iss >> y;
            atLeastOneParameter = true;
        }
        else if (ch == 'Z')
        {
            iss >> z;
            atLeastOneParameter = true;
        }
        else if (ch == 'E')
        {
            iss >> e;
            atLeastOneParameter = true;
        }
    }
    if (! atLeastOneParameter)
    {
        throw std::invalid_argument("Supplied gcode command needs to have at least a single parameter (X, Y, Z or E)");
    }
    return GCodeMove(x, y, z, e);
};

#endif