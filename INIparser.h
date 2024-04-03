#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <fstream>
#include <array>

namespace INIparser
{
class INIparser
{
public:

    typedef std::unordered_map<std::string, std::string> Section;

    class InvalidToken : public std::runtime_error
    {
    public:

        InvalidToken(const std::string &message) : std::runtime_error(message) {}
    };

public:

    INIparser() = default;
    ~INIparser() = default;

    void load(std::string file_path);

    const std::string &get(const std::string &key) const;
    const std::string &get(const std::string &section, const std::string &key) const;

private:

    std::unordered_map<std::string, Section> m_sections;

    enum class TokenType
    {
        SECTION = 0,
        KEY     = 1,
        VAL     = 2,
    };

    struct ReadState
    {
        TokenType current_token_type;

        std::array<std::string, 3> current {
            " ", // SECTION
            "", // KEY
            ""  // VAL
        };
    };

    ReadState m_read_state;

private:

    // returns false if empty line
    bool readLine(std::fstream &file);

    const std::string &getVal(
        const std::string &section,
        const std::string &key
    ) const;
};

const std::string &INIparser::get(const std::string &key) const {
    return getVal(" ", key);
}

const std::string &INIparser::get(const std::string &section, const std::string &key) const {
    return getVal(section, key);
}

void INIparser::load(std::string file_path) {
    std::fstream file;
    file.open(file_path, std::ios::in);

    if (!file.is_open()) throw std::runtime_error("Failed to open file");

    m_sections.clear();
    m_read_state = ReadState();

    while (file.good()) {
        if (!readLine(file)) continue;

        if (m_read_state.current_token_type != TokenType::SECTION) {
            Section section;
            m_sections[m_read_state.current[0]][m_read_state.current[1]] = m_read_state.current[2];
        }

        m_read_state.current[1] = "";
        m_read_state.current[2] = "";

        m_read_state.current_token_type = TokenType::KEY;
    }
}

bool INIparser::readLine(std::fstream &file) {
    char current_char = ' ';

    bool escape = false;
    bool escape_used = false;

    bool read = false;
    bool first = true;

    m_read_state.current_token_type = TokenType::KEY;

    while (file.get(current_char) && current_char != '\n') {
        if (
            current_char == '\t' ||
            current_char == '\f' ||
            current_char == '\r' ||
            current_char == '\v'
        ) {
            continue;
        }

        if (m_read_state.current_token_type != TokenType::SECTION && current_char == ' ') {
            continue;
        }

        if (current_char == ';' && !escape) {
            std::string _;
            getline(file, _);
            return m_read_state.current[int(TokenType::KEY)] != "" && m_read_state.current[int(TokenType::VAL)] != "";
        }

        read = true;

        if (current_char == '=' && !escape) {
            if (m_read_state.current[int(TokenType::KEY)] == "") throw InvalidToken("Key can\'t be empty");
            m_read_state.current_token_type = TokenType::VAL;
            continue;
        }

        if (current_char == '[' && !escape) {
            if (!first) throw InvalidToken("\'[\' must be the first character in a line");
            m_read_state.current[int(TokenType::SECTION)] = "";
            m_read_state.current_token_type = TokenType::SECTION;
            continue;
        }

        first = false;

        if (current_char == ']' && !escape) {
            if (m_read_state.current_token_type != TokenType::SECTION) throw InvalidToken("Unexpected character: \']\'");
            if (m_read_state.current[int(TokenType::SECTION)] == "") throw InvalidToken("Section name cannot be empty");
            return false;
        }

        if (current_char == '\\') {
            if (escape) {
                m_read_state.current[int(m_read_state.current_token_type)] += "\\";
                escape = false;
            } else {
                escape = true;
            }
            continue;
        }

        escape = false;

        m_read_state.current[int(m_read_state.current_token_type)] += current_char;
    }

    if (
        read &&
        m_read_state.current_token_type != TokenType::SECTION &&
        m_read_state.current[int(TokenType::KEY)] == "" &&
        m_read_state.current[int(TokenType::VAL)] == ""
    ) throw std::runtime_error("Uncomplete line");

    return read;
}

const std::string &INIparser::getVal(
    const std::string &section,
    const std::string &key
) const {
    auto section_iter = m_sections.find(section);
    if (section_iter == m_sections.end()) throw std::runtime_error("Section not found");

    auto key_iter = section_iter->second.find(key);
    if (key_iter == section_iter->second.end()) throw std::runtime_error("Key not found");

    return key_iter->second;
}
}