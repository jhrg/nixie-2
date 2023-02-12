
#define MODE_SWITCH 5  // D5
#define INPUT_SWITCH 4 // D4
#define INPUT_SWITCH_PORT PD4  // Used for faster reads

void set_date_time_mode_handler();

enum modes {
    main,
    set_date_time,
    set_config  // set_config not yet implemented
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

enum switch_press_duration {
    none,
    quick,     // momentary
    medium,    // 2s
    very_long  // 5s
};

void mode_switch_push();
void mode_switch_release();
void input_switch_push();
void input_switch_release();

bool poll_mode_button();
void process_mode_switch_press();

bool poll_input_button();
bool input_switch_held_down();
void process_input_switch_press();
void process_input_switch_held();
