#!/usr/bin/env python3
"""
Console COM Port Logger
Logs data from STM32 COM ports to console
"""

import serial
import serial.tools.list_ports
import sys

# ==================== Configuration ====================
BAUD_RATE = 115200      # Baud rate: 115200
BYTE_SIZE = 8           # 8 bits
PARITY = 'N'            # No parity
STOP_BITS = 1           # 1 stop bit
TIMEOUT = 1             # Read timeout in seconds
# =======================================================


def scan_com_ports():
    """
    Scan and list all available COM ports.
    
    Returns:
        list: List of available COM port names
    """
    ports = []
    print("\n" + "="*50)
    print("Scanning COM ports...")
    print("="*50)
    
    for port in serial.tools.list_ports.comports():
        ports.append(port.device)
        print(f"Found: {port.device} - {port.description}")
    
    if not ports:
        print("No COM ports found!")
        return None
    
    print("="*50 + "\n")
    return ports


def select_com_port(ports):
    """
    Select a COM port. If only one port is available, use it directly.
    If multiple ports are available, let user choose by number.
    
    Args:
        ports (list): List of available COM ports
        
    Returns:
        str: Selected COM port name
    """
    if len(ports) == 1:
        selected = ports[0]
        print(f"Only one port found. Using: {selected}\n")
        return selected
    
    # Multiple ports - let user choose
    print("Multiple COM ports found. Please select:")
    for i, port in enumerate(ports, 1):
        print(f"{i}. {port}")
    
    while True:
        try:
            choice = input("\nEnter port number (1-{}): ".format(len(ports)))
            choice_int = int(choice)
            if 1 <= choice_int <= len(ports):
                selected = ports[choice_int - 1]
                print(f"Selected: {selected}\n")
                return selected
            else:
                print(f"Invalid choice. Please enter a number between 1 and {len(ports)}")
        except ValueError:
            print("Invalid input. Please enter a valid number.")


def open_serial_port(port_name):
    """
    Open serial port with specified configuration.
    
    Args:
        port_name (str): COM port name
        
    Returns:
        serial.Serial: Serial port object or None if failed
    """
    try:
        ser = serial.Serial(
            port=port_name,
            baudrate=BAUD_RATE,
            bytesize=BYTE_SIZE,
            parity=PARITY,
            stopbits=STOP_BITS,
            timeout=TIMEOUT
        )
        
        print("="*50)
        print(f"Connected to {port_name}")
        print(f"Baud Rate: {BAUD_RATE}")
        print(f"Configuration: {BYTE_SIZE}{PARITY}{STOP_BITS} (8N1)")
        print("="*50)
        print("Reading data (Press Ctrl+C to stop)...\n")
        
        return ser
    
    except serial.SerialException as e:
        print(f"Error opening port {port_name}: {e}")
        return None


def read_serial_data(ser):
    """
    Read and display data from serial port.
    
    Args:
        ser (serial.Serial): Serial port object
    """
    try:
        while True:
            if ser.in_waiting > 0:
                # Read one byte at a time
                data = ser.read(1)
                
                # Try to decode as ASCII
                try:
                    char = data.decode('utf-8')
                    sys.stdout.write(char)
                    sys.stdout.flush()
                except UnicodeDecodeError:
                    # If decoding fails, print as hex
                    sys.stdout.write(f"[{data.hex()}]")
                    sys.stdout.flush()
    
    except KeyboardInterrupt:
        print("\n\nReading stopped by user.")
    except Exception as e:
        print(f"\nError reading data: {e}")
    finally:
        ser.close()
        print(f"Port closed.")


def main():
    """Main function."""
    print("\n")
    print("╔════════════════════════════════════════════════╗")
    print("║     STM32 COM Port Console Logger              ║")
    print("╚════════════════════════════════════════════════╝")
    
    # Scan for available COM ports
    ports = scan_com_ports()
    
    if not ports:
        print("Exiting...")
        sys.exit(1)
    
    # Select COM port
    selected_port = select_com_port(ports)
    
    # Open serial port
    ser = open_serial_port(selected_port)
    
    if ser is None:
        print("Failed to open port. Exiting...")
        sys.exit(1)
    
    # Read and display data
    read_serial_data(ser)


if __name__ == "__main__":
    main()
