
#define MODE_SWITCH 5  // D5
#define INPUT_SWITCH 4 // D4

void mode_switch_push();
void mode_switch_release();
void input_switch_push();
void input_switch_release();

void set_date_time_mode_handler();

enum modes {
    main,
    set_date_time,
    set_config
};

enum main_modes {
    show_time,
    show_date,
    show_weather
};

enum set_time_modes {
    set_year,
    set_month,
    set_day,
    set_hour,
    set_minute,
    zero_seconds
};
