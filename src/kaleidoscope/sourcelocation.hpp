#ifndef INCLUDED_KALEIDOSCOPE_SOURCE_LOCATION_HPP
#define INCLUDED_KALEIDOSCOPE_SOURCE_LOCATION_HPP

namespace kaleidoscope
{
    class SourceLocation
    {
    public:
        void advance(char c);
        void advanceLine();

        int line() const { return line_; }
        int column() const { return column_; }

    private:
        int line_ = 1;
        int column_ = 0;
    };
}

#endif
