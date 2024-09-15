#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>

#define RS PC5
#define RW PC6
#define ENABLE PC7
#define LCD_DATA PORTD

// Keypad row and column definitions
#define ROW1 PA0
#define ROW2 PA1
#define ROW3 PA2
#define ROW4 PA3
#define COL1 PA4
#define COL2 PA5
#define COL3 PA6
#define COL4 PA7

void lcd_command(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_init(void);
void latch();
void display_on_lcd(char* str, uint8_t clear_display);
char read_keypad(void);
int calculate(int a, int b, char op);

int main(void)
{
	// Set control pins as output
	DDRC |= (1 << RS) | (1 << RW) | (1 << ENABLE);

	// Set LCD data pins as output
	DDRD = 0xFF;

	// Set keypad rows as input and columns as output
	DDRA = 0xF0;  // Upper nibble (PA4-PA7) as output for columns, lower nibble (PA0-PA3) as input for rows
	PORTA = 0x0F; // Enable pull-up resistors for rows

	lcd_init();  // Initialize the LCD

	int operand1 = 0, operand2 = 0;
	char operator = 0;
	char buffer[16];
	int result = 0;
	char key = 0;
	char last_key = 0;
	uint8_t clear_display = 0;

	while (1)
	{
		key = read_keypad();  // Read the pressed key
		
		// Process the key only if it has changed and is different from the last key
		if (key && key != last_key) {
			_delay_ms(200);  // Simple debounce delay
			
			if (key >= '0' && key <= '9') {
				// If a number is pressed
				if (operator == 0) {
					// First operand
					operand1 = operand1 * 10 + (key - '0');
					snprintf(buffer, 16, "%d", operand1);
					} else {
					// Second operand
					operand2 = operand2 * 10 + (key - '0');
					snprintf(buffer, 16, "%d", operand2);
				}
				display_on_lcd(buffer, clear_display);  // Append the number to the display
				clear_display = 0;
			}
			else if (key == '+' || key == '-' || key == '*' || key == '/') {
				// If an operator is pressed
				operator = key;
				snprintf(buffer, 2, "%c", operator);
				display_on_lcd(buffer, clear_display);  // Append the operator to the display
				clear_display = 0;
			}
			else if (key == '=') {
				// If '=' is pressed, perform calculation
				result = calculate(operand1, operand2, operator);
				snprintf(buffer, 16, "%d", result);
				display_on_lcd(buffer, 1);  // Clear the display and show the result
				clear_display = 0;
				
				// Reset for the next calculation
				operand1 = result;
				operand2 = 0;
				operator = 0;
			}
			else if (key == 'C') {
				// If 'C' is pressed, clear everything
				operand1 = 0;
				operand2 = 0;
				operator = 0;
				lcd_command(0x01);  // Clear display
				clear_display = 1;
			}

			last_key = key;  // Update last key pressed
		}
		else if (!key) {
			last_key = 0;  // Reset last_key when no key is pressed
		}
	}
}

// Initialize the LCD
void lcd_init(void)
{
	lcd_command(0x38);  // 8-bit, 2 lines, 5x7 dots
	_delay_ms(10);
	lcd_command(0x0F);  // Display ON, Cursor OFF
	_delay_ms(10);
	lcd_command(0x01);  // Clear Display
	_delay_ms(10);
	lcd_command(0x06);  // Entry Mode Set
	_delay_ms(10);
}

// Send command to the LCD
void lcd_command(unsigned char cmd)
{
	PORTC &= ~(1 << RS);  // RS = 0 for command
	PORTC &= ~(1 << RW);  // RW = 0 for write
	LCD_DATA = cmd;       // Put command on data bus
	latch();              // Latch the command
}

// Send data to the LCD
void lcd_data(unsigned char data)
{
	PORTC |= (1 << RS);   // RS = 1 for data
	PORTC &= ~(1 << RW);  // RW = 0 for write
	LCD_DATA = data;      // Put data on data bus
	latch();              // Latch the data
}

// Latch operation for the LCD
void latch()
{
	PORTC |= (1 << ENABLE);   // ENABLE = 1
	_delay_ms(1);
	PORTC &= ~(1 << ENABLE);  // ENABLE = 0
	_delay_ms(1);
}

// Display a string on the LCD
void display_on_lcd(char* str, uint8_t clear_display)
{
	if (clear_display) {
		lcd_command(0x01);  // Clear display
	}
	while (*str) {
		lcd_data(*str++);  // Display each character
	}
}

// Read the keypad
char read_keypad(void)
{
	// Scan columns
	for (uint8_t col = 0; col < 4; col++) {
		PORTA = ~(1 << (col + 4));  // Set one column low at a time
		
		// Check rows
		if (!(PINA & (1 << ROW1))) return "789/"[col];
		if (!(PINA & (1 << ROW2))) return "456*"[col];
		if (!(PINA & (1 << ROW3))) return "123-"[col];
		if (!(PINA & (1 << ROW4))) return "C0=+"[col];
	}
	return 0;  // No key pressed
}

// Perform calculation based on the operator
int calculate(int a, int b, char op)
{
	switch (op) {
		case '+': return a + b;
		case '-': return a - b;
		case '*': return a * b;
		case '/': return (b != 0) ? a / b : 0;  // Handle division by zero
		default: return 0;
	}
}
