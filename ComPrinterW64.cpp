//
// ComPrinter.c - This program reads text from a
//   serial port and prints it to the console.
//
// Written by Ted Burke - last updated 19-12-2014
//
// To compile with MinGW:
//
//      gcc -o ComPrinter.exe ComPrinter.c
//
// Example commands:
//
//      ComPrinter /devnum 22 /baudrate 38400
//      ComPrinter /id 12 /quiet
//      ComPrinter /baudrate 38400 /keystrokes
//      ComPrinter /charcount 5
//      ComPrinter /timeout 3000
//      ComPrinter /endchar x
//      ComPrinter /endhex FF
//
// To stop the program, press Ctrl-C (or use one of /charcount, /timeout, /endchar, /endhex)
//

#define WINVER 0x0500

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define MESSAGE_LENGTH 100

// Declare variables and structures
HANDLE hSerial = INVALID_HANDLE_VALUE;
DCB dcbSerialParams = { 0 };
COMMTIMEOUTS timeouts = { 0 };
DWORD dwBytesWritten = 0;
char dev_name[MAX_PATH] = " ";
int dev_number = -1;
int baudrate = 2400;
int scan_max = 30;
int scan_min = 1;
int simulate_keystrokes = 0;
int debug = 1; // print some info by default
int id = -1;

// variables related to terminating program
long charcount = -1;    // program exits once this number of bytes processed
long chars_read = 0;   // used to count how many bytes have been processed
int endchar = -1;      // program exits when this byte is read
ULONGLONG lastchartime = -1; // time that last character was received
DWORD timeout_ms = -1;   // program exits when no data received for this duration

void CloseSerialPort()
{
    if (hSerial != INVALID_HANDLE_VALUE)
    {
        // Close serial port
        fprintf(stderr, "\nClosing serial port...");
        if (CloseHandle(hSerial) == 0)
            fprintf(stderr, "Error\n");
        else fprintf(stderr, "OK\n");
    }
}

void exit_message(const char* error_message, int error)
{
    // Print an error message
    fprintf(stderr, error_message);
    fprintf(stderr, "\n");

    // Exit the program
    exit(error);
}

void simulate_keystroke(char c)
{
    // This structure will be used to create the keyboard
    // input event.
    INPUT ip;

    // Set up a generic keyboard event.
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0; // hardware scan code for key
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    // Press the key
    // Currently only alphabet lettes, spaces, commas and full stops.
    if (c >= 0x61 && c <= 0x7A) c -= 0x20;
    if (c >= 0x30 && c <= 0x39) ip.ki.wVk = c;
    else if (c >= 0x41 && c <= 0x5A) ip.ki.wVk = c;
    else if (c == ' ') ip.ki.wVk = VK_SPACE;
    else if (c == ',') ip.ki.wVk = VK_OEM_COMMA;
    else if (c == '.') ip.ki.wVk = VK_OEM_PERIOD;
    else if (c == '\b') ip.ki.wVk = VK_BACK;
    else if (c == '\t') ip.ki.wVk = VK_TAB;
    else if (c == '\n') ip.ki.wVk = VK_RETURN;
    else return;

    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));

    // Release the key
    ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
    SendInput(1, &ip, sizeof(INPUT));
}

int main(int argc, char* argv[])
{
    // Parse command line arguments. Available options:
    //
    // /devnum DEVICE_NUMBER
    // /baudrate BITS_PER_SECOND
    // /id ROBOT_ID_NUMBER
    // /keystrokes
    // /debug
    // /quiet
    //
    int n = 1;
    while (n < argc)
    {
        // Process next command line argument
        if (strcmp(argv[n], "/devnum") == 0)
        {
            if (++n >= argc) exit_message("Error: no device number specified", 1);

            // Set device number to specified value
            dev_number = atoi(argv[n]);
        }
        else if (strcmp(argv[n], "/baudrate") == 0)
        {
            if (++n >= argc) exit_message("Error: no baudrate value specified", 1);

            // Set baudrate to specified value
            baudrate = atoi(argv[n]);
        }
        else if (strcmp(argv[n], "/charcount") == 0)
        {
            if (++n >= argc) exit_message("Error: number of characters not specified", 1);

            // Set character count to specified value
            charcount = atoi(argv[n]);
        }
        else if (strcmp(argv[n], "/timeout") == 0)
        {
            if (++n >= argc) exit_message("Error: timeout in ms not specified", 1);

            // Set timeout to specified value
            timeout_ms = atoi(argv[n]);
        }
        else if (strcmp(argv[n], "/endchar") == 0)
        {
            if (++n >= argc) exit_message("Error: terminating character not specified", 1);

            // Set terminating character to specified value
            endchar = argv[n][0];
        }
        else if (strcmp(argv[n], "/endhex") == 0)
        {
            if (++n >= argc) exit_message("Error: terminating hex byte not specified", 1);

            // Set terminating character to specified hex byte
            endchar = strtol(argv[n], NULL, 16);
        }
        else if (strcmp(argv[n], "/keystrokes") == 0)
        {
            simulate_keystrokes = 1;
        }
        else if (strcmp(argv[n], "/debug") == 0)
        {
            debug = 2;
        }
        else if (strcmp(argv[n], "/quiet") == 0)
        {
            debug = 0;
        }
        else
        {
            // Unknown command line argument
            if (debug) fprintf(stderr, "Unrecognised option: %s\n", argv[n]);
        }

        n++; // Increment command line argument counter
    }

    // Welcome message
    if (debug)
    {
        fprintf(stderr, "\nComPrinter.exe - written by Ted Burke\n");
        fprintf(stderr, "https://batchloaf.wordpress.com\n");
        fprintf(stderr, "This version: 19-12-2012\n\n");
    }

    // Debug messages
    if (debug > 1) fprintf(stderr, "dev_number = %d\n", dev_number);
    if (debug > 1) fprintf(stderr, "baudrate = %d\n\n", baudrate);

    // Register function to close serial port at exit time
    atexit(CloseSerialPort);

    if (dev_number != -1)
    {
        // Limit scan range to specified COM port number
        scan_max = dev_number;
        scan_min = dev_number;
    }


    // Scan for an available COM port in _descending_ order
    for (n = scan_max; n >= scan_min; --n)
    {
        // Try next device
        sprintf_s(dev_name, "\\\\.\\COM%d", n);
        if (debug > 1) fprintf(stderr, "Trying %s...", dev_name);
        hSerial = CreateFileA(dev_name, GENERIC_READ | GENERIC_WRITE, 0, 0,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hSerial != INVALID_HANDLE_VALUE)
        {
            if (debug > 1) fprintf(stderr, "OK\n");
            dev_number = n;
            break; // stop scanning COM ports
        }
        else if (debug > 1) fprintf(stderr, "FAILED\n");
    }

    // Check in case no serial port could be opened
    if (hSerial == INVALID_HANDLE_VALUE)
        exit_message("Error: could not open serial port", 1);

    // If we get this far, a serial port was successfully opened
    if (debug) fprintf(stderr, "Opening COM%d at %d baud\n\n", dev_number, baudrate);

    // Set device parameters:
    //  baudrate (default 2400), 1 start bit, 1 stop bit, no parity
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (GetCommState(hSerial, &dcbSerialParams) == 0)
        exit_message("Error getting device state", 1);
    dcbSerialParams.BaudRate = baudrate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (SetCommState(hSerial, &dcbSerialParams) == 0)
        exit_message("Error setting device parameters", 1);

    // Set COM port timeout settings
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if (SetCommTimeouts(hSerial, &timeouts) == 0)
        exit_message("Error setting timeouts", 1);

    // Read text and print to console (and maybe simulate keystrokes)
    //int state = 1;
    //int i;
    char c;
    //char message_buffer[MESSAGE_LENGTH];
    DWORD bytes_read;

    // Initialise time stamp for timeout
    lastchartime = GetTickCount64();

    // Now, just print everything to console
    while (1)
    {
        if (ReadFile(hSerial, &c, 1, &bytes_read, NULL)) {};
        if (bytes_read == 1 && c != endchar)
        {
            lastchartime = GetTickCount64();
            chars_read++;
            printf("%c", c);
            if (simulate_keystrokes == 1) simulate_keystroke(c);
        }

        if (c == endchar && endchar >= 0) break;
        if (GetTickCount64() - lastchartime > timeout_ms && timeout_ms > 0) break;
        if (chars_read >= charcount && charcount > 0) break;
    }

    return 0;
}
