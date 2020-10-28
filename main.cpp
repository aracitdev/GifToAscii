#include <iostream>
#include <vector>
#include <Magick++.h>
#include <MagickCore/distort.h>
#include <SFML/Graphics.hpp>
#include <fstream>
#include <AsciiCmd/Animation.h>

std::string ASCIITable=" .:-=+*#%@";
//" .-*:o+8&#@";
//" .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";

bool ConvertToAscii(const std::string& inputFile, sf::Vector2f outScale, std::vector<std::vector<std::vector<std::pair<uint8_t, sf::Color>>>>& outPixels, std::vector<uint32_t>& delays, bool useColor, sf::Color fillColor, float speed, bool inverse)
{
    std::vector<Magick::Image> imageList;
    Magick::readImages( &imageList, inputFile );  //read frames and coalesce them
    if(!imageList.size())
    {
        std::cout <<"Failed to read image file " <<inputFile<<"\n";
        return false;
    }
    Magick::coalesceImages( &imageList, imageList.begin(), imageList.end());
    outPixels.resize(imageList.size());
    delays.resize(imageList.size());
    for(uint32_t counter=0; counter < imageList.size(); counter++)
    {
        sf::Vector2u outsize = sf::Vector2u(imageList[counter].columns() * outScale.x, imageList[counter].rows() * outScale.y);
        delays[counter]=imageList[counter].animationDelay()/speed;
        outPixels[counter].resize(outsize.x);
        for(uint32_t counterx=0; counterx < outsize.x; counterx++)
            outPixels[counter][counterx].resize(outsize.y);

        uint32_t w = imageList[counter].columns();
        uint32_t h= imageList[counter].rows();
        MagickCore::Quantum* pixels = imageList[counter].getPixels(0, 0, w, h);
        for(uint32_t counterx=0; counterx < outsize.x; counterx++)
            for(uint32_t countery=0; countery < outsize.y; countery++)
            {
                uint32_t x = counterx / outScale.x;
                uint32_t y = countery / outScale.y;
                uint32_t index = imageList[counter].channels() * (w * y + x);
                if(pixels[index+3] == 0x00)
                    outPixels[counter][counterx][countery].first = ' ';
                else
                {
                    //outPixels[counter][counterx][countery].first=(pixels[index] * 0.3 + pixels[index + 1] *0.59 + pixels[index+2] * 0.11)/QuantumRange * 255.0f;
                    outPixels[counter][counterx][countery].first=((pixels[index]+pixels[index+1]+pixels[index+2])/3.0) / QuantumRange * 255.0f;
                    if(inverse)
                        outPixels[counter][counterx][countery].first=255.0-outPixels[counter][counterx][countery].first;
                    outPixels[counter][counterx][countery].first= ASCIITable.at((outPixels[counter][counterx][countery].first/256.0) * ASCIITable.size());
                    outPixels[counter][counterx][countery].second = fillColor;
                    if(useColor)
                        outPixels[counter][counterx][countery].second=sf::Color( pixels[index]/QuantumRange * 255.0f, pixels[index+1]/QuantumRange * 255.0f, pixels[index+2]/QuantumRange * 255.0f);
                }
            }
    }
    return true;
}

bool GenerateGif(const std::string& fontFile, std::vector<std::vector<std::vector<std::pair<uint8_t,sf::Color>>>>& chars, const std::string& outFile, uint32_t pointSize, std::vector<uint32_t>& delays, sf::Color backColor)
{
    sf::Font Font;
    if(!Font.loadFromFile(fontFile))
    {
        std::cout <<"Failed to load font " <<fontFile<<"\n";
        return false;
    }

    sf::Vector2f charSize=Font.getGlyph('#', pointSize, false).bounds.getSize();

    std::vector<Magick::Image> outImages(chars.size());

    sf::RenderTexture tex;
    tex.create(charSize.x * chars[0].size(), charSize.y * chars[0][0].size());
    sf::Text text;
    text.setFont(Font);
    text.setCharacterSize(pointSize);
    text.setFillColor(sf::Color::White);
    for(uint32_t counter=0; counter < chars.size(); counter++)
    {
        tex.clear(backColor);
        for(uint32_t counterx=0; counterx < chars[counter].size(); counterx++)
            for(uint32_t countery=0; countery < chars[counter][counterx].size(); countery++)
        {
            text.setFillColor(chars[counter][counterx][countery].second);
            text.setString(sf::String((char)chars[counter][counterx][countery].first));
            text.setPosition(charSize.x * counterx, charSize.y * countery);
            tex.draw(text);
        }
        tex.display();
        sf::Image image = tex.getTexture().copyToImage();
        image.saveToFile("tmp.tga");
        outImages[counter].read("tmp.tga");
        outImages[counter].animationDelay(delays[counter]);
        outImages[counter].gifDisposeMethod((MagickCore::DisposeType)2);
        outImages[counter].rotate(180.0);
    }
    std::cout <<"Writing "<<outImages.size() <<" images.\n";
    Magick::writeImages(outImages.begin(), outImages.end(), outFile.c_str());
    return true;
}

void GenerateAsciiAnimation(const std::string& outfile, std::vector<std::vector<std::vector<std::pair<uint8_t, sf::Color>>>>& outPixels, std::vector<uint32_t>& delays)
{
    AsciiCmd::Animation anim;
    for(uint32_t frameCounter=0; frameCounter < outPixels.size(); frameCounter++)
    {
        AsciiCmd::Texture newTex(Vector2<uint32_t>(outPixels[frameCounter].size(), outPixels[frameCounter][0].size()));
        for(uint32_t counterx=0; counterx < outPixels[frameCounter].size(); counterx++)
            for(uint32_t countery=0; countery < outPixels[frameCounter][counterx].size(); countery++)
            newTex.SetPixel(Vector2<uint32_t>(counterx,countery), outPixels[frameCounter][counterx][countery].first, 0x0f);
        anim.AddFrame(newTex);
        anim.SetFrameDelay(frameCounter, 10 * delays[frameCounter]);
    }
    anim.SaveToFile(outfile);
}



bool WriteAscii(const std::string& outfile, std::vector<std::vector<std::pair<uint8_t,sf::Color>>>& ascii)
{
    std::ofstream file;
    file.open(outfile.c_str(), std::ofstream::out);
    if(!file.is_open())
        return false;
    for(uint32_t countery=0; countery < ascii[0].size(); countery++)
    {
        for(uint32_t counterx=0; counterx < ascii.size(); counterx++)
        {
            file <<ascii[counterx][countery].first;
        }
        file<<"\n";
    }
    return true;
}


bool HandleArguments(int argc, char* argv[], sf::Vector2f& scale, std::string& inFile, std::string& outFile, std::string& fontFile, uint32_t& pointSize, bool& userColor, sf::Color& backColor, bool& asciiOut, sf::Color& fillColor, float& dt, bool& inverse)
{
    for(int32_t currentArg=1; currentArg < argc; currentArg++)
    {
        std::string argtype = argv[currentArg];
        if(argtype == "--scale")
        {
            if(currentArg+1 >= argc || currentArg+2 >= argc)
            {
                std::cout <<"Expected arguments x and y after scale.\n";
                std::cout <<"Use --help to get help info.\n";
                return false;
            }
            scale.x = std::stof(argv[++currentArg]);
            scale.y = std::stof(argv[++currentArg]);
            if(scale.x > 1.0 || scale.y > 1.0 || scale.x <= 0.0 || scale.y <= 0.0)
            {
                std::cout << "Scale expected to be between 0.0 and 1.0.\n";
                std::cout <<"Use --help to get help info.\n";
                return false;
            }
        }
        else
        if(argtype == "--speed")
        {
            if(currentArg+1 >= argc)
            {
                std::cout <<"Expected time constant after --speed.\n";
                return false;
            }
            dt = std::stof(argv[++currentArg]);
        }
        else
        if(argtype == "--inverse")
        {
            inverse=true;
        }
        else
        if(argtype == "--font")
        {
            if(currentArg+1 >= argc)
            {
                std::cout <<"Expected font file name after --font.\n";
                std::cout <<"Use --help to get help info.\n";
                return false;
            }
            fontFile=argv[++currentArg];
        }
        else
        if(argtype == "--out")
        {
            if(currentArg+1 >= argc)
            {
                std::cout <<"Expected output file name after --out.\n";
                std::cout <<"Use --help to get help info.\n";
                return false;
            }
            outFile=argv[++currentArg];
        }
        else
        if(argtype == "--size")
        {
            if(currentArg+1 >= argc)
            {
                std::cout <<"Expected point size after --size\n";
                std::cout <<"Use --help to get help info.\n";
                return false;
            }
            pointSize = std::stoi(argv[++currentArg]);
        }
        else
        if(argtype == "--ascii")
            asciiOut=true;
        else
        if(argtype == "--transparent")
            backColor=sf::Color::Transparent;
        else
        if(argtype == "--backcolor")
        {
            if(currentArg + 3 >= argc)
            {
                std::cout <<"Expected r g b after back color.\n";
                return false;
            }
            backColor.r = std::stoi(argv[++currentArg]);
            backColor.g = std::stoi(argv[++currentArg]);
            backColor.b = std::stoi(argv[++currentArg]);
        }
        else
        if(argtype == "--fillcolor")
        {
            if(currentArg+3 >= argc)
            {
                std::cout <<"Expected r g b after fill color.\n";
                return false;
            }
            fillColor.r = std::stoi(argv[++currentArg]);
            fillColor.g = std::stoi(argv[++currentArg]);
            fillColor.b = std::stoi(argv[++currentArg]);
        }
        else
        if(argtype == "--help")
        {
            std::cout <<"Help Info:\n";
            std::cout <<"Usage: GifToAscii inputFile.gif.\n";
            std::cout <<"Options:\n";
            std::cout <<"--size pointSize : The point size of the text (default 12).\n";
            std::cout <<"--font fontFile : The file to use for the font (default font.ttf).\n";
            std::cout <<"--out outFile : The file to save the output to (default out.gif).\n";
            std::cout <<"--scale x y : The scale of the output x and y Range (0.0,1.0] (default 1.0,1.0).\n";
            std::cout <<"--color : Uses the colors of the image when generating the gif.\n";
            std::cout <<"--transparent: Sets the background color to be transparent rather than black.\n";
            return false;
        }
        else
        if(argtype == "--color")
            userColor=true;
        else
        {
            inFile=argtype;
        }
    }
    return true;
}


int main(int argc, char* argv[])
{
    bool useColor=false;
    sf::Vector2f scale=sf::Vector2f(1.0,1.0);
    std::string inFile="in.gif";
    std::string outFile="out.gif";
    uint32_t pointSize = 12;
    std::string fontFile = "font.ttf";
    sf::Color backColor=sf::Color::Black;
    sf::Color fillColor=sf::Color::White;
    bool inverse=false;
    float speed=1.0;
    bool asciiOut=false;

    if(!HandleArguments(argc, argv, scale, inFile, outFile, fontFile, pointSize, useColor, backColor, asciiOut, fillColor, speed, inverse))
        return 0;

    std::cout <<"Converting.\n";
    std::vector<std::vector<std::vector<std::pair<uint8_t, sf::Color>>>> ascii;
    std::vector<uint32_t> delays;
    if(!ConvertToAscii(inFile, scale,ascii, delays, useColor,fillColor, speed, inverse))
        return 0;
    std::cout <<"Generating.\n";
    if(asciiOut)
        GenerateAsciiAnimation(outFile, ascii, delays);
    else
        GenerateGif(fontFile, ascii, outFile, pointSize, delays, backColor);
    std::cout <<"Done.\n";
    return 0;
}
