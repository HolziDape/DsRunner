#pragma once
#include <stdint.h>
#include <string.h>

/// @brief DsRunner: the keys of the in-game reset (settings.json "inGameResetKeys"), as the
///        bits of REG_KEYINPUT that read 0 while held. Only the keys the arm9 can read are
///        possible: A, B, SELECT, START, RIGHT, LEFT, UP, DOWN, R, L (not X and Y).
///        Free of DsRunner dependencies, so loader/test can check the parser on a pc.
namespace DsRunnerInGameResetKeys
{
    /// The default: L + R + DOWN + B. Must match IN_GAME_RESET_DEFAULT_KEY_MASK in loader/patches.
    inline constexpr uint32_t kDefaultMask = 0x382;
    inline constexpr const char* kDefaultText = "L+R+DOWN+B";

    /// Fewer keys would be too easy to hit by accident.
    inline constexpr uint32_t kMinimumKeys = 3;

    struct KeyName
    {
        const char* name;
        uint32_t bit;
    };

    /// Names in REG_KEYINPUT bit order, then German alternatives for the directions.
    inline constexpr KeyName kKeyNames[] =
    {
        { "A", 0 }, { "B", 1 }, { "SELECT", 2 }, { "START", 3 }, { "RIGHT", 4 }, { "LEFT", 5 },
        { "UP", 6 }, { "DOWN", 7 }, { "R", 8 }, { "L", 9 },
        { "RECHTS", 4 }, { "LINKS", 5 }, { "OBEN", 6 }, { "UNTEN", 7 },
    };

    enum class ParseResult
    {
        Ok,
        UnknownKey,   // a name that is not one of kKeyNames (X and Y included)
        TooFewKeys,   // fewer than kMinimumKeys different keys
    };

    inline bool IsSeparator(char c)
    {
        return c == '+' || c == ' ' || c == ',' || c == '\t';
    }

    inline char ToUpper(char c)
    {
        return c >= 'a' && c <= 'z' ? (char)(c - 'a' + 'A') : c;
    }

    /// @brief Parses "L+R+DOWN+B" (case-insensitive, separated by '+', spaces or commas).
    /// @param mask Receives the key mask when the result is ParseResult::Ok.
    inline ParseResult Parse(const char* text, uint32_t& mask)
    {
        mask = 0;
        const char* p = text;
        while (*p != 0)
        {
            while (IsSeparator(*p))
                p++;
            if (*p == 0)
                break;
            const char* start = p;
            while (*p != 0 && !IsSeparator(*p))
                p++;
            uint32_t length = p - start;
            bool known = false;
            for (const auto& key : kKeyNames)
            {
                if (strlen(key.name) != length)
                    continue;
                uint32_t i = 0;
                while (i < length && ToUpper(start[i]) == key.name[i])
                    i++;
                if (i == length)
                {
                    mask |= 1u << key.bit;
                    known = true;
                    break;
                }
            }
            if (!known)
                return ParseResult::UnknownKey;
        }
        uint32_t count = 0;
        for (uint32_t bit = 0; bit < 10; bit++)
        {
            if (mask & (1u << bit))
                count++;
        }
        return count < kMinimumKeys ? ParseResult::TooFewKeys : ParseResult::Ok;
    }

    /// Order of the names when writing them: L, R, SELECT, START, UP, DOWN, LEFT, RIGHT, A, B.
    inline constexpr uint8_t kFormatOrder[] = { 9, 8, 2, 3, 6, 7, 5, 4, 0, 1 };

    /// @brief Writes the names of a mask in kFormatOrder: "L+R+DOWN+B", "SELECT+START+DOWN".
    inline void Format(uint32_t mask, char* buffer, uint32_t bufferSize)
    {
        uint32_t length = 0;
        buffer[0] = 0;
        for (uint32_t bit : kFormatOrder)
        {
            if (!(mask & (1u << bit)))
                continue;
            const char* name = kKeyNames[bit].name;
            uint32_t nameLength = strlen(name);
            if (length + (length ? 1 : 0) + nameLength + 1 > bufferSize)
                break;
            if (length)
                buffer[length++] = '+';
            memcpy(buffer + length, name, nameLength);
            length += nameLength;
            buffer[length] = 0;
        }
    }
}
