#include "string_utils.h"
#include <locale>
#include <sstream>

namespace raid6 {
struct coma_numpunct : std::numpunct<char>
{
protected:
    virtual char do_thousands_sep () const override { return ','; }
    virtual char do_decimal_point () const override { return '.'; }
    virtual std::string do_grouping () const override { return "\03"; }
};

std::string thousand_separated (double x, int precision)
{
    const size_t MAX_PREC = 4;
    size_t p = (size_t)precision;
    if (x == 0.0)
        p = MAX_PREC;
    else if (precision < 0)
    {
        double l = floor (log10 (x)) + 1;
        if (l > 3)
            p = 0;
        else if (l < -3)
            p = MAX_PREC;
        else
            p = (size_t)(MAX_PREC - (int)l);
    }
    p = p > MAX_PREC ? MAX_PREC : p;
    std::ostringstream os;
    os.imbue (std::locale (os.getloc (), new coma_numpunct));
    //os.imbue (std::locale ("en-US"));
    os.precision (p);
    std::fixed (os);
    os << x;
    return os.str ();
}

std::string thousand_separated_prop (double x)
{
    return thousand_separated (x, -1);
}

std::string thousand_separated (unsigned long long x)
{
    std::ostringstream os;
    os.imbue (std::locale (os.getloc (), new coma_numpunct));
    //os.imbue (std::locale ("en-US"));
    os << x;
    return os.str ();
}

std::string thousand_separated (long long x)
{
    std::ostringstream os;
    os.imbue (std::locale (os.getloc (), new coma_numpunct));
    //os.imbue (std::locale ("en-US"));
    os << x;
    return os.str ();
}
}