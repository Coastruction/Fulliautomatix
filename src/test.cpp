#include <gtest/gtest.h>

#include "processor/process.h"


// Define your test cases here
TEST(TestCaseName, get_block_indices)
{
    // Test assertions go here
    EXPECT_EQ(1 + 1, 2);
    EXPECT_TRUE(true);
}


TEST(basic_values, get_g_move)
{
    std::string input_str = "G1 X0 Y700 Z23 E65.23";
    auto res = get_g_move(input_str);
    // Test assertions go here
    EXPECT_EQ(res.X, 0);
    EXPECT_EQ(res.Y, 700);
    EXPECT_EQ(res.Z, 23);
    EXPECT_FLOAT_EQ(res.E, 65.23);
    EXPECT_TRUE(res.isExtrusionMove());

    input_str = "G1 X0 Y700 Z24";
    res = get_g_move(input_str);
    EXPECT_FALSE(res.isExtrusionMove());

    input_str = "G1 X0 Y700 Z24 E0.0";
    res = get_g_move(input_str);
    EXPECT_FALSE(res.isExtrusionMove());

    input_str = "G1 X0 Y700 Z24 E-123.1";
    res = get_g_move(input_str);
    EXPECT_TRUE(res.isExtrusionMove());
}

TEST(constructor, printhead)
{
    auto ph = PrintHead(5, 11, 8);
    EXPECT_EQ(ph.nr_of_nozzles(), 88);
}

TEST(block_indeces, printhead)
{
    auto ph = PrintHead(5, 11, 8);
    int block_index, block_offset;
    ph.get_block_indices(0, block_index, block_offset);
    EXPECT_EQ(block_index, 0);
    EXPECT_EQ(block_offset, 0);
    ph.get_block_indices(30, block_index, block_offset);
    EXPECT_EQ(block_index, 0);
    EXPECT_EQ(block_offset, 6);
    ph.get_block_indices(860, block_index, block_offset);
    EXPECT_EQ(block_index, 21);
    EXPECT_EQ(block_offset, 4);
}

TEST(construction, spraypattern)
{
    auto sp = SprayPattern(PrintHead(5.0, 11, 8), 109);
    EXPECT_EQ(sp.get_spray_pattern_data_width(), 22);
    EXPECT_EQ(sp.get_y_size(), 109);
}

TEST(adding_spray_lines, spraypattern)
{
    auto sp = SprayPattern(PrintHead(5, 11, 8), 109);
    GCodeMove begin(30, 30, 0, 0.1);
    GCodeMove end(30, 40, 0, 0.1);
    sp.add_spray_line(begin, end);
    EXPECT_TRUE(sp.pattern[30][0].test(6));
    EXPECT_TRUE(sp.pattern[31][0].test(6));
    EXPECT_FALSE(sp.pattern[29][0].test(6));
    EXPECT_FALSE(sp.pattern[31][0].test(5));
    EXPECT_FALSE(sp.pattern[31][0].test(7));
    EXPECT_FALSE(sp.pattern[40][0].test(6));
}


TEST(basic_exception, get_g_move)
{
    std::string input_str = "G1";
    EXPECT_THROW(get_g_move(input_str), std::invalid_argument);

    input_str = "G0 G12";
    EXPECT_THROW(get_g_move(input_str), std::invalid_argument);

    input_str = "G6 X5 Y120 Z9";
    EXPECT_THROW(get_g_move(input_str), std::invalid_argument);
}

TEST(basic_g_code_moves, gcodeparser)
{
    PrintHead ph(5.0, 11, 8);
    GCodeParser gp(ph, ph.printhead_size()*2, 15);
    gp.parse(";Layer 1");
    gp.parse("M18");
    gp.parse("G0 X0 Y0 E0");
    gp.parse("G1 X0 Y5 E12.1231");
    gp.parse("G1 X15 Y10");
    gp.parse("G1 X15 Y0 E123.12");
    gp.parse(";Layer 1");

    EXPECT_TRUE(gp.pattern.pattern[0][0].test(0));
    EXPECT_TRUE(gp.pattern.pattern[3][0].test(0));
    EXPECT_FALSE(gp.pattern.pattern[10][0].test(0));
    EXPECT_FALSE(gp.pattern.pattern[3][0].test(1));
    EXPECT_TRUE(gp.pattern.pattern[3][0].test(3));
    EXPECT_TRUE(gp.pattern.pattern[5][0].test(3));

    GCodeGenerator gg;
    auto out = gg.generate(gp.pattern);

    //std::cout << out;
}


TEST(bitset, gcodegenerator)
{ 
    GCodeGenerator gg;
    std::bitset<8> bits1("11101110");
    std::bitset<8> bits2("01101101");

    std::bitset<8> result = gg.extractEverySecondBit(bits1, bits2);

    EXPECT_EQ(result, std::bitset<8>("10111010"));
    EXPECT_EQ(result.to_ulong(), 186);

    std::vector<std::bitset<8>> test_data(2);
    std::vector<std::bitset<8>> result2(2);
    test_data[0] = std::bitset<8>("11010001");
    test_data[1] = std::bitset<8>("00100111");
    result2[0] = std::bitset<8>("00111101");
    result2[1] = std::bitset<8>("01011000");
    gg.interlace_and_separate(test_data);
    EXPECT_EQ(result2, test_data);

    std::vector<std::bitset<8>> test_data3(4);
    std::vector<std::bitset<8>> result3(4);
    test_data3[0] = std::bitset<8>("11010001");
    test_data3[1] = std::bitset<8>("00100111");
    test_data3[2] = std::bitset<8>("11011001");
    test_data3[3] = std::bitset<8>("11111001");
    result3[0] = std::bitset<8>("00111101");
    result3[1] = std::bitset<8>("11011101");
    result3[2] = std::bitset<8>("01011000");
    result3[3] = std::bitset<8>("11101010");
    gg.interlace_and_separate(test_data3);
    EXPECT_EQ(result3, test_data3);
}




TEST(conversion, gcodegenerator)
{
    PrintHead ph(5, 11, 8);
    GCodeParser gp(ph, ph.printhead_size() * 2, 10);
    gp.parse("G0 X0 Y0 E0");
    gp.parse("G1 X0 Y10 E12.1231");
    gp.parse("G1 X10 Y10");
    gp.parse("G1 X10 Y0 E123.12");
    
    GCodeGenerator gg;
    auto res = gg.generate(gp.pattern);
    EXPECT_TRUE(true);
}

int main(int argc, char** argv)
{
    // Initialize the Google Test framework
    ::testing::InitGoogleTest(&argc, argv);
    // Run all tests
    return RUN_ALL_TESTS();

}
