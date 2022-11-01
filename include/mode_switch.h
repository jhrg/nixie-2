
#define MODE_SWITCH 3 // D3
#define INPUT_SWITCH 4  // D4

void timed_mode_switch_push();
void timed_mode_switch_release();
void input_switch_event();

    enum main_mode {
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