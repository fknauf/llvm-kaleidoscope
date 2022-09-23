#ifndef INCLUDED_KALEIDOSCOPE_TOKEN_HPP
#define INCLUDED_KALEIDOSCOPE_TOKEN_HPP

#include <cassert>
#include <limits>
#include <string>

namespace kaleidoscope
{
    enum TokenType
    {
        // < 0 means no value associated with the token
        tok_eof = -1,
        tok_def = -2,
        tok_extern = -3,

        // > 0: value associated
        tok_identifier = 4,
        tok_number = 5,
        tok_char = 6
    };

    class Token
    {
    public:
        Token(TokenType const &type) : type_(type) { assert(type < 0); }
        Token(char charValue) : type_(tok_char), charValue_(charValue) {}
        Token(double numValue) : type_(tok_number), numValue_(numValue) {}
        Token(std::string identifier)
            : type_(tok_identifier), identifier_(identifier) {}

        TokenType getType() const noexcept { return type_; }

        char getCharValue() const
        {
            assert(type_ == tok_char);
            return charValue_;
        }

        double getNumValue() const
        {
            assert(type_ == tok_number);
            return numValue_;
        }

        std::string const &getIdentifierValue() const
        {
            assert(type_ == tok_identifier);
            return identifier_;
        }

        bool operator==(Token const &other) const
        {
            if (getType() == other.getType())
            {
                switch (getType())
                {
                case tok_char:
                    return getCharValue() == other.getCharValue();
                case tok_number:
                    return getNumValue() == other.getNumValue();
                case tok_identifier:
                    return getIdentifierValue() ==
                           other.getIdentifierValue();
                default:
                    return true;
                }
            }
            else
            {
                return false;
            }
        }

        bool operator!=(Token const &other) const { return !(*this == other); }

    private:
        TokenType type_;
        char charValue_ = '\0';
        double numValue_ = std::numeric_limits<double>::quiet_NaN();
        std::string identifier_;
    };
} // namespace kaleidoscope

#endif
