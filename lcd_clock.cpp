/**
 * @file lcd_clock.cpp
 *
 * @brief This cpp file was made within Keil
 * Studio Cloud and commented inside of CLion.
 * This program utilizes the mbed OS and the
 * TextLCD header file for 4-bit interface
 * with the HD44780 LCD display controller
 *
 * @author Levi Vande Kerkhoff
 *
 */

#include "mbed.h"
#include "TextLCD.h"
#include <string>

/**
 * @brief Initializes the lcd object using
 * the TextLCD library.
 *
 * The pins on the LCD screen that correspond
 * to the output pins on the Nucleoboard are
 * as follows:
 *
 * TextLCD lcd(RS, E, D4, D5, D6, D7);
 *
 */
TextLCD lcd(PA_0, PA_1, PA_4, PB_0, PC_1, PC_0);

/**
 * @brief This instantiates the temperature
 * sensor analog input pin.
 *
 */
AnalogIn temp_sensor(PC_2);

/**
 * @brief DigitalOut instantiates the GPIO pin objects that
 * read the rows of the 4x4 keypad
 *
 * They are set as output pins.
 */
DigitalOut rows[] = {(PA_6), (PA_7), (PB_6), (PC_7)}; // array of GPIO pin objects -- set to out

/**
 * @brief DigitalIn instantiates the GPIO pin objects that
 * read the columns of the keypad.
 *
 * They are input pins.
 */
DigitalIn cols[] = {(PA_9), (PA_8), (PB_10), (PB_4)}; // array of GPIO pin objects -- set to in

/**
 * @brief This instantiates the interrupt used to toggle the
 * temperature unit shown on the LCD.
 */
InterruptIn button(PC_13, PullUp);

/** Array of keypad values for user input */
char key_map [4][4] = {
        {'1', '2', '3', 'A'}, //1st row
        {'4', '5', '6', 'P'}, //2nd row
        {'7', '8', '9', 'M'}, //3rd row
        {'*', '0', '#', 'D'}, //4th row
};


/**
 * @brief Scans the columns of the keypad.
 *
 * This checks to see if the input pins are low (if
 * a button is pressed).
 *
 * @return Index of a button press or a "null character".
 */
int col_scan(void){
    for(int i=0; i<4; i++){
        if(cols[i].read() == 0)
            return i;
    }
    return -1;
}

/**
 * @brief Function scans the rows of the keypad and calls
 * the col_scan function.
 *
 * This function also acts as a button debounce when a button
 * is pressed.
 *
 * @return a null character ('x') if no keys are pressed,
 * or return the character pressed.
 */
char keypadScan(void){
    static int row_col[] = {-1, -1};
    static int last_key[] = {-1, -1};
    int delay = 4;
    bool noKeyPressed = true;
    static Timer debounceTimer;
    const int debounce_time = 500

    for(int i=0; i<4; i++){
        rows[i].write(0);
        ThisThread::sleep_for(chrono::milliseconds(delay));
        row_col[1] = col_scan();
        ThisThread::sleep_for(chrono::milliseconds(delay));
        rows[i].write(1);

        if(row_col[1] > -1){
            row_col[0] = i;
            noKeyPressed = false;
            debounceTimer.start();
            break;
        }
    }

    /** No key pressed */
    if (noKeyPressed) {
        last_key[0] = -1;
        last_key[1] = -1;
        return 'x';
    }

    /** If the same key is pressed again or the debounce time is not elapsed, return no key */
    if ((last_key[0] == row_col[0] && last_key[1] == row_col[1]) || debounceTimer.read_ms() < debounce_time) {
        return 'x';
    }

    /** Update last key and return the character corresponding to the key press */
    last_key[0] = row_col[0];
    last_key[1] = row_col[1];
    debounceTimer.reset();
    return key_map[row_col[0]][row_col[1]];
}

/** These constants act as mode macros **/
const int HOUR = 0,
          MIN = 1,
          AM_PM = 2,
          ENTER = 3;

/** These constants act as mode macros **/
const int NORMAL_MODE = 0,
          SET_MODE = 1,
          ERROR_MODE = 2;

bool toggle = 0;

/**
 * @brief This function is used in conjunction
 * with the boolean variable, toggle, for
 * controlling the temperature output unit.
 */
void temp_toggle(void){
    toggle = toggle == 0 ? 1 : 0;
}

/**
 * @brief This function reads the GPIO pin value, converts the voltage
 * value to Celsius and optionally converts further to Fahrenheit.
 *
 * @param toggle
 * @return Temperature value converted from voltage in C or F
 */
int getTemp(int toggle){
    if(!toggle)
        return int((temp_sensor.read()*3300.0)/10.0);
    return int(((temp_sensor.read()*3300.0)/10.0)*(9.0/5.0))+32;
}

int index = 0;
char current_entry[2];
/**
 * @brief This function resets the current time entries.
 */
void reset_entries(void){
    current_entry[0] = '_';
    current_entry[1] = '_';
}



/**
 * @brief Program entry point.
 *
 * The program is organized as follows:
 *      1 - SET UP
 *          - This initializes all variables and
 *            sets the GPIO settings for keypad
 *      2 - OPERATION
 *          - Keypad scan
 *          - Keypad Press Interpretation
 *          - Time Object Update
 *          - LCD UPDATE
 *
 * @return 0, This will never be returned.
 */
int main()
{
    /** SET UP SECTION */

    for(int i=0; i<4; i++){
        rows[i].write(1);
        cols[i].mode(PullUp);
    } /** settings for the GPIO pins */

    char key_map_val;

    /** This is called in start up to initialize the blank characters */
    reset_entries();

    /** Get initial temperature and initialize temp. unit characters */
    int temp = getTemp(toggle);
    char C_F[2] = {'C', 'F'};

    /** Time set up */
    time_t seconds = time(NULL);
    struct tm *timeinfo = localtime(&seconds);
    button.fall(temp_toggle);
    int hr = 0, min = 0;


    static Timer timer;
    timer.start();

    int mode = NORMAL_MODE, entry_mode = HOUR; /** initializing start mode */
    bool update_LCD = 0; /** this tells the program whether or not to update the screen */

    /** OPERATION SECTION */
    while (1) {

        /**
         * The program constantly scans for key presses
         */
        key_map_val = keypadScan();

        /**
         * This is the conditional entry point for key press entries.
         *
         * Null characters, 'x' represent no key presses.
         *
         * Key presses are not used to update entries when ERROR_MODE
         * is active. This prevents bugs/errors in operation.
         *
         * SET_MODE is activated by the '*' key. Pressing the '*' key
         * will also reset the current entries and take the user back
         * to the HOUR entry menu.
         *
         * Pressing the 'D' key at any time returns the user to
         * NORMAL operation without updating the time.
         *
         * Each entry can be checked/entered by pressing the '#' key.
         * If the entry is incorrect/out of bounds, then ERROR_MODE
         * will be entered for 2 seconds.
         *
         */
        if(key_map_val != 'x' && mode != ERROR_MODE){
            /**
             * If the '*' key is entered, the program enters SET_MODE and the screen is updated.
             * If the '*' key is pressed while in SET_MODE the entries are reset and the user is
             * returned to HOUR entry.
             */
            if(key_map_val == '*'){
                mode = SET_MODE;
                entry_mode = HOUR;
                update_LCD = 1;
                index = 0; /** Returns user entry to first entry field. */
                reset_entries();
            }
            else if(key_map_val == 'D'){
                mode = NORMAL_MODE;
                entry_mode = HOUR;
                reset_entries();
            }
            else if(mode == SET_MODE){
                if(key_map_val != '#'){
                    current_entry[index] = key_map_val;
                    index = index == 0 ? 1 : 0; /** Switches entry index location. */
                    update_LCD = 1;
                }
                else{
                    if(entry_mode == HOUR){
                        hr = (current_entry[0] - '0')*10 + (current_entry[1] - '0');
                        if(hr < 1 || hr > 12){
                            timer.reset();
                            mode = ERROR_MODE;
                        }
                        else
                            entry_mode++;
                    }
                    else if(entry_mode == MIN){
                        min = (current_entry[0] - '0')*10 + (current_entry[1] - '0');
                        if(min > 59){
                            timer.reset();
                            mode = ERROR_MODE;
                        }
                        else
                            entry_mode++;
                    }
                    else if(entry_mode == AM_PM){
                        if(current_entry[0] != 'A' && current_entry[0] != 'P' || current_entry[1] != 'M'){
                            timer.reset();
                            mode = ERROR_MODE;
                        }
                        else{
                            if(current_entry[0] == 'P' && hr != 12)
                                hr = hr + 12;
                            entry_mode++;
                        }
                    }
                    /** This ensures the screen is always updated after a key press. */
                    update_LCD = 1;
                    reset_entries();
                }
            }
        }

        /**
         * TIME UPDATE SECTION
         *
         * This section updates the RTC time and resets the mode of operation.
         *
         * */
        if(entry_mode == ENTER){
            timeinfo->tm_hour = hr; // Hour (24-hour format)
            timeinfo->tm_min = min; // Minutes
            timeinfo->tm_sec = 0; // seconds
            time_t new_time_t = mktime(timeinfo);
            set_time(new_time_t);
            mode = NORMAL_MODE; // normal
            entry_mode = HOUR;
            reset_entries();
        }


        /**
         * SCREEN UPDATE SECTION
         *
         * This section updates the LCD Screen.
         *
         * It updates once every second for the time display
         * while in NORMAL MODE.
         *
         * It updates for each keyp press while in SET_MODE.
         *
         * It updates after 2 seconds in ERROR_MODE.
         *
         */

        if((mode == NORMAL_MODE && timer.read_ms() >= 1000)){
            temp = getTemp(toggle);
            seconds = time(NULL);
            tm *timeinfo = localtime(&seconds);
            lcd.cls();
            lcd.printf("%02d:%02d:%02d %s %02d %c",
                       (timeinfo->tm_hour % 12 == 0) ? 12 : timeinfo->tm_hour % 12,
                       timeinfo->tm_min,
                       timeinfo->tm_sec,
                       (timeinfo->tm_hour > 12) ? "PM" : "AM",
                       temp,
                       C_F[toggle]);
            timer.reset();
        }
        else if(mode >= SET_MODE && update_LCD == 1){
            lcd.cls();
            if(mode == ERROR_MODE)
                lcd.printf("---- ERROR! ----");
            else if(entry_mode == HOUR)
                lcd.printf("HOUR:  %c%c", current_entry[0], current_entry[1]);
            else if(entry_mode == MIN)
                lcd.printf("MIN:  %c%c", current_entry[0], current_entry[1]);
            else if(entry_mode == AM_PM)
                lcd.printf("AM or PM:  %c%c", current_entry[0], current_entry[1]);
            update_LCD = 0;
        }
        else if(mode == ERROR_MODE && timer.read_ms() >= 2000){
            timer.reset();
            mode--;
            index = 0;
            update_LCD = 1;
        }
    }


    return 0;
}
