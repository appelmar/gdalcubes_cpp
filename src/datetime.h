/*
   Copyright 2018 Marius Appel <marius.appel@uni-muenster.de>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/**
 * Simplistic library for datetime objects that works on top of boost::posix_time.
 * The assumption of the library is that durations are only meaningful if given as a single number with a single datetime unit. Whether or not this assumption
 * is valid depends on the application.
 */

#ifndef DATETIME_H
#define DATETIME_H

#include <boost/date_time.hpp>
#include <boost/regex.hpp>

enum datetime_unit {
    SECOND = 0,
    MINUTE = 1,
    HOUR = 2,
    DAY = 3,
    WEEK = 4,
    MONTH = 5,
    YEAR = 6,
    NONE = 255
};

struct duration {
    duration() : dt_interval(0), dt_unit(DAY) {}
    duration(int32_t interval, datetime_unit unit) : dt_interval(interval), dt_unit(unit) {}

    int32_t dt_interval;
    datetime_unit dt_unit;

    std::string to_string() {
        switch (dt_unit) {
            case SECOND:
                return "PT" + std::to_string(dt_interval) + "S";
            case MINUTE:
                return "PT" + std::to_string(dt_interval) + "M";
            case HOUR:
                return "PT" + std::to_string(dt_interval) + "H";
            case DAY:
                return "P" + std::to_string(dt_interval) + "D";
            case WEEK:
                return "P" + std::to_string(dt_interval) + "W";
            case MONTH:
                return "P" + std::to_string(dt_interval) + "M";
            case YEAR:
                return "P" + std::to_string(dt_interval) + "Y";
        }
    }

    static duration from_string(std::string s) {
        boost::regex rexp("P(T?)([0-9]+)([YMWD])");

        boost::cmatch res;
        if (!boost::regex_match(s.c_str(), res, rexp)) {
            throw std::string("ERROR in duration::from_string(): cannot derive date interval");
        }
        duration d;
        d.dt_interval = std::stoi(res[2]);
        if (!res[1].str().empty()) {
            if (res[3] == "H")
                d.dt_unit = HOUR;
            else if (res[3] == "M")
                d.dt_unit = MINUTE;
            else if (res[3] == "S")
                d.dt_unit = SECOND;
        } else {
            if (res[3] == "Y")
                d.dt_unit = YEAR;
            else if (res[3] == "M")
                d.dt_unit = MONTH;
            else if (res[3] == "W")
                d.dt_unit = WEEK;
            else if (res[3] == "D")
                d.dt_unit = DAY;
        }
        return d;
    }

    friend duration operator*(duration l, const int& r) {
        duration out;
        out.dt_unit = l.dt_unit;
        out.dt_interval = l.dt_interval * r;
        return out;
    }

    friend duration operator+(duration l, const int& r) {
        duration out;
        out.dt_unit = l.dt_unit;
        out.dt_interval = l.dt_interval + r;
        return out;
    }

    friend duration operator/(duration l, const int& r) {
        duration out;
        out.dt_unit = l.dt_unit;
        out.dt_interval = l.dt_interval / r;
        return out;
    }

    friend int operator/(duration l, duration& r) {
        if (l.dt_unit != r.dt_unit) {
            throw std::string("ERROR in duration::operator/(): Incompatible datetime duration units");
        }
        if (r.dt_interval == 0) {
            throw std::string("ERROR in duration::operator/(): Division by zero");
        }

        return l.dt_interval / r.dt_interval;
    }

    friend int operator%(duration l, duration& r) {
        if (l.dt_unit != r.dt_unit) {
            throw std::string("ERROR in duration::operator%(): Incompatible datetime duration units");
        }
        if (r.dt_interval == 0) {
            throw std::string("ERROR in duration::operator%(): Division by zero");
        }

        return l.dt_interval % r.dt_interval;
    }

    friend duration operator-(duration l, const int& r) {
        duration out;
        out.dt_unit = l.dt_unit;
        out.dt_interval = l.dt_interval - r;
        return out;
    }
};

class datetime {
   public:
    datetime() : _p(), _unit(DAY) {}
    datetime(boost::posix_time::ptime p) : _p(p), _unit(DAY) {}
    datetime(boost::posix_time::ptime p, datetime_unit u) : _p(p), _unit(u) {}

    /**
     * Convert to a numeric datetime format as in 20180401123059.
     * This function does **not** convert the datetime to a timestamp or similar
     * @return
     */
    double to_double() {
        double out = _p.date().year();
        if (_unit <= MONTH) {
            out *= 100;
            out += _p.date().month();
        }
        if (_unit <= DAY) {
            out *= 100;
            out += _p.date().day();
        }
        if (_unit <= HOUR) {
            out *= 100;
            out += _p.time_of_day().hours();
        }
        if (_unit <= MINUTE) {
            out *= 100;
            out += _p.time_of_day().minutes();
        }
        if (_unit <= SECOND) {
            out *= 100;
            out += _p.time_of_day().seconds();
        }
        return out;
    }

    std::string to_string() {
        std::stringstream os;
        std::string format;

        os.imbue(std::locale(std::locale::classic(), new boost::posix_time::time_facet(datetime_format_for_unit(_unit).c_str())));
        os << _p;
        return os.str();
    }

    /**
     * Produce datetime string up to the given unit.
     * The string will possibly include e.g. seconds although the unit is actually lower e.g. days.
     * This function is implemented to interface tools that require full datetime strings and do not work with
     * e.g. "2001-01".
     * @param u
     * @return
     */
    std::string to_string(datetime_unit u) {
        std::stringstream os;
        std::string format;

        os.imbue(std::locale(std::locale::classic(), new boost::posix_time::time_facet(datetime_format_for_unit(u).c_str())));
        os << _p;
        return os.str();
    }

    static datetime from_string(std::string s) {
        std::istringstream is(s);

        // TODO: Regex does not support ISO weeks / day of year yet
        boost::regex regex1("([0-9]{4})(?:-?([0-9]{2})(?:-?([0-9]{2})(?:[T\\s]?([0-9]{2})(?::?([0-9]{2})(?::?([0-9]{2}))?)?)?)?)?");
        datetime out;

        boost::cmatch res;
        if (!boost::regex_match(s.c_str(), res, regex1)) {
            throw std::string("ERROR in datetime::from_string(): cannot derive datetime from string");
        } else {
            if (!res.size() == 7) throw std::string("ERROR in datetime::from_string(): cannot derive datetime from string");
            uint16_t i = 2;
            while (!res[i].str().empty() && i < 7) ++i;
            if (i == 2) {
                out._p = boost::posix_time::ptime(boost::gregorian::date(std::stoi(res[1].str()), 1, 1), boost::posix_time::time_duration(0, 0, 0));
                out._unit = YEAR;
            } else if (i == 3) {
                out._p = boost::posix_time::ptime(boost::gregorian::date(std::stoi(res[1].str()), std::stoi(res[2].str()), 1), boost::posix_time::time_duration(0, 0, 0));
                out._unit = MONTH;
            } else if (i == 4) {
                out._p = boost::posix_time::ptime(boost::gregorian::date(std::stoi(res[1].str()), std::stoi(res[2].str()), std::stoi(res[3].str())), boost::posix_time::time_duration(0, 0, 0));
                out._unit = DAY;
            } else if (i == 5) {
                out._p = boost::posix_time::ptime(boost::gregorian::date(std::stoi(res[1].str()), std::stoi(res[2].str()), std::stoi(res[3].str())), boost::posix_time::time_duration(std::stoi(res[4].str()), 0, 0));
                out._unit = HOUR;
            } else if (i == 6) {
                out._p = boost::posix_time::ptime(boost::gregorian::date(std::stoi(res[1].str()), std::stoi(res[2].str()), std::stoi(res[3].str())), boost::posix_time::time_duration(std::stoi(res[4].str()), std::stoi(res[5].str()), 0));
                out._unit = MINUTE;
            } else if (i == 7) {
                out._p = boost::posix_time::ptime(boost::gregorian::date(std::stoi(res[1].str()), std::stoi(res[2].str()), std::stoi(res[3].str())), boost::posix_time::time_duration(std::stoi(res[4].str()), std::stoi(res[5].str()), std::stoi(res[6].str())));
                out._unit = SECOND;
            }
        }

        //        for (uint16_t i = 0; i < res.size(); ++i) {
        //            std::cout << "Expression " << i << ": " << res[i] << "empty: " << res[i].str().empty() << std::endl;
        //        };
        //        std::cout << "--------------------------------" << std::endl;

        return out;
    }

    datetime_unit& unit() { return _unit; }
    datetime_unit unit() const { return _unit; }

    friend duration operator-(const datetime& l,
                              const datetime& r) {
        duration out;
        out.dt_unit = std::max(l.unit(), r.unit());  // TODO: warning if not the same unit
        switch (out.dt_unit) {
            case SECOND:
                out.dt_interval = (l._p - r._p).total_seconds();
                break;
            case MINUTE:
                out.dt_interval = (l._p - r._p).total_seconds() / 60;
                break;
            case HOUR:
                out.dt_interval = (l._p - r._p).total_seconds() / (3600);
                break;
            case DAY:
                out.dt_interval = (l._p.date() - r._p.date()).days();
                break;
            case WEEK:
                out.dt_interval = (l._p.date() - r._p.date()).days() / 7;
                break;
            case MONTH: {
                int yd = l._p.date().year() - r._p.date().year();
                if (std::abs(yd) >= 1) {
                    out.dt_interval = yd * 12 + (l._p.date().month() - r._p.date().month());
                } else if (yd == 0) {
                    out.dt_interval = (l._p.date().month() - r._p.date().month());
                }
                break;
            }
            case YEAR:
                out.dt_interval = l._p.date().year() - r._p.date().year();
                break;
        }
        return out;
    }

    friend bool operator==(const datetime& l, const datetime& r) {
        if (l.unit() != r._unit) return false;  // TODO: exception
        switch (l.unit()) {
            case SECOND:
                return (l._p.date() == r._p.date() &&
                        l._p.time_of_day().hours() == r._p.time_of_day().hours() &&
                        l._p.time_of_day().minutes() == r._p.time_of_day().minutes() &&
                        l._p.time_of_day().seconds() == r._p.time_of_day().seconds());
            case MINUTE:
                return (l._p.date() == r._p.date() &&
                        l._p.time_of_day().hours() == r._p.time_of_day().hours() &&
                        l._p.time_of_day().minutes() == r._p.time_of_day().minutes());
            case HOUR:
                return (l._p.date() == r._p.date() &&
                        l._p.time_of_day().hours() == r._p.time_of_day().hours());
            case DAY:
                return (l._p.date() == r._p.date());
            case WEEK:
                // TODO: what if the same week overlaps two years?
                return (l._p.date().year() == r._p.date().year() && l._p.date().week_number() == r._p.date().week_number());
            case MONTH:
                return (l._p.date().year() == r._p.date().year() && l._p.date().month() == r._p.date().month());

            case YEAR:
                return (l._p.date().year() == r._p.date().year());
        }
    }

    inline friend bool operator!=(const datetime& l, const datetime& r) { return !(l == r); }

    friend bool operator<(datetime& l, datetime& r) {
        if (l.unit() != r._unit) return false;  // TODO: exception
        switch (l.unit()) {
            case SECOND:
                if (l._p.date() < r._p.date()) return true;
                if (l._p.date() > r._p.date()) return false;
                return (l._p.time_of_day().total_seconds() < r._p.time_of_day().total_seconds());
            case MINUTE:
                if (l._p.date() < r._p.date()) return true;
                if (l._p.date() > r._p.date()) return false;
                if (l._p.time_of_day().hours() < r._p.time_of_day().hours()) return true;
                if (l._p.time_of_day().hours() > r._p.time_of_day().hours()) return false;
                return (l._p.time_of_day().hours() < r._p.time_of_day().hours());
            case HOUR:
                if (l._p.date() < r._p.date()) return true;
                if (l._p.date() > r._p.date()) return false;
                return (l._p.time_of_day().hours() < r._p.time_of_day().hours());
            case DAY:
                if (l._p.date().year() < r._p.date().year()) return true;
                if (l._p.date().year() > r._p.date().year()) return false;
                return (l._p.date().day_of_year() < r._p.date().day_of_year());
            case WEEK:
                // TODO: what if the same week overlaps two years?
                if (l._p.date().year() < r._p.date().year()) return true;
                if (l._p.date().year() > r._p.date().year()) return false;
                return (l._p.date().week_number() < r._p.date().week_number());
            case MONTH:
                if (l._p.date().year() < r._p.date().year()) return true;
                if (l._p.date().year() > r._p.date().year()) return false;
                return (l._p.date().month() < r._p.date().month());
            case YEAR:
                return l._p.date().year() < r._p.date().year();
        }
    }
    inline friend bool operator>(datetime& l, datetime& r) { return r < l; }
    inline friend bool operator<=(datetime& l, datetime& r) { return !(l > r); }
    inline friend bool operator>=(datetime& l, datetime& r) { return !(l < r); }

    friend datetime operator+(datetime l, const duration& r) {
        datetime out(l._p);
        switch (r.dt_unit) {
            case SECOND:
                out._p += boost::posix_time::seconds(r.dt_interval);
                break;
            case MINUTE:
                out._p += boost::posix_time::minutes(r.dt_interval);
                break;
            case HOUR:
                out._p += boost::posix_time::hours(r.dt_interval);
                break;
            case DAY:
                out = boost::posix_time::ptime(out._p.date() + boost::gregorian::days(r.dt_interval));  // ignore time
                break;
            case WEEK:
                out = boost::posix_time::ptime(out._p.date() + boost::gregorian::days(r.dt_interval * 7));  // ignore time
                break;
            case MONTH:
                out = boost::posix_time::ptime(boost::gregorian::date(out._p.date().year() + (long)(r.dt_interval / 12), out._p.date().month() + r.dt_interval % 12, 1));  // ignore time and day
                break;
            case YEAR:
                out = boost::posix_time::ptime(boost::gregorian::date(out._p.date().year() + r.dt_interval, 1, 1));  // ignore time, day, and month
                break;
        }
        return out;
    }

   protected:
    boost::posix_time::ptime _p;
    datetime_unit _unit;

    static std::string datetime_format_for_unit(datetime_unit u) {
        switch (u) {
            case SECOND:
                return "%Y-%m-%dT%H:%M:%S";
            case MINUTE:
                return "%Y-%m-%dT%H:%M";
            case HOUR:
                return "%Y-%m-%dT%H";
            case DAY:
                return "%Y-%m-%d";
            case WEEK:
                return "%Y-%m-%dT%H:%M:%S";
            case MONTH:
                return "%Y-%m";
            case YEAR:
                return "%Y";
        }
    }
};

#endif  //DATETIME_H
