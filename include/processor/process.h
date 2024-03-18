#ifndef PROCESS_H
#define PROCESS_H

#include "gcode.h"


#include <filesystem>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <exception>
#include <bitset>
#include <format>
    
const int x_size = 880 * 2; // we use a resoluation of 0.5 mm, hence the times two.
const int y_size = 1330 * 2; // TODO make this based on the Cura print bed output (how to get this setting??)


class PrintHead
{
public:
    /**
    *  Creates a PrintHead object according to the provided parameters
    * @param valve_spacing - Distance between the centers of two valves
    * @param nr_of_blocks - In how many blocks the valves are distrubuted
    * @param nozzles_per_bloc - As the name suggests
    * */
    PrintHead(float valve_spacing, uint16_t nr_of_blocks, uint16_t nozzles_per_block)
        : _valve_spacing(valve_spacing)
        , _nr_of_blocks(nr_of_blocks)
        , _nozzles_per_block(nozzles_per_block)
    {};

    /* returns the index and the offset of a coordinate, based on the interval */
    void get_block_indices(float x_coordinate, int& block_index, int& block_offset)
    {
        int valve_index = static_cast<int>(x_coordinate / _valve_spacing);
        block_index = static_cast<int>(valve_index / _nozzles_per_block);
        block_offset = valve_index % _nozzles_per_block;
    }

    uint16_t nr_of_nozzles()
    {
        return _nr_of_blocks * _nozzles_per_block;
    }

    float printhead_size()
    {
        return nr_of_nozzles() * _valve_spacing;
    }

private:
    const uint16_t _nr_of_blocks;
    const uint16_t _nozzles_per_block;
    const float _valve_spacing;
};

// Class holding the pattern that has to be sprayed
// can be filled with individual 'spray lines'
// and will generate our machine specific output g-code
// at the moment only supports 2 passes (there and back)
class SprayPattern
{
public:
    SprayPattern(PrintHead ph, uint16_t y_bed_size, uint16_t nr_passes = 2)
        : _ph(ph)
        , _spray_pattern_data_width(std::ceil(_ph.nr_of_nozzles() * nr_passes / 8.0))
        , pattern(y_bed_size, std::vector<std::bitset<8>>(_spray_pattern_data_width))
    {
        
    };

    void set_layer_nr(int layer_nr)
    {
        _layer_nr = layer_nr;
    }

    void add_spray_line(GCodeMove begin, GCodeMove end)
    {
        float tolerance = 1e-8;
        if (std::fabs(begin.X - end.X) > tolerance)
        {
            throw std::invalid_argument("Begin and end coordinates do not have the same X-value");
        }

        GCodeMove first = begin, second = end;
        if (begin.Y > end.Y)
        {
            first = end;
            second = begin;
        }

        set_valves(begin.X, first.Y, second.Y);
    };

    void set_valves(uint16_t x_coord, uint16_t y_begin_coord, uint16_t y_end_coord)
    {
        // first convert from coordinate to y_coord index
        int begin_index = get_y_index(y_begin_coord);
        int end_index = get_y_index(y_end_coord);

        // also get the valve that we have to turn on/off
        int block_index, block_offset;
        _ph.get_block_indices(x_coord, block_index, block_offset);

        for (int n = begin_index; n < end_index; n++)
        {
            pattern[n][block_index][block_offset] = 1;
        }
    }

    //for now just round off the y value
    uint16_t get_y_index(float y_coord)
    {
        return static_cast<int>(y_coord);
    }

    uint16_t get_y_size()
    {
        return pattern.size();
    }

    uint16_t get_spray_pattern_data_width()
    {
        return _spray_pattern_data_width;
    }

    PrintHead _ph;
    const uint16_t _spray_pattern_data_width;
    std::vector<std::vector<std::bitset<8>>> pattern;
    int _layer_nr = -1;
};

class GCodeGenerator
{
public:
    const int Y_INITIAL_POSITION = 118;
    const int Y_FEED_RATE = 8000;

    const int N_NOZZLES = 88;
    const int N_NOZZLES_PER_MANIFOLD = 8;
    const int N_PASSES = 2;
    const int PRINTABLE_AREA = (880, 1462);
    const int RESOLUTION = N_NOZZLES * N_PASSES;
    const int RESOLUTION_MM = 5;

    std::string print_begin_cmd(int NUMBER_OF_LAYERS)
    {
        return std::format(
            "SET_PRINT_STATS_INFO TOTAL_LAYER={}\n"
            "G28 X Y SET_FIRST_PASS G4 P3000 ;wait for servo\n"
            "VALVES_SET VALUES=0,0,0,0,0,0,0,0,0,0,0\n"
            "VALVES_ENABLE ; change to VALVES_DISABLE to do run without valves active\n",
            NUMBER_OF_LAYERS);
    }

    std::string print_end_cmd(int NUMBER_OF_LAYERS)
    {
        return std::format(
            "; total layers count = {}\n", NUMBER_OF_LAYERS);
    }

    std::string layer_begin_cmd(int layer_idx)
    {
        return std::format(
            ";Layer{}\n"
            "SET_PRINT_STATS_INFO CURRENT_LAYER={}\n"
            "SET_FIRST_PASS\n"
            "Z_ONE_LAYER\n"
            "FILL_HOPPER_UNTIL_FULL\n"
            "PAUSE_PRINTER ;wait for button press\n"
            "DEPOSIT_ONE_LAYER\n", layer_idx+1, layer_idx+1);
    }
    
    std::string layer_return_cmd(int y_pos)
    {
        return std::format(
            "G1 Y{}\n"
            "VALVES_SET VALUES=0,0,0,0,0,0,0,0,0,0,0\n"
            "G1 Y{}\n"
            "SET_SECOND_PASS\n"
            "G4 P3000\n", y_pos, y_pos+1);
    }
    
    std::string layer_end_cmd(int y_start_bed_pos)
    {
        return std::format(
            "G1 Y{}\n"
            "VALVES_SET VALUES=0,0,0,0,0,0,0,0,0,0,0\n"
            "G1 Y0\n",
            y_start_bed_pos - 1);
    }

    std::bitset<8> extractEverySecondBit(const std::bitset<8>& input1, const std::bitset<8>& input2)
    {
        std::bitset<8> result;

        for (size_t i = 0; i < 8; i += 2)
        {
            result.set(i / 2, input1.test(i));
            result.set(i / 2 + 4, input2.test(i));
        }

        return result;
    }

    //takes a vector of bitset<8> with an even number of elements
    //and moves all even bits to the first half of the elements,
    //and all uneven bits to the last half of the elements
    //so 1010.1010 1010.1010 would become 0000.0000 1111.1111
    //note that the bitset indexing when written out starts at the 
    //last element (i.e. 10000001)
    //                   ^      ^
    //            index [7]    [0]
    void interlace_and_separate(std::vector<std::bitset<8>>& data)
    {
        if (data.size() % 2 != 0)
        {
            throw std::invalid_argument("Input vector must contain an even number of elements");
        }

        int s = data.size() / 2;

        std::vector<std::bitset<8>> orig(data.begin(), data.end());
        int idx = 0;
        for (int n=0; n < data.size(); n++)
        {
            for (int i = 0; i < 8; i += 2) // process all bits in chunks of 2
            {
                int idx = n * 8 + i;
                int v_idx = (idx / 2) / 8;
                int b_idx = (idx / 2) % 8;
                data[v_idx][b_idx] = orig[n][i];
                data[v_idx + s][b_idx] = orig[n][i + 1];
            }
        }
    }

    template<std::size_t N>
    void reverse(std::bitset<N>& b)
    {
        for (std::size_t i = 0; i < N / 2; ++i)
        {
            bool t = b[i];
            b[i] = b[N - i - 1];
            b[N - i - 1] = t;
        }
    }

    std::string generate(SprayPattern sp, uint16_t layer_nr=0, uint16_t y_start_of_bed=0, uint16_t bed_length=1400)
    {
        //get an idea of the max size of the vector that is need
        //so we can allocate in one go
        // one entry will become max: G1 Y0000\nSET_VALVES VALUES=255,255,255,255,255,255,255,255,255,255,255\n
        int MAX_LEN_ONE_ENTRY = 71;
        int n = sp.pattern.size() * MAX_LEN_ONE_ENTRY + 1;

        std::string s;
        s.reserve(n);
        
        s += layer_begin_cmd(layer_nr);

        int y_pos = y_start_of_bed;
        for (auto& p : sp.pattern)
        {
            interlace_and_separate(p);
            if (y_pos == y_start_of_bed)
            {
                s += "G1 Y" + std::to_string(y_pos++) + " F8000\n";    //the first time we add the print velocity
            }
            else
            {
                s += "G1 Y" + std::to_string(y_pos++) + '\n';
            }
            s += "VALVES_SET VALUES=";
            for (int i=0; i<p.size()/2; i++)
            {
                reverse(p[i]);
                s += std::to_string(p[i].to_ulong()) + ',';
            }
            s.pop_back(); //remove last comma
            s += '\n';
        }

        s += layer_return_cmd(y_pos++);

        //and the return leg
        for (auto it = sp.pattern.rbegin(); it != sp.pattern.rend(); ++it)
        {
            //no need to interlace again, already done before
            s += "G1 Y" + std::to_string(--y_pos) + '\n';
            s += "VALVES_SET VALUES=";
            for (int i = (*it).size() / 2; i < (*it).size(); i++)
            {
                if (y_pos == 0) //we already returned to base, so we close the valves. This can only happen if the bed_begin_y_coord = 0
                {
                    s += "0,0,0,0,0,0,0,0,0,0,0,";
                }
                else
                {
                    reverse((*it)[i]);
                    s += std::to_string((*it)[i].to_ulong()) + ',';
                }
            }
            s.pop_back(); // remove last comma
            s += '\n';
        }

        // end of layer gcode
        s += layer_end_cmd(y_pos);

        return s;
    }
};

/* can be fed gcode, and it will populate the Spraypattern*/
class GCodeParser
{
public:
    GCodeParser(PrintHead ph, uint16_t x_bed_size, uint16_t y_bed_size)
        : pattern(ph, y_bed_size, x_bed_size / ph.printhead_size()){

        };

    void set_layer_nr(int layer_nr)
    {
        pattern.set_layer_nr(layer_nr);
    }

    void parse(std::string line){ 
        try
        {
            auto current_move = get_g_move(line);
            if (current_move.isExtrusionMove())
            {
                if (first_move_processed)
                {
                    pattern.add_spray_line(prev_move, current_move);
                }
            }
            first_move_processed = true; 
            prev_move = current_move;
            
        }
        catch (std::invalid_argument e)
        {
            //not a valid G-code command aparently
        }

    }
    bool first_move_processed = false;
    SprayPattern pattern;
    GCodeMove prev_move;
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

/* High level class that manages everything to achieve 
   a valid, working gcode. */
class PrintManager
{
public:
    /**
     *  Creates a PrintHead object according to the provided parameters
     * @param ph - A PrintHead object
     * @param y_start_pos - The Y-position of the print head where the bed starts
     * @param bed_length - The length in mm of the bed. This plus the y_start_pos 
     * should be equal less than the maximum y-position the print head can reach.
     * */
    PrintManager(PrintHead print_head, int y_start_pos, int bed_length)
        : printhead(print_head)
        , _y_start_pos(y_start_pos)
        , _bed_length(bed_length-1)   //substract one, because we need it to stop, close the valves and return 
        , gcodeparser(printhead, printhead.printhead_size() * 2, _bed_length)
    {

    }

    std::string generate(uint16_t layer_nr)
    {
        return gg.generate(gcodeparser.pattern, layer_nr, _y_start_pos, _bed_length);
    }

    void parse(std::string line)
    {
        gcodeparser.parse(line);
    }

    PrintHead printhead;
    int _y_start_pos;
    int _bed_length;
    GCodeParser gcodeparser;
    GCodeGenerator gg;

};

int asdasd(int a)
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
    GCodeMove last_cmd;
    while (std::getline(file, line))
    {
        // Check if the line starts with 'G0' or 'G1'
        if (line.substr(0, 2) == "G0" || line.substr(0, 2) == "G1")
        {
            // Parse the line and create a Command object
            GCodeMove current_cmd = get_g_move(line);

            if (current_cmd.isExtrusionMove())
            {
                //add_valve_output(last_cmd, current_cmd, V_OUT, number_of_nozzles);
            }
        }
    }
     
    // Close the file
    file.close();

    return 0;
}


#endif