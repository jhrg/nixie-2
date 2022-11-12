
#define MODE_SWITCH 3  // D3
#define INPUT_SWITCH 4 // D4

void mode_switch_push();
void mode_switch_release();
void input_switch_push();
void input_switch_release();

void set_time_mode_handler();

enum modes {
    main,
    set_time,
    set_config
};

enum main_modes {
    show_time,
    show_date,
    show_weather
};

enum set_time_modes {
    set_hours,
    set_minutes,
    zero_seconds
};
