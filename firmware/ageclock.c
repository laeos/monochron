#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "util.h"
#include "ratt.h"
#include "ks0108.h"
#include "glcd.h"

#ifdef AGECLOCK

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t date_m, date_d, date_y;

struct sdate {
    uint32_t y;
    uint32_t m;
    uint32_t d;
};

struct age {
    uint32_t years;
    uint32_t months;
    uint32_t days;

    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
};

#include "demo.h"

static struct age age;
static uint32_t calstart;

/* convert date to day number */
static uint32_t gday(const struct sdate *d)
{
    uint32_t  y, m;
    m = (d->m + 9)%12;      /* mar=0, feb=11 */
    y = d->y - m/10;        /* if Jan/Feb, year-- */
    return y*365 + y/4 - y/100 + y/400 + (m*306 + 5)/10 + (d->d - 1);
}

/* convert day number to y,m,d format */
static void dtf(uint32_t od, struct sdate *pd)
{
    int64_t y, ddd, mi, d = od;

    y = (10000UL*d + 14780UL)/3652425UL;
    ddd = d - (y*365 + y/4 - y/100 + y/400);
    if (ddd < 0) {
        y--;
        ddd = d - (y*365 + y/4 - y/100 + y/400);
    }
    mi = (52 + 100*ddd)/3060;
    pd->y = y + (mi + 2)/12;
    pd->m = (mi + 2)%12 + 1;
    pd->d = ddd - (mi*306 + 5)/10 + 1;
}

static void setup_calc(void)
{
    struct sdate s = {
        .y = 1582,
        .m = 10,
        .d =1
    };
    calstart = gday(&s);
}

/* return gday, or exit if bad date */
static bool legald(const struct sdate *d, uint32_t *v)
{
    struct sdate t;
    uint32_t g;

    g = gday(d);
    if (g < calstart)
	return false;

    dtf(g, &t);
    if (d->y == t.y && d->m == t.m && d->d == t.d) {
	*v = g;
	return true;
    }

    return false;
}

static uint32_t get_today(void)
{
    struct sdate today;

    today.y = 2000 + date_y;
    today.m = date_m;
    today.d = date_d;

    return gday(&today);
}

static void normalize(uint32_t days)
{
    age.years = days / 365;
    days -= age.years * 365;

    age.months = trunc(days / 30.42);
    days -= trunc(age.months * 30.42);

    age.days = days;
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


static bool around(uint32_t digit, uint32_t *value, bool printed)
{
    if (*value >= digit) {
	lputc(*value / digit + '0');
	*value = *value % digit;
	return true;
    } else if (printed) {
	lputc('0');
	return true;
    }
    return false;
}


static void lputn(uint32_t n)
{
    bool did = false;
    did = around(100000, &n, did);
    did = around(10000, &n, did);
    did = around(1000, &n, did);
    did = around(100, &n, did);
    did = around(10, &n, did);
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


void initanim(void)
{
    setup_calc();
}

void initdisplay(uint8_t inverted)
{
    glcdClearScreen();
}

void step(void)
{
    uint32_t diff, past, today = get_today();

    if (legald(&the_birth, &past)) {
	diff = (past > today) ? past - today : today - past;
	normalize(diff);
	age.hours = time_h;
	age.minutes = time_m;
	age.seconds = time_s;
    } 
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

    if ((the_birth.m == date_m) &&
	    (the_birth.d  == date_d))
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
