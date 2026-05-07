import serial
import logging
import time
import argparse

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(message)s'
)

logger = logging.getLogger(__name__)

def serial_read(port='COM10', baudrate=115200, timeout=1):
    """
    Read from serial port and log data
    
    Args:
        port: Serial port name (default: COM10)
        baudrate: Baud rate (default: 115200)
        timeout: Read timeout in seconds (default: 1)
    """
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        logger.info(f"Connected to {port} at {baudrate} baud")
        
        while True:
            try:
                if ser.in_waiting > 0:
                    data = ser.readline().decode('utf-8', errors='ignore').strip()
                    if data:
                        logger.info(f"RX: {data}")
            except KeyboardInterrupt:
                logger.info("Interrupted by user")
                break
            except Exception as e:
                logger.error(f"Error reading: {e}")
                time.sleep(0.1)
        
        ser.close()
        logger.info("Serial connection closed")
        
    except serial.SerialException as e:
        logger.error(f"Serial port error: {e}")
    except Exception as e:
        logger.error(f"Unexpected error: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Serial port reader and logger")
    parser.add_argument('-p', '--port', default='COM9', help='Serial port (default: COM9)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('-t', '--timeout', type=float, default=1, help='Read timeout in seconds (default: 1)')
    
    args = parser.parse_args()
    serial_read(port=args.port, baudrate=args.baudrate, timeout=args.timeout)
