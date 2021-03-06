﻿
#pragma once

#include "astronomy/date_time.hpp"

#include <array>

#include "base/macros.hpp"
#include "base/mod.hpp"
#include "glog/logging.h"

namespace principia {
namespace astronomy {
namespace date_time {
namespace internal_date_time {

using base::mod;

// Arithmetico-calendrical utility functions.

constexpr int mjd0_yyyy = 1858;
constexpr int mjd0_yyyymmdd = 1858'11'17;
constexpr int mjd0_jd0_offset = 2'400'000;  // 2'400'000.5, actually.

constexpr std::array<int, 12> non_leap_year_month_lengths{
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

constexpr bool is_gregorian_leap_year(int const year) {
  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

constexpr int gregorian_year_length(int const year) {
  return is_gregorian_leap_year(year) ? 366 : 365;
}

constexpr int month_length(int year, int month) {
  return (is_gregorian_leap_year(year) && month == 2)
             ? 29
             : non_leap_year_month_lengths[month - 1];
}

// Result in [1, 7], 1 is Monday.
constexpr int day_of_week_on_january_1st(int const year) {
  // Gauss's formula, see
  // https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week#Gauss.27s_algorithm.
  return mod(1 + 5 * ((year - 1) % 4) +
                 4 * ((year - 1) % 100) +
                 6 * ((year - 1) % 400),
             7,
             1);
}

constexpr int number_of_weeks_in_year(int const year) {
  return day_of_week_on_january_1st(year) == 4 ||
                 (is_gregorian_leap_year(year) &&
                  day_of_week_on_january_1st(year) == 3)
             ? 53
             : 52;
}

// Returns the ordinal in |year| of the first day of the first week of |year|.
// The result is in [-2, 4], with values in [-2, 0] meaning that the first week
// of |year| starts in |year - 1|.
// A result in [-2, 1] means that the first day of |year| is in the first week
// of |year|; otherwise, it is in the last week of |year - 1|.
constexpr int ordinal_of_w_01_1(int const year) {
  return mod(2 - day_of_week_on_january_1st(year), 7, -2);
}

constexpr int days_in_1_year = 365;
constexpr int days_in_4_years = days_in_1_year * 4 + 1;
constexpr int days_in_100_years = days_in_4_years * 25 - 1;
constexpr int days_in_400_years = days_in_100_years * 4 + 1;

// Given the number of days |d| since 0000-01-01 (proleptic Gregorian), returns
// the Gregorian year.
constexpr int gregorian_days_from_0000_01_01_to_year(int const d) {
  // NOTE(egg): in order to extend this to the whole proleptic Gregorian
  // calendar (including d ≤ 0), we would need to use |mod| and a division
  // consistent with it.  However, in astronomy the proleptic Julian calendar is
  // used before 1582, and that is not allowed by ISO 8601, so for now let us
  // ignore the problem and assume that there are no dates before 1583-01-01.
  return CHECKING(
      d > 0,
      ((d - 1) / days_in_400_years) * 400 +
      (((d - 1) % days_in_400_years) / days_in_100_years) * 100 +
      ((((d - 1) % days_in_400_years) % days_in_100_years) / days_in_4_years) *
          4 +
      ((((d - 1) % days_in_400_years) % days_in_100_years) % days_in_4_years) /
          days_in_1_year);
}

// Given the number of days |d| since 0000-01-01 (proleptic Gregorian), returns
// the ordinal in the current Gregorian year.
constexpr int gregorian_days_from_0000_01_01_to_ordinal(int const d) {
  return CHECKING(
      d > 0,
      (((((d - 1) % days_in_400_years) % days_in_100_years) % days_in_4_years) %
       days_in_1_year) + 1);
}

// The number of days since 0000-01-01 on the first day of |year|, in the
// proleptic Gregorian calendar.
// |gregorian_days_from_0000_01_01_to_year| is a left inverse of this function.
constexpr int gregorian_days_from_0000_01_01_at_start_of_year(int const year) {
  return CHECKING(year > 0,
                  1 + (year) * 365 +
                      (year - 1) / 4 -
                      (year - 1) / 100 +
                      (year - 1) / 400);
}

// Returns the number formed by taking |end - begin| increasingly significant
// digits, starting from the digit of the (10 ** |begin|)s.
constexpr std::int64_t digit_range(std::int64_t const digits,
                                   int const begin,
                                   int const end) {
  return CHECKING(
      digits >= 0 && begin >= 0 && begin <= end,
      (begin == end)
          ? 0
          : ((begin == 0)
                 ? digit_range(digits / 10, 0, end - 1) * 10 + digits % 10
                 : digit_range(digits / 10, begin - 1, end - 1)));
}

// Returns x * 10 ** count.
constexpr std::int64_t shift_left(std::int64_t const x, int const count) {
  return CHECKING(count >= 0, count == 0 ? x : shift_left(x * 10, count - 1));
}

// Returns x / 10 ** count.
constexpr std::int64_t shift_right(std::int64_t const x, int const count) {
  return CHECKING(count >= 0, count == 0 ? x : shift_right(x / 10, count - 1));
}

// Returns |date| advanced by the specified number of |days|. The result must be
// in the same year.
constexpr Date add_days_within_year(Date const& date, int const days) {
  return CHECKING(
      days >= 0,
      days == 0
          ? date
          : (date.day() + days > month_length(date.year(), date.month())
                 ? CHECKING(
                       date.month() <= 11,
                       add_days_within_year(
                           Date::Calendar(date.year(), date.month() + 1, 1),
                           days - month_length(date.year(), date.month()) +
                               date.day() - 1))
                 : Date::Calendar(date.year(),
                                  date.month(),
                                  date.day() + days)));
}

// The |day|th day of some |year|.  The resulting date need not be in |year|.
constexpr Date arbitrary_ordinal(int const year, int const day) {
  return Date::Ordinal(
      gregorian_days_from_0000_01_01_to_year(
          gregorian_days_from_0000_01_01_at_start_of_year(year) + day - 1),
      gregorian_days_from_0000_01_01_to_ordinal(
          (gregorian_days_from_0000_01_01_at_start_of_year(year) + day - 1)));
}

// Implementation of class |Date|.

constexpr Date Date::YYYYMMDD(std::int64_t const digits) {
  return CHECKING(digits >= 0 && digits <= 9999'99'99,
                  Date::Calendar(digit_range(digits, 4, 8),
                                 digit_range(digits, 2, 4),
                                 digit_range(digits, 0, 2)));
}

constexpr Date Date::YYYYDDD(std::int64_t const digits) {
  return CHECKING(
      digits >= 0 && digits <= 9999'999,
      Date::Ordinal(digit_range(digits, 3, 7), digit_range(digits, 0, 3)));
}

constexpr Date Date::YYYYwwD(std::int64_t const digits) {
  return CHECKING(digits >= 0 && digits <= 9999'99'9,
                  Date::Week(digit_range(digits, 3, 7),
                             digit_range(digits, 1, 3),
                             digit_range(digits, 0, 1)));
}

constexpr Date Date::Calendar(int const year, int const month, int const day) {
  return CHECKING(year >= 1583 && year <= 9999 && month >= 1 && month <= 12 &&
                  day >= 1 && day <= month_length(year, month),
                  Date(year, month, day));
}

constexpr Date Date::Ordinal(int const year, int const day) {
  return CHECKING(
      day >= 1 && day <= gregorian_year_length(year),
      add_days_within_year(Date::Calendar(year, 1, 1), day - 1));
}

constexpr Date Date::Week(int const year, int const week, int const day) {
  return CHECKING(
      week >= 1 && week <= number_of_weeks_in_year(year) &&
      day >= 1 && day <= 7,
      arbitrary_ordinal(year,
                        (week - 1) * 7 + day - 1 + ordinal_of_w_01_1(year)));
}

constexpr int Date::year() const {
  return year_;
}

constexpr int Date::month() const {
  return month_;
}

constexpr int Date::day() const {
  return day_;
}

constexpr int Date::ordinal() const {
  return day_ > 1 ? (day_ - 1) + Date(year_, month_, 1).ordinal()
                  : month_ > 1
                        ? month_length(year_, month_ - 1) +
                              Date(year_, month_ - 1, 1).ordinal()
                        : 1;
}

constexpr int Date::mjd() const {
  return gregorian_days_from_0000_01_01_at_start_of_year(year_) + ordinal() -
         (gregorian_days_from_0000_01_01_at_start_of_year(mjd0_yyyy) +
          Date::YYYYMMDD(mjd0_yyyymmdd).ordinal());
}

constexpr Date Date::next_day() const {
  return day_ == month_length(year_, month_)
             ? month_ == 12 ? Date(year_ + 1, 1, 1) : Date(year_, month_ + 1, 1)
             : Date(year_, month_, day_ + 1);
}

constexpr Date::Date(int const year,
                     int const month,
                     int const day)
      : year_(year),
        month_(month),
        day_(day) {}

// Implementation of class Time.

constexpr Time Time::hhmmss_ms(int const hhmmss, int ms) {
  return CHECKING(hhmmss >= 0 && hhmmss <= 99'99'99,
                  Time(digit_range(hhmmss, 4, 6),
                       digit_range(hhmmss, 2, 4),
                       digit_range(hhmmss, 0, 2),
                       ms).checked());
}

constexpr int Time::hour() const {
  return hour_;
}

constexpr int Time::minute() const {
  return minute_;
}

constexpr int Time::second() const {
  return second_;
}

constexpr int Time::millisecond() const {
  return millisecond_;
}

constexpr bool Time::is_leap_second() const {
  return second_ == 60;
}

constexpr bool Time::is_end_of_day() const {
  return hour_ == 24;
}

// Note that millisecond == 1000 can happen for JD and MJD because of rounding.
constexpr Time::Time(int const hour,
                     int const minute,
                     int const second,
                     int const millisecond)
    : hour_(millisecond == 1000 && second == 59 && minute == 59
                ? hour + 1
                : hour),
      minute_(millisecond == 1000 && second == 59
                  ? minute == 59
                        ? 0
                        : minute + 1
                  : minute),
      second_(millisecond == 1000
                  ? second == 59
                        ? 0
                        : second + 1
                  : second),
      millisecond_(millisecond % 1000) {}

constexpr Time const& Time::checked() const {
  return CHECKING(
      (hour_ == 24 && minute_ == 0 && second_ == 0 && millisecond_ == 0) ||
          ((millisecond_ >= 0 && millisecond_ <= 999) &&
           ((hour_ == 23 && minute_ == 59 && second_ == 60) ||
            (hour_ >= 0 && hour_ <= 23 && minute_ >= 0 && minute_ <= 59 &&
             second_ >= 0 && second_ <= 59))),
      *this);
}

// Implementation of class DateTime.

constexpr DateTime DateTime::BeginningOfDay(Date const & date) {
  return DateTime(date, Time::hhmmss_ms(00'00'00, 0), /*jd=*/false);
}

constexpr Date const& DateTime::date() const {
  return date_;
}

constexpr Time const& DateTime::time() const {
  return time_;
}

constexpr bool DateTime::jd() const {
  return jd_;
}

constexpr DateTime DateTime::normalized_end_of_day() const {
  return time_.is_end_of_day() ? BeginningOfDay(date_.next_day()) : *this;
}

constexpr DateTime::DateTime(Date const date, Time const time, bool const jd)
    : date_(date),
      time_(time),
      jd_(jd) {}

constexpr DateTime const& DateTime::checked() const {
  return CHECKING(!time_.is_leap_second() ||
                      date_.day() == month_length(date_.year(), date_.month()),
                  *this);
}

// Parsing utilities.

constexpr bool contains(char const* str, std::size_t size, char const c) {
  return size > 0 && (str[0] == c || contains(str + 1, size - 1, c));
}

constexpr int index_of(char const* str, std::size_t size, char const c) {
  return CHECKING(size > 0,
                  str[0] == c ? 0 : (index_of(str + 1, size - 1, c) + 1));
}

class CStringIterator final {
 public:
  constexpr CStringIterator(char const* str, std::size_t size);

  constexpr bool at_end() const;
  constexpr CStringIterator next() const;
  constexpr int index() const;
  constexpr char const& operator*() const;

 private:
  constexpr CStringIterator(char const* str, char const* end, char const* it);

  char const* const str_;
  char const* const end_;
  char const* const it_;
};

constexpr CStringIterator::CStringIterator(char const* str, std::size_t size)
    : str_(str),
      end_(str + size),
      it_(str) {}

constexpr bool CStringIterator::at_end() const {
  return it_ == end_;
}

constexpr CStringIterator CStringIterator::next() const {
  return CHECKING(!at_end(), CStringIterator(str_, end_, it_ + 1));
}

constexpr int CStringIterator::index() const {
  return it_ - str_;
}

constexpr char const& CStringIterator::operator*() const {
  return CHECKING(!at_end(), *it_);
}

constexpr CStringIterator::CStringIterator(char const* str,
                                           char const* end,
                                           char const* it)
    : str_(str),
      end_(end),
      it_(it) {}

// Date parsing.

// A |DateParser| contains information about a string necessary to interpret it
// as a date representation.
class DateParser final {
 public:
  // Returns a |Date| corresponding to the representation |str|.
  // Fails unless |str| is a date representation of one of the following forms:
  // [YYYY-MM-DD], [YYYYMMDD], [YYYY-Www-D], [YYYYWwwD], [YYYY-DDD], [YYYYDDD].
  static constexpr Date ParseISO(char const* str, std::size_t size);

  // |integer| is the integer part of the date, |ffd| the first fractional
  // digit.
  static constexpr Date ParseJD(char const* integer,
                                char ffd,
                                std::size_t size);

  // The input must be the integer part of the date.
  static constexpr Date ParseMJD(char const* str, std::size_t size);

 private:
  constexpr DateParser(std::int64_t digits,
                       int digit_count,
                       int hyphens,
                       int first_hyphen_index,
                       int second_hyphen_index,
                       bool has_w,
                       int w_index);

  // Returns a |DateParser| describing the given string. Fails if the string
  // does not exclusively consist of:
  //   - any number of decimal digits;
  //   - at most two hyphens;
  //   - at most one 'W'.
  static constexpr DateParser ReadToEnd(char const* str, std::size_t size);
  static constexpr DateParser ReadToEnd(CStringIterator str,
                                        std::int64_t digits,
                                        int digit_count,
                                        int colons,
                                        int first_hyphen_index,
                                        int second_hyphen_index,
                                        bool has_w,
                                        int w_index);

  // Returns a |Date| corresponding to the string that |*this| describes.
  // Fails if the format is invalid or the string represents an invalid date.
  constexpr Date ISOToDate() const;
  constexpr Date JDToDate(char ffd) const;
  constexpr Date MJDToDate() const;

  // The number formed by all digits in the string.
  std::int64_t const digits_;
  // The number of digits.
  int const digit_count_;
  // The number of hyphens.
  int const hyphens_;
  // The index of the first hyphen.
  int const first_hyphen_index_;
  // The index of the second hyphen.
  int const second_hyphen_index_;
  // Whether the string contains a W.
  bool const has_w_;
  // The index of the W.
  int const w_index_;
};

constexpr Date DateParser::ParseISO(char const* const str,
                                    std::size_t const size) {
  return ReadToEnd(str, size).ISOToDate();
}

constexpr Date DateParser::ParseJD(char const* const integer,
                                   char const ffd,
                                   std::size_t size) {
  return ReadToEnd(integer, size).JDToDate(ffd);
}

constexpr Date DateParser::ParseMJD(char const* str, std::size_t size) {
  return ReadToEnd(str, size).MJDToDate();
}

constexpr DateParser::DateParser(std::int64_t const digits,
                                 int const digit_count,
                                 int const hyphens,
                                 int const first_hyphen_index,
                                 int const second_hyphen_index,
                                 bool const has_w,
                                 int const w_index)
    : digits_(digits),
      digit_count_(digit_count),
      hyphens_(hyphens),
      first_hyphen_index_(first_hyphen_index),
      second_hyphen_index_(second_hyphen_index),
      has_w_(has_w),
      w_index_(w_index) {}

constexpr DateParser DateParser::ReadToEnd(char const* str, std::size_t size) {
  return ReadToEnd(CStringIterator(str, size),
                   /*digits=*/0,
                   /*digit_count=*/0,
                   /*hyphens=*/0,
                   /*first_hyphen_index=*/-1,
                   /*second_hyphen_index=*/-1,
                   /*has_w=*/false,
                   /*w_index=*/-1);
}

constexpr DateParser DateParser::ReadToEnd(CStringIterator const str,
                                           std::int64_t const digits,
                                           int const digit_count,
                                           int const hyphens,
                                           int const first_hyphen_index,
                                           int const second_hyphen_index,
                                           bool const has_w,
                                           int const w_index) {
  return str.at_end()
             ? DateParser{digits,
                          digit_count,
                          hyphens,
                          first_hyphen_index,
                          second_hyphen_index,
                          has_w,
                          w_index}
             : *str == '-'
                   ? CHECKING(
                         hyphens < 2,
                         hyphens == 0
                             ? ReadToEnd(str.next(),
                                         digits,
                                         digit_count,
                                         hyphens + 1,
                                         /*first_hyphen_index=*/str.index(),
                                         second_hyphen_index,
                                         has_w,
                                         w_index)
                             : ReadToEnd(str.next(),
                                         digits,
                                         digit_count,
                                         hyphens + 1,
                                         first_hyphen_index,
                                         /*second_hyphen_index=*/str.index(),
                                         has_w,
                                         w_index))
                   : *str == 'W' ? CHECKING(!has_w,
                                            ReadToEnd(str.next(),
                                                      digits,
                                                      digit_count,
                                                      hyphens,
                                                      first_hyphen_index,
                                                      second_hyphen_index,
                                                      /*has_w=*/true,
                                                      /*w_index=*/str.index()))
                                 : CHECKING(*str >= '0' && *str <= '9',
                                            ReadToEnd(str.next(),
                                                      digits * 10 + *str - '0',
                                                      digit_count + 1,
                                                      hyphens,
                                                      first_hyphen_index,
                                                      second_hyphen_index,
                                                      has_w,
                                                      w_index));
}

constexpr Date DateParser::ISOToDate() const {
  return digit_count_ == 8
             ? CHECKING(hyphens_ == 0 ||
                            (hyphens_ == 2 && first_hyphen_index_ == 4 &&
                             second_hyphen_index_ == 7),
                        Date::YYYYMMDD(digits_))
             : CHECKING(
                   digit_count_ == 7,
                   has_w_
                       ? CHECKING(
                             (hyphens_ == 0 && w_index_ == 4) ||
                                 (hyphens_ == 2 && first_hyphen_index_ == 4 &&
                                  w_index_ == 5 && second_hyphen_index_ == 8),
                             Date::YYYYwwD(digits_))
                       : CHECKING(hyphens_ == 0 || (hyphens_ == 1 &&
                                                    first_hyphen_index_ == 4),
                                  Date::YYYYDDD(digits_)));
}

constexpr Date DateParser::JDToDate(char const ffd) const {
  return CHECKING(hyphens_ == 0 &&
                  !has_w_ &&
                  ffd >= '0' && ffd <= '9',
                  ffd >= '5'
                      ? arbitrary_ordinal(
                            mjd0_yyyy,
                            Date::YYYYMMDD(mjd0_yyyymmdd).ordinal() +
                                (digits_ - mjd0_jd0_offset))
                      : arbitrary_ordinal(
                            mjd0_yyyy,
                            Date::YYYYMMDD(mjd0_yyyymmdd).ordinal() +
                                (digits_ - mjd0_jd0_offset - 1)));
}

constexpr Date DateParser::MJDToDate() const {
  return CHECKING(hyphens_ == 0 && !has_w_,
                  arbitrary_ordinal(
                      mjd0_yyyy,
                      Date::YYYYMMDD(mjd0_yyyymmdd).ordinal() + digits_));
}

// Time parsing.

// A |TimeParser| contains information about a string necessary to interpret it
// as a time representation.
class TimeParser final {
 public:
  // Returns a |Time| corresponding to the representation |str|.
  // Fails unless |str| is a valid time representation of one of the following
  // forms: [hh:mm:ss], [hhmmss], [hh:mm:ss.ss̲], [hh:mm:ss,ss̲], [hhmmss.ss̲],
  // [hhmmss,ss̲], with at most three digits after the decimal mark.
  static constexpr Time ParseISO(char const* str, std::size_t size);

  // The input must be the fractional part of the date, without the period.
  static constexpr Time ParseJD(char const* str, std::size_t size);

  // The input must be the fractional part of the date, without the period.
  static constexpr Time ParseMJD(char const* str, std::size_t size);

 private:
  constexpr TimeParser(std::int64_t digits,
                       int digit_count,
                       int colons,
                       int first_colon_index,
                       int second_colon_index,
                       bool has_decimal_mark,
                       int decimal_mark_index);

  // Returns a |TimeParser| describing the given string. Fails if the string
  // does not exclusively consist of:
  // Fails if the string does not exclusively consist of:
  //   - any number of decimal digits;
  //   - at most two colons;
  //   - at most one decimal mark ('.' or ',').
  static constexpr TimeParser ReadToEnd(char const* str, std::size_t size);
  static constexpr TimeParser ReadToEnd(CStringIterator str,
                                        std::int64_t digits,
                                        int digit_count,
                                        int colons,
                                        int first_colon_index,
                                        int second_colon_index,
                                        bool has_decimal_mark,
                                        int decimal_mark_index);

  // Returns a |Time| corresponding to the string that |*this| describes.
  // Fails if the format is invalid or the string represents an invalid time.
  constexpr Time ISOToTime() const;
  constexpr Time JDToTime() const;
  constexpr Time MJDToTime() const;

  static constexpr Time JDFractionToTime(std::int64_t digits,
                                         int digit_count);
  static constexpr int JDRoundedMilliseconds(std::int64_t digits,
                                             int digit_count);

  // The number formed by all digits in the string.
  std::int64_t const digits_;
  // The number of digits.
  int const digit_count_;
  // The number of colons.
  int const colons_;
  // The index of the first colon.
  int const first_colon_index_;
  // The index of the second colon.
  int const second_colon_index_;
  // Whether the string contains a decimal mark.
  bool const has_decimal_mark_;
  // The index of the decimal mark.
  int const decimal_mark_index_;
};

constexpr Time TimeParser::ParseISO(char const* str, std::size_t size) {
  return ReadToEnd(str, size).ISOToTime();
}

constexpr Time TimeParser::ParseJD(char const* str, std::size_t size) {
  return ReadToEnd(str, size).JDToTime();
}

constexpr Time TimeParser::ParseMJD(char const* str, std::size_t size) {
  return ReadToEnd(str, size).MJDToTime();
}

constexpr TimeParser::TimeParser(std::int64_t const digits,
                                 int const digit_count,
                                 int const colons,
                                 int const first_colon_index,
                                 int const second_colon_index,
                                 bool const has_decimal_mark,
                                 int const decimal_mark_index)
    : digits_(digits),
      digit_count_(digit_count),
      colons_(colons),
      first_colon_index_(first_colon_index),
      second_colon_index_(second_colon_index),
      has_decimal_mark_(has_decimal_mark),
      decimal_mark_index_(decimal_mark_index) {}

constexpr TimeParser TimeParser::ReadToEnd(char const* str, std::size_t size) {
  return ReadToEnd(CStringIterator(str, size),
                   /*digits=*/0,
                   /*digit_count*/ 0,
                   /*colons=*/0,
                   /*first_colon_index=*/-1,
                   /*second_colon_index=*/-1,
                   /*has_decimal_mark=*/false,
                   /*decimal_mark_index=*/-1);
}

constexpr TimeParser TimeParser::ReadToEnd(CStringIterator const str,
                                           std::int64_t const digits,
                                           int const digit_count,
                                           int const colons,
                                           int const first_colon_index,
                                           int const second_colon_index,
                                           bool const has_decimal_mark,
                                           int const decimal_mark_index) {
  return str.at_end()
             ? TimeParser(digits,
                          digit_count,
                          colons,
                          first_colon_index,
                          second_colon_index,
                          has_decimal_mark,
                          decimal_mark_index)
             : *str == ':'
                   ? CHECKING(
                         colons < 2,
                         colons == 0
                             ? ReadToEnd(str.next(),
                                         digits,
                                         digit_count,
                                         colons + 1,
                                         /*first_colon_index=*/str.index(),
                                         second_colon_index,
                                         has_decimal_mark,
                                         decimal_mark_index)
                             : ReadToEnd(str.next(),
                                         digits,
                                         digit_count,
                                         colons + 1,
                                         first_colon_index,
                                         /*second_colon_index=*/str.index(),
                                         has_decimal_mark,
                                         decimal_mark_index))
                   : *str == ',' || *str == '.'
                         ? CHECKING(
                               !has_decimal_mark,
                               ReadToEnd(str.next(),
                                         digits,
                                         digit_count,
                                         colons,
                                         first_colon_index,
                                         second_colon_index,
                                         /*has_decimal_mark=*/true,
                                         /*decimal_mark_index=*/str.index()))
                         : CHECKING(*str >= '0' && *str <= '9',
                                    ReadToEnd(str.next(),
                                              digits * 10 + *str - '0',
                                              digit_count + 1,
                                              colons,
                                              first_colon_index,
                                              second_colon_index,
                                              has_decimal_mark,
                                              decimal_mark_index));
}

constexpr Time TimeParser::ISOToTime() const {
  return CHECKING(
      digit_count_ >= 6 &&
          (colons_ == 0 || (colons_ == 2 && first_colon_index_ == 2 &&
                            second_colon_index_ == 5)) &&
          ((digit_count_ == 6 && !has_decimal_mark_) ||
           (has_decimal_mark_ &&
            ((colons_ == 0 && decimal_mark_index_ == 6) ||
             (colons_ != 0 && decimal_mark_index_ == 8)))) &&
          digit_count_ <= 15,
      Time::hhmmss_ms(digit_range(digits_, digit_count_ - 6, digit_count_),
                      shift_left(digit_range(digits_, 0, digit_count_ - 6),
                                 3 - (digit_count_ - 6))));
}

constexpr Time TimeParser::JDToTime() const {
  // Computation example: JD2455200.6234567
  //   digits_ = 6'234'567, digit_count_ = 7
  //   digit_range(6'234'567, 6, 7) = 6
  //   shift_left(6 - 1, 6) = 1'000'000
  //   digit_range(6'234'567, 0, 6) = 234'567
  //   calls JDFractionToTime(1'234'567, 7)
  // Computation example: JD2455201.25000
  //   digits_ = 25'000, digit_count = 5
  //   digit_range(25'000, 4, 5) = 2
  //   shift_left(2 + 5, 4) = 70'000
  //   digit_range(25'000, 0, 4) = 5'000
  //   calls JDFractionToTime(75'000, 5)
  return CHECKING(
      colons_ == 0 && !has_decimal_mark_,
      digit_range(digits_, digit_count_ - 1, digit_count_) >= 5
          ? JDFractionToTime(
                shift_left(
                    digit_range(digits_, digit_count_ - 1, digit_count_) - 5,
                    digit_count_ - 1) +
                digit_range(digits_, 0, digit_count_ - 1),
                digit_count_)
          : JDFractionToTime(
                shift_left(
                    digit_range(digits_, digit_count_ - 1, digit_count_) + 5,
                    digit_count_ - 1) +
                digit_range(digits_, 0, digit_count_ - 1),
                digit_count_));
}

constexpr Time TimeParser::MJDToTime() const {
  return CHECKING(colons_ == 0 && !has_decimal_mark_,
                  JDFractionToTime(digits_, digit_count_));
}

constexpr Time TimeParser::JDFractionToTime(std::int64_t const digits,
                                            int const digit_count) {
  // Computation example: MJD55200.1234567
  //   digits = 1'234'567, digit_count = 7
  //   24 * digits = 29'629'608
  //   digit_range(29'629'608, 7, 9) = 2
  //   digit_range(29'629'608, 0, 7) = 9'629'608
  //   60 * 9'629'608 = 577'776'480
  //   digit_range(577'776'480, 7, 9) = 57
  //   digit_range(577'776'480, 0, 7) = 7'776'480
  //   60 * 24 * digits = 1'777'776'480
  //   digit_range(1'777'776'480, 0, 7) = 7'776'480
  //   60 * 7'776'480 = 466'588'800
  //   digit_range(466'588'800, 7, 9) = 46
  //   60 * 60 * 24 * digits = 106'666'588'800
  //   digit_range(106'666'588'800, 3, 7) = 6588
  //   (6588 + 5) / 10 = 659
  // Note that this results in rounding to the nearest millisecond.
  return CHECKING(
      digit_count <= 14,
      Time(digit_range(24 * digits, digit_count, digit_count + 2),
           digit_range(60 * digit_range(24 * digits, 0, digit_count),
                       digit_count, digit_count + 2),
           digit_range(60 * digit_range(60 * 24 * digits, 0, digit_count),
                       digit_count, digit_count + 2),
           JDRoundedMilliseconds(digits, digit_count)).checked());
}

constexpr int TimeParser::JDRoundedMilliseconds(std::int64_t const digits,
                                                int const digit_count) {
  // Because 86'400 is a multiple of 100, fractions with less than 2 digits
  // after the period don't contribute to the milliseconds.  No rounding is
  // necessary if there are less than 5 digits after the period.
  return digit_count <= 2
             ? 0
             : digit_count <= 5
                   ? digit_range(60 * 60 * 24 * digits, 0, 3)
                   : (digit_range(60 * 60 * 24 * digits,
                                  digit_count - 4, digit_count) + 5) / 10;
}

// Operators.

constexpr bool operator==(Date const& left, Date const& right) {
  return left.year() == right.year() &&
         left.month() == right.month() &&
         left.day() == right.day();
}

constexpr bool operator!=(Date const& left, Date const& right) {
  return !(left == right);
}

constexpr bool operator<(Date const& left, Date const& right) {
  return left.year() < right.year() ||
         (left.year() == right.year() &&
          (left.month() < right.month() ||
           (left.month() == right.month() && left.day() < right.day())));
}

constexpr bool operator>(Date const& left, Date const& right) {
  return right < left;
}

constexpr bool operator<=(Date const& left, Date const& right) {
  return !(right < left);
}

constexpr bool operator>=(Date const& left, Date const& right) {
  return !(left < right);
}

constexpr Date operator""_Date(char const* str, std::size_t size) {
  return DateParser::ParseISO(str, size);
}

constexpr bool operator==(Time const& left, Time const& right) {
  return left.hour() == right.hour() &&
         left.minute() == right.minute() &&
         left.second() == right.second() &&
         left.millisecond() == right.millisecond();
}

constexpr bool operator!=(Time const& left, Time const& right) {
  return !(left == right);
}

constexpr Time operator""_Time(char const* str, std::size_t size) {
  return TimeParser::ParseISO(str, size);
}

constexpr bool operator==(DateTime const& left, DateTime const& right) {
  return left.normalized_end_of_day().date() ==
             right.normalized_end_of_day().date() &&
         left.normalized_end_of_day().time() ==
             right.normalized_end_of_day().time();
}

constexpr bool operator!=(DateTime const& left, DateTime const& right) {
  return !(left == right);
}

constexpr DateTime operator""_DateTime(char const* str, std::size_t size) {
  // Given correctness of the date and time parts of the string, this check
  // ensures that either both are in basic format or both are in extended
  // format.
  return CHECKING(
      contains(str, size, '-') == contains(str, size, ':'),
      size >= 2 && str[0] == 'J' && str[1] == 'D'
          ? contains(str, size - 1, '.')
                ? DateTime(DateParser::ParseJD(
                               str + 2,
                               str[index_of(str + 2, size - 2, '.') + 3],
                               index_of(str + 2, size - 2, '.')),
                           TimeParser::ParseJD(
                               str + index_of(str, size, '.') + 1,
                               size - (index_of(str, size, '.') + 1)),
                           /*jd=*/true)
                : DateTime(DateParser::ParseJD(str + 2, '0', size - 2),
                           TimeParser::ParseJD("0", 1),
                           /*jd=*/true)
          : size >= 3 && str[0] == 'M' && str[1] == 'J' && str[2] == 'D'
                ? contains(str, size - 1, '.')
                      ? DateTime(DateParser::ParseMJD(
                                     str + 3,
                                     index_of(str + 3, size - 3, '.')),
                                 TimeParser::ParseMJD(
                                     str + index_of(str, size, '.') + 1,
                                     size - (index_of(str, size, '.') + 1)),
                                 /*jd=*/true)
                      : DateTime(DateParser::ParseMJD(str + 3, size - 3),
                                 TimeParser::ParseMJD("0", 1),
                                 /*jd=*/true)
                : DateTime(DateParser::ParseISO(str, index_of(str, size, 'T')),
                           TimeParser::ParseISO(
                               str + index_of(str, size, 'T') + 1,
                               size - (index_of(str, size, 'T') + 1)),
                           /*jd=*/false)
                      .checked());
}

}  // namespace internal_date_time
}  // namespace date_time
}  // namespace astronomy
}  // namespace principia
