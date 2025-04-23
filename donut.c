#define _DEFAULT_SOURCE // For useconds_t
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>  // For atof, atoi
#include <termios.h> // For terminal control
#include <fcntl.h>   // For fcntl
#include <errno.h>   // For errno, EAGAIN, EWOULDBLOCK

// Global variable for the original terminal settings
struct termios orig_termios;

// Function to disable raw mode and restore terminal settings
void disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
  printf("\x1b[?25h"); // Show cursor again
  fflush(stdout);      // Ensure the cursor is displayed immediately
}

// Function to enable raw mode (non-canonical, no echo)
void enableRawMode()
{
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
  {
    perror("tcgetattr failed"); // Error message in English
    exit(1);
  }
  // Register disableRawMode to ensure it's called at program exit
  atexit(disableRawMode);

  struct termios raw = orig_termios;
  // Disable echo, canonical mode, signal characters (Ctrl+C, etc.)
  raw.c_lflag &= ~(ECHO | ICANON | ISIG);
  // Disable software flow control (Ctrl+S, Ctrl+Q)
  raw.c_iflag &= ~(IXON);
  // Disable output processing (e.g., \n to \r\n) - optional
  // raw.c_oflag &= ~(OPOST); // Disabled - Maybe needed for correct display?

  // Settings for non-blocking read
  raw.c_cc[VMIN] = 0;  // read() does not block, returns 0 if no data
  raw.c_cc[VTIME] = 0; // No timeout for read()

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
  {
    perror("tcsetattr failed"); // Error message in English
    exit(1);
  }
  printf("\x1b[?25l"); // Hide cursor
  fflush(stdout);
}

// Function to set the color palette based on the name
void setColorPalette(const char *colorName, const char **palette)
{
  if (strcmp(colorName, "rot") == 0 || strcmp(colorName, "red") == 0)
  {
    palette[0] = "\x1b[38;2;100;0;0m";     // Dark Red
    palette[1] = "\x1b[38;2;180;0;0m";     // Medium Red
    palette[2] = "\x1b[38;2;255;100;100m"; // Light Red (Highlight)
  }
  else if (strcmp(colorName, "blau") == 0 || strcmp(colorName, "blue") == 0)
  {
    palette[0] = "\x1b[38;2;0;0;100m";     // Dark Blue
    palette[1] = "\x1b[38;2;0;0;180m";     // Medium Blue
    palette[2] = "\x1b[38;2;100;100;255m"; // Light Blue (Highlight)
  }
  else if (strcmp(colorName, "cyan") == 0)
  {
    palette[0] = "\x1b[38;2;0;100;100m";   // Dark Cyan
    palette[1] = "\x1b[38;2;0;180;180m";   // Medium Cyan
    palette[2] = "\x1b[38;2;100;255;255m"; // Light Cyan (Highlight)
  }
  else if (strcmp(colorName, "magenta") == 0)
  {
    palette[0] = "\x1b[38;2;100;0;100m";   // Dark Magenta
    palette[1] = "\x1b[38;2;180;0;180m";   // Medium Magenta
    palette[2] = "\x1b[38;2;255;100;255m"; // Light Magenta (Highlight)
  }
  else if (strcmp(colorName, "gelb") == 0 || strcmp(colorName, "yellow") == 0)
  {
    palette[0] = "\x1b[38;2;100;100;0m";   // Dark Yellow
    palette[1] = "\x1b[38;2;180;180;0m";   // Medium Yellow
    palette[2] = "\x1b[38;2;255;255;100m"; // Light Yellow (Highlight)
  }
  else if (strcmp(colorName, "weiss") == 0 || strcmp(colorName, "white") == 0)
  {
    palette[0] = "\x1b[38;2;100;100;100m"; // Dark Gray
    palette[1] = "\x1b[38;2;180;180;180m"; // Gray
    palette[2] = "\x1b[38;2;255;255;255m"; // White (Highlight)
  }
  // Default/Fallback: Green
  else
  {
    if (strcmp(colorName, "gruen") != 0 && strcmp(colorName, "green") != 0)
    { // Also allow "green"
      fprintf(stderr, "Warning: Unknown color '%s'. Using default 'green'.\nAvailable: green, red, blue, cyan, magenta, yellow, white\n", colorName);
    }
    palette[0] = "\x1b[38;2;0;100;0m";     // Dark Green
    palette[1] = "\x1b[38;2;0;180;0m";     // Medium Green
    palette[2] = "\x1b[38;2;100;255;100m"; // Light Green (Highlight)
  }
}

int main(int argc, char *argv[])
{
  // Default values
  const char *colorName = "gruen"; // Default color
  float speedFactor = 1.0f;
  long baseSleep = 33333; // Base sleep time in microseconds (approx. 30 FPS)

  // Process arguments
  if (argc > 1)
  {
    // Show help?
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
      printf("Usage: %s [color] [speed]\n", argv[0]);
      printf("Press 'q' or ESC to quit.\n\n");
      printf("Arguments:\n");
      printf("  color          Color name (optional, default: green).\n");
      printf("                 Available: green, red, blue, cyan, magenta, yellow, white\n"); // English names
      printf("  speed          Positive speed factor (optional, default: 1.0).\n");
      printf("                 > 1.0: faster, < 1.0: slower.\n");
      return 0;
    }
    colorName = argv[1];
  }
  if (argc > 2)
  {
    char *endptr;
    speedFactor = strtof(argv[2], &endptr);
    // Check if conversion was successful and the value is positive
    if (*endptr != '\0' || speedFactor <= 0)
    {
      fprintf(stderr, "Warning: Invalid speed factor '%s'. Must be a positive number. Using default 1.0.\n", argv[2]);
      speedFactor = 1.0f;
    }
  }
  if (argc > 3)
  {
    fprintf(stderr, "Warning: Too many arguments. Use '%s --help' for help.\n", argv[0]);
    // Optional: Exit with error here or simply ignore
    // return 1;
  }

  // Set color palette based on name (accepts German/English names)
  const char *colorPalette[3]; // Array for 3 intensity levels
  setColorPalette(colorName, colorPalette);

  // Terminal setup for non-blocking input
  enableRawMode();

  float A = 0, B = 0, i, j, z[1760];
  char b[1760];
  printf("\x1b[2J"); // Clear screen (cursor is hidden in enableRawMode)

  int quit = 0;
  while (!quit) // Main loop, until quit != 0
  {
    // Check input non-blockingly
    char input_char = '\0'; // Initialize
    ssize_t bytes_read = read(STDIN_FILENO, &input_char, 1);

    if (bytes_read == 1)
    {
      if (input_char == 'q' || input_char == 'Q' || input_char == 27)
      {           // 27 is ESC
        quit = 1; // Signal to exit the loop
        continue; // Skip the rest of the loop
      }
    }
    else if (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
      // Error reading (except "no data available")
      perror("read stdin failed"); // English error
      quit = 1;                    // Exit on read error
      continue;
    }

    memset(b, 32, 1760);     // Clear framebuffer (characters) (fill with spaces)
    memset(z, 0, sizeof(z)); // Clear depth buffer (fill with 0)

    // Donut calculation (rotation and projection)
    for (j = 0; 6.28 > j; j += 0.07)
    { // Outer ring (torus rotation j)
      for (i = 0; 6.28 > i; i += 0.02)
      { // Inner ring (circle rotation i)
        float c = sin(i), d = cos(j), e = sin(A), f = sin(j), g = cos(A),
              h = d + 2, D = 1 / (c * h * e + f * g + 5), l = cos(i),
              m = cos(B), n = sin(B),
              t = c * h * g - f * e;
        // Projection to 2D (x, y) and depth calculation (o)
        int x = 40 + 30 * D * (l * h * m - t * n),
            y = 12 + 15 * D * (l * h * n + t * m), o = x + 80 * y;
        // Calculation of surface normal and brightness (N)
        int N = 8 * ((f * e - c * d * g) * m - c * d * e - f * g - l * d * n);

        // Z-buffer test and drawing
        if (22 > y && y > 0 && x > 0 && 80 > x && D > z[o])
        {
          z[o] = D; // Store depth
          // Select character based on brightness
          b[o] = ".,-~:;=!*#$@"[N > 0 ? N : 0];
        }
      }
    }

    printf("\x1b[H"); // Cursor to home position
    // Output framebuffer with colors
    for (int k = 0; 1761 > k; k++)
    { // Go through all characters of the framebuffer + 1
      if (k % 80 == 0)
      {
        putchar('\n'); // New line after 80 characters (or at k=0)
      }
      else
      {
        char ch = (k < 1760) ? b[k] : ' '; // Ensure we don't read out-of-bounds
        const char *color_code = "";       // Default: no color (for spaces)

        // Color based on character intensity and selected palette
        if (strchr(".,-", ch))
        {
          color_code = colorPalette[0]; // Low intensity
        }
        else if (strchr("~:;=", ch))
        {
          color_code = colorPalette[1]; // Medium intensity
        }
        else if (strchr("!*#$@", ch))
        {
          color_code = colorPalette[2]; // High intensity (Highlight)
        }

        // Print color code, character, and reset code
        // Important: Use color codes directly here
        printf("%s%c\x1b[0m", color_code, ch);
      }
    }
    fflush(stdout); // Ensure frame is displayed immediately

    // Update rotation angles
    A += 0.04;
    B += 0.02;

    // Wait time based on speed factor
    usleep((useconds_t)(baseSleep / speedFactor));
  }

  // Terminal mode is automatically restored by atexit(disableRawMode)
  return 0;
}
