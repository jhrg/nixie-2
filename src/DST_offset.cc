// From: https://forum.arduino.cc/t/ds3231-rtc-daylight-savings-time/410699
// Code by "odometer" ca. Oct. 2016

/**
 * Return true if DST is now in effect, otherwise return false.
 */
bool is_dst(int year, int month, int dayOfMonth, int hour) {
    bool DST = false;

    // ********************* Calculate offset for Sunday *********************
    int y = year;                 // DS3231 uses two digit year (required here)
    int x = (y + y / 4 + 2) % 7;  // remainder will identify which day of month
    // is Sunday by subtracting x from the one
    // or two week window. First two weeks for March
    // and first week for November

    // *********** Test DST: BEGINS on 2nd Sunday of March @ 2:00 AM *********
    if (month == 3 && dayOfMonth == (14 - x) && hour >= 2) {
        DST = true;  // Daylight Savings Time is TRUE (add one hour)
    }
    if ((month == 3 && dayOfMonth > (14 - x)) || month > 3) {
        DST = true;
    }

    // ************* Test DST: ENDS on 1st Sunday of Nov @ 2:00 AM ************
    if (month == 11 && dayOfMonth == (7 - x) && hour >= 2) {
        DST = false;  // daylight savings time is FALSE (Standard time)
    }
    if ((month == 11 && dayOfMonth > (7 - x)) || month > 11 || month < 3) {
        DST = false;
    }

    return DST;
}
