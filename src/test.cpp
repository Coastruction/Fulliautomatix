#include "plugin/process.h"

#include <filesystem>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <exception>

namespace Processor
{


// Define the Command class
class Command
{
public:
    float X, Y, Z, E;
    Command()
        : X(0)
        , Y(0)
        , Z(0)
        , E(0)
    {
    }

    Command(float x, float y, float z, float e)
        : X(x)
        , Y(y)
        , Z(z)
        , E(e)
    {
    }

    bool hasExtrusionValue()
    {
        float tolerance = 1e-8;
        if (std::fabs(E) < tolerance)
            return true;
        else
            return false;
    }
};

#define OPEN 1
#define CLOSE 2
class ValveCommand
{
public:
    float X, Y;
    int TYPE;
    ValveCommand(float x, float y, int type)
        : X(x)
        , Y(y)
        , TYPE(type)
    {
    }
};

// Function to parse a line and extract X, Y, Z, E values
Command parseLine(const std::string& line)
{
    float x = 0.0f, y = 0.0f, z = 0.0f, e = 0.0f;
    std::istringstream iss(line);
    std::string commandType;
    iss >> commandType;
    char ch;
    while (iss >> ch)
    {
        if (ch == 'X')
        {
            iss >> x;
        }
        else if (ch == 'Y')
        {
            iss >> y;
        }
        else if (ch == 'Z')
        {
            iss >> z;
        }
        else if (ch == 'E')
        {
            iss >> e;
        }
    }
    return Command(x, y, z, e);
}

/* calculates the */
int32_t calculate_slice(int begin, int end, int lower_bound, int upper_bound)
{
    if (upper_bound - lower_bound > 32)
    {
        throw std::invalid_argument("Selected interval exceeds what fits in a 32-bit integer.");
    }
    if (begin > end)
    {
        throw std::invalid_argument("Begin index is larger then end index");
    }

    if (begin <= lower_bound)
    {
        begin = lower_bound;
    }

    if (end >= upper_bound)
    {
        end = upper_bound - 1;
    }

    int a = begin - lower_bound;
    int b = end - lower_bound;

    return pow(2.0, b) - pow(2.0, a - 1);
}

/* returns the index and the offset of a coordinate, based on the interval */
void get_block_indices(float x_coordinate, float interval, unsigned int block_size, int& block_index, int& block_offset)
{
    int valve_index = static_cast<int>(x_coordinate / interval);

    block_index = static_cast<int>(valve_index / block_size);
    block_offset = valve_index % block_index;
}

void add_valve_output(Command begin, Command end, std::vector<std::vector<uint8_t>>& output_array, int number_of_nozzles)
{
    float tolerance = 1e-8;
    if (std::fabs(begin.X - end.X) > tolerance)
    {
        throw std::invalid_argument("Begin and end coordinates do not have the same X-value");
    }

    Command first = begin, second = end;
    if (begin.Y > end.Y)
    {
        first = end;
        second = begin;
    };

    // first convert from coordinate to valve index
    int begin_index = static_cast<int>(first.Y / 0.5);
    int end_index = static_cast<int>(second.Y / 0.5);

    // split them up according to each column of our output_array
    for (int n = 0; n < output_array[0].size(); n++)
    {
        output_array[0][n] = calculate_slice(begin_index, end_index, n * 32, (n + 1) * 32);
    }
}

const int x_size = 880 * 2; // we use a resoluation of 0.5 mm, hence the times two.
const int y_size = 1330 * 2; // TODO make this based on the Cura print bed output (how to get this setting??)

int main(int argc, const char** argv)
{
    std::filesystem::path currentPath = std::filesystem::current_path();

    std::string dataPath = currentPath.string() + "\\..\\..\\dat\\";
    std::string filename = "commands.txt";

    /* The resulting G-code is getnerated in several steps:
     1. We construct a matrixs representing the Y direction
        cells of 0.5mm long, and in the X direction cells of 5mm wide.
        The columns, containing 11 8-bit values, represent the bit values
        for each of the 88 valves. The rows represent the y-coordinate (step size 0.5mm)
        As the print is interlaced, the matrix has twice the number of columns (22).
     2. as Cura doesn't output the print lines exactly at 5 mm intervals,
        we first calculate the offset, to be applied to all feature X coordinates
     3. we step through the original G-code, looking for extrusion moves. Every extrusion
        move is then mapped into our output matrix based on the start and end-position of
        the extrusion move.
     4. the output matrix is converted to G-code that our printer understands.
     */


    const int number_of_nozzles = 88;
    const int n_vals = 22; // 11 first the 88 valves on the first pass, and 11 for the return pass

    std::vector<std::vector<uint8_t>> V_OUT(y_size, std::vector<uint8_t>(n_vals));

    // Convert the path to a string and print it
    std::cout << "Data directory: " << dataPath << std::endl;

    // Open the file
    std::ifstream file(dataPath + filename);
    if (! file.is_open())
    {
        std::cerr << "Error opening file!" << std::endl;
        return 1;
    }

    // Vector to store Command objects

    // Read file line by line
    std::string line;
    bool is_extruding = false;
    Command last_cmd;
    while (std::getline(file, line))
    {
        // Check if the line starts with 'G0' or 'G1'
        if (line.substr(0, 2) == "G0" || line.substr(0, 2) == "G1")
        {
            // Parse the line and create a Command object
            Command current_cmd = parseLine(line);

            if (current_cmd.hasExtrusionValue())
            {
                add_valve_output(last_cmd, current_cmd, V_OUT, number_of_nozzles);
            }
        }
    }

    // Close the file
    file.close();


    return 0;
}

} // namespace Processor