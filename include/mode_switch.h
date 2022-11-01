
#define MODE_SWITCH 3 // D3

void timed_mode_switch_push();
void timed_mode_switch_release();

enum main_mode
{
    show_time,
    show_weather,
    set_time
};

enum set_time_mode
{
    set_hours,
    adv_hours_slow, // no fast advance for hours

    set_minutes,
    adv_minutes_slow,
    adv_minutes_fast,

    zero_seconds
};
