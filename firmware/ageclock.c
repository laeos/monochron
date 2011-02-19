#include <stdint.h>
#include <string.h>
#include <math.h>

#include "util.h"
#include "ratt.h"
#include "ks0108.h"
#include "glcd.h"

#ifdef AGECLOCK

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t date_m, date_d, date_y;

struct date {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t seconds;
};

struct age {
    uint32_t years;
    uint32_t months;
    uint32_t weeks;
    uint32_t days;
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
};

#include "demo.h"

static struct age age;

static int compare_mdhm(const struct date *a, const struct date *b)
{
    int r;

    if ((r = a->month - b->month))
	return r;
    if ((r = a->day - b->day))
	return r;
    if ((r = a->hour - b->hour))
	return r;
    if ((r = a->minute - b->minute))
	return r;
    return 0;
}

static void so_far(const struct date *today, struct age *age)
{
    age->seconds += today->seconds;
    age->minutes += today->minute;
    age->hours += today->hour;
    age->days += today->day - 1;
    age->months += today->month - 1;
}

static void diff(const struct date *birth, const struct date *today, struct age *age)
{
    memcpy(age, &start_age, sizeof(*age));
    so_far(today, age);
    age->years = today->year - (birth->year + 1);
    if (compare_mdhm(birth, today) < 0)
	age->years++;
}

static uint32_t boink(uint32_t *from, uint32_t count)
{
    uint32_t mod = *from % count;
    uint32_t rok = (*from - mod) / count;
    *from = mod;
    return rok;
}

static void normalize(struct age *age)
{
    double f, m;

    age->minutes += boink(&age->seconds, 60);
    age->hours += boink(&age->minutes, 60);
    age->days += boink(&age->hours, 24);
    age->weeks += boink(&age->days, 7);

    f = age->weeks;
    m = truncf(f / 4.3);
    age->months += m;
    age->weeks -= truncf((m * 4.3));

    age->years += boink(&age->months, 12);
}

void initanim(void)
{
}

void initdisplay(uint8_t inverted)
{
    glcdClearScreen();
}

void step(void)
{
    struct date today = {
	2000 + date_y,
	date_m,
	date_d,
	time_h,
	time_m,
	time_s
    };
    diff(&the_birth, &today, &age);
    normalize(&age);
}

#define NLINES 8
#define NCOLS  21

static char display[NLINES][NCOLS];
static char old_display[NLINES][NCOLS];
static int pos_l, pos_c;
static int valid;

static void update(void)
{
    int line, col;
    int last_line = -1;

    for (line = 0; line < NLINES; line++) {
	for (col = 0; col < NCOLS; col++) {
	    if (!valid || (display[line][col] != old_display[line][col])) {
		if (last_line != line) {
		    glcdSetYAddress(line);
		    last_line = line;
		}
		glcdSetXAddress(6 * col);
		glcdWriteChar(display[line][col], NORMAL);
	    }
	}
    }
    valid = 1;
    memcpy(old_display, display, NLINES * NCOLS);
}

static void next_line(void)
{
    pos_l++;
    pos_c = 0;
}

static void last_line(void)
{
    pos_l = NLINES - 1;
    pos_c = 0;
}

static void lputc(char ch)
{
    display[pos_l][pos_c] = ch;
    ++pos_c;
}

static void lputs(const char *str)
{
    while (*str)
	lputc(*str++);
}

static void lputn(uint8_t n)
{
    if (n >= 100) {
	lputc(n / 100 + '0');
	n = n % 100;
    }
    if (n >= 10)
	lputc(n / 10 + '0');
    lputc(n % 10 + '0');
}

static void lputn_zero(uint8_t n)
{
    if (n >= 10)
	lputc(n / 10 + '0');
    else
	lputc('0');
    lputc(n % 10 + '0');
}

static const char *dow_data[] = {
    "Sun ",
    "Mon ",
    "Tue ",
    "Wed ",
    "Thu ",
    "Fri ",
    "Sat ",
};

static void lprint_dow(uint8_t mon, uint8_t day, uint8_t yr)
{
    lputs(dow_data[dotw(mon,day,yr)]);
}

void draw(uint8_t inverted)
{
    int did = 0;

    memset(display, ' ', NLINES * NCOLS);
    pos_l = 0;
    pos_c = 0;

    lputs("Time flies, " NAME "! ");

    next_line();
    lputs("You are ");

    lputn(age.years);
    lputs(" years, ");

    next_line();

    if (age.months) {
	did++;
	lputn(age.months);
	lputs(" month");

	if (age.months != 1)
	    lputc('s');
	lputs(", ");
    }

    if (did == 2) { next_line(); did = 0; }

    if (age.weeks) {
	did++;
	lputn(age.weeks);
	lputs(" week");
	if (age.weeks != 1)
	    lputc('s');
	lputs(", ");
    }

    if (did == 2) { next_line(); did = 0; }

    if (age.days) {
	did++;
	lputn(age.days);
	lputs(" day");
	if (age.days != 1)
	    lputc('s');
	lputs(", ");
    }

    if (did == 2) { next_line(); did = 0; }

    if (age.hours) {
	did++;
	lputn(age.hours);
	lputs(" hour");
	if (age.hours != 1)
	    lputc('s');
	lputs(", ");
    }

    if (did == 2) { next_line(); did = 0; }

    if (age.minutes) {
	did++;
	lputn(age.minutes);
	lputs(" minute");
	if (age.minutes != 1)
	    lputc('s');
	lputs(", ");
    }

    if (did) { next_line(); did = 0; }

    lputs("and ");
    lputn(age.seconds);
    lputs(" second");
    if (age.seconds != 1)
	lputc('s');
    lputs(" old !! ");

    next_line();

    if ((the_birth.month == date_m) &&
	    (the_birth.day  == date_d))
	lputs("   HAPPY BIRTHDAY!   ");

    last_line();

    lprint_dow(date_m, date_d, date_y);

    lputn_zero(date_m);
    lputc('/');
    lputn_zero(date_d);
    lputc('/');
    lputn_zero(date_y);
    lputc(' '); 
    lputn(time_h);
    lputc(':');
    lputn_zero(time_m);
    lputc(':');
    lputn_zero(time_s);

    update();
}

void setscore(void)
{
}

void init_crand(void)
{
}
#endif
