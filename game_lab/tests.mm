#import <Foundation/Foundation.h>
#include "emperor_assets.hpp"
#include <iostream>
#include <fstream>
#include <chrono>

using namespace game_lab;

namespace {
struct Stats {
    size_t assertions = 0;
    size_t testsPassed = 0;
} g_stats;

void check(bool condition, const std::string& msg) {
    ++g_stats.assertions;
    if (!condition) {
        std::cerr << "ASSERTION FAILED: " << msg << std::endl;
        std::exit(1);
    }
}
#define CHECK(cond) check((cond), #cond " at line " + std::to_string(__LINE__))
} // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    @autoreleasepool {
        const auto start = std::chrono::steady_clock::now();
        std::cout << "Starting GameData native asset verification..." << std::endl;

        // 1. JPEG Verification
        {
            std::cout << "[1/7] Testing JPEG loading via ImageIO (China_FE_MainMenu.jpg)..." << std::endl;
            const auto img = loadJPEG("GameData/DATA/China_FE_MainMenu.jpg");
            CHECK(img.valid());
            CHECK(img.width == 1024);
            CHECK(img.height == 768);
            CHECK(img.pixelsBGRA.size() == 1024 * 768);
            std::cout << "  -> Main menu decoded: 1024x768 BGRA8 (" << img.pixelsBGRA.size() << " pixels)" << std::endl;
            ++g_stats.testsPassed;
        }

        // 2. Raw 555 Verification
        {
            std::cout << "[2/7] Testing Raw 555 loading and RGB555LE conversion (China_DEMO1.555)..." << std::endl;
            const auto raw = loadRaw555("GameData/DATA/China_DEMO1.555", 640, 480);
            CHECK(raw.valid());
            CHECK(raw.width == 640);
            CHECK(raw.height == 480);
            CHECK(raw.pixels555.size() == 640 * 480);
            CHECK(raw.pixelsBGRA.size() == 640 * 480);
            std::cout << "  -> Demo screen decoded: 640x480 RGB555LE -> BGRA8" << std::endl;
            ++g_stats.testsPassed;
        }

        // 3. SG3 Sprite Archive Parsing
        {
            std::cout << "[3/7] Testing SG3 archive parsing (China_Elevation.sg3 & China_General.sg3)..." << std::endl;
            const auto elev = loadSG3("GameData/DATA/China_Elevation.sg3");
            CHECK(elev.version == 213);
            CHECK(elev.numImages == 372);
            CHECK(elev.sprites.size() == 372);
            CHECK(elev.sprites[201].offset == 39218);
            CHECK(elev.sprites[201].type == 78);

            const auto gen = loadSG3("GameData/DATA/China_General.sg3");
            CHECK(gen.version == 213);
            CHECK(gen.numImages == 2941);
            CHECK(gen.sprites.size() == 2941);
            CHECK(gen.sprites[6].type == 16);
            CHECK(gen.sprites[6].length == 512);
            std::cout << "  -> Elevation SG3: 372 sprites verified; General SG3: 2941 sprites verified" << std::endl;
            ++g_stats.testsPassed;
        }

        // 4. Sprite Extraction to BGRA8
        {
            std::cout << "[4/7] Testing sprite extraction from companion .555 file..." << std::endl;
            const auto gen = loadSG3("GameData/DATA/China_General.sg3");
            const auto spriteTile = extractSprite(gen.sprites[6], "GameData/DATA/China_General.555");
            CHECK(spriteTile.valid());
            CHECK(spriteTile.width == 16);
            CHECK(spriteTile.height == 16);
            CHECK(spriteTile.pixelsBGRA.size() == 256);
            std::cout << "  -> Extracted 16x16 tile sprite into BGRA8 texture successfully" << std::endl;
            ++g_stats.testsPassed;
        }

        // 5. RIFF WAV Linear PCM Verification
        {
            std::cout << "[5/7] Testing RIFF WAV audio decoding (Fire.wav & D_earthquake.wav)..." << std::endl;
            const auto wav1 = loadWAV("GameData/Audio/Wavs/Fire.wav");
            CHECK(wav1.valid());
            CHECK(wav1.channels == 1);
            CHECK(wav1.sampleRate == 22050);
            CHECK(wav1.bitsPerSample == 16);
            CHECK(wav1.samples.size() == 132248);

            const auto wav2 = loadWAV("GameData/Audio/Wavs/D_earthquake.wav");
            CHECK(wav2.valid());
            CHECK(wav2.channels == 2);
            CHECK(wav2.sampleRate == 22050);
            CHECK(wav2.bitsPerSample == 16);
            CHECK(wav2.samples.size() / wav2.channels == 208172);
            std::cout << "  -> Fire.wav (mono 22.05kHz, " << wav1.samples.size() << " samples) & Earthquake.wav (stereo 22.05kHz, " << (wav2.samples.size()/2) << " frames) verified" << std::endl;
            ++g_stats.testsPassed;
        }

        // 6. City Map zlib Decompression
        {
            std::cout << "[6/7] Testing city map zlib decompression (Anyang.map)..." << std::endl;
            const auto map = loadCityMap("GameData/Cities/Anyang.map");
            CHECK(map.valid());
            CHECK(map.width == 128);
            CHECK(map.height == 128);
            CHECK(map.tiles.size() == 16384);
            std::cout << "  -> Anyang map verified: 128x128 grid decompressed (" << map.tiles.size() << " tile IDs)" << std::endl;
            ++g_stats.testsPassed;
        }

        // 7. Chinese Text Transcoding & Classical Music Check
        {
            std::cout << "[7/7] Testing localized Chinese text catalog & music assets..." << std::endl;
            const auto lines = loadEmperorText("GameData/EmperorText.txt");
            CHECK(lines.size() > 500);

            bool foundNewGame = false;
            bool foundExit = false;
            for (const auto& line : lines) {
                if (line.find("新建游戏") != std::string::npos) foundNewGame = true;
                if (line.find("退出游戏") != std::string::npos) foundExit = true;
            }
            CHECK(foundNewGame);
            CHECK(foundExit);

            std::ifstream musicFile("GameData/Audio/Music/MENU1-San_Xuan.mp3", std::ios::binary);
            CHECK(bool(musicFile));
            musicFile.seekg(0, std::ios::end);
            CHECK(musicFile.tellg() > 1000000); // > 1MB

            std::cout << "  -> EmperorText.txt parsed: " << lines.size() << " localized Chinese strings (found '新建游戏', '退出游戏')" << std::endl;
            std::cout << "  -> Chinese Menu Sanxian music track (MENU1-San_Xuan.mp3) verified: " << musicFile.tellg() << " bytes" << std::endl;
            ++g_stats.testsPassed;
        }

        const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        std::cout << "ALL GAMEDATA ASSET VERIFICATIONS PASSED: "
                  << g_stats.testsPassed << "/7 tests, "
                  << g_stats.assertions << " assertions, elapsed "
                  << elapsed << " ms." << std::endl;

        std::ofstream summary("game_lab/evidence/summary.json");
        summary << "{\n"
                << "  \"passed\": true,\n"
                << "  \"tests_passed\": " << g_stats.testsPassed << ",\n"
                << "  \"assertions\": " << g_stats.assertions << ",\n"
                << "  \"elapsed_ms\": " << elapsed << ",\n"
                << "  \"gamedata_verified\": true\n"
                << "}\n";
    }
    return 0;
}
