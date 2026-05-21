import serial
import struct

# ============ CONFIGURATION VARIABLES ============
COM_PORT = "COM10"
BAUDRATE = 115200
PACKET_START = bytes([0xAF, 0xFA, 0x55])  # 3 bytes start marker
PACKET_END = bytes([0x77, 0xAA])  # 2 bytes end marker
SPEED_ENDIAN = '<'  # '<' for little-endian, '>' for big-endian
# ==================================================

# DataPacket struct layout (after 3-byte start marker):
# uint8_t steering       - byte 0
# uint8_t user_throttle  - byte 1
# uint8_t true_throttle  - byte 2
# uint8_t brake          - byte 3
# int32_t speed          - bytes 4-7 (little-endian)
# uint8_t PWM1           - byte 8
# uint8_t PWM2           - byte 9
# uint16_t distance      - bytes 10-11 (little-endian)
# Total: 12 bytes of data

# Field name abbreviations (2 characters)
FIELD_NAMES = {
    'steering': 'st',
    'user_throttle': 'ut',
    'true_throttle': 'tt',
    'brake': 'br',
    'speed': 'sp',
    'PWM1': 'p1',
    'PWM2': 'p2',
    'distance': 'di'
}


def read_serial_data():
    """Read and parse data from serial port"""
    try:
        ser = serial.Serial(COM_PORT, BAUDRATE, timeout=1)
        print(f"Connected to {COM_PORT} at {BAUDRATE} baud")
        print("Waiting for packets...\n")
        
        while True:
            # Find packet start marker
            byte1 = ser.read(1)
            if not byte1:
                continue
            
            if byte1[0] == PACKET_START[0]:
                byte2 = ser.read(1)
                if not byte2 or byte2[0] != PACKET_START[1]:
                    continue
                
                byte3 = ser.read(1)
                if not byte3 or byte3[0] != PACKET_START[2]:
                    continue
                
                # Found valid start, read 12 bytes of data
                data_bytes = ser.read(12)
                if len(data_bytes) < 12:
                    continue
                
                # Read 2 bytes end marker
                end_bytes = ser.read(2)
                if len(end_bytes) < 2:
                    continue
                
                # Verify end marker
                if end_bytes != PACKET_END:
                    continue
                
                # Parse data according to DataPacket struct
                steering = data_bytes[0]
                user_throttle = data_bytes[1]
                true_throttle = data_bytes[2]
                brake = data_bytes[3]
                # speed: 4 bytes as int32_t (little-endian from STM32F4)
                speed = struct.unpack(f'{SPEED_ENDIAN}i', data_bytes[4:8])[0]
                pwm1 = data_bytes[8]
                pwm2 = data_bytes[9]
                # distance: 2 bytes as uint16_t (little-endian)
                distance = struct.unpack(f'{SPEED_ENDIAN}H', data_bytes[10:12])[0]
                
                # Format and print output
                output = (
                    f"{FIELD_NAMES['steering']}: {steering:03d}, "
                    f"{FIELD_NAMES['user_throttle']}: {user_throttle:03d}, "
                    f"{FIELD_NAMES['true_throttle']}: {true_throttle:03d}, "
                    f"{FIELD_NAMES['brake']}: {brake:03d}, "
                    f"{FIELD_NAMES['speed']}: {speed}, "
                    f"{FIELD_NAMES['PWM1']}: {pwm1:03d}, "
                    f"{FIELD_NAMES['PWM2']}: {pwm2:03d}, "
                    f"{FIELD_NAMES['distance']}: {distance:05d}"
                )
                print(output)
            
    except serial.SerialException as e:
        print(f"Error opening serial port {COM_PORT}: {e}")
        print("Make sure the device is connected and the COM port is correct.")
    except KeyboardInterrupt:
        print("\n\nExiting...")
        if 'ser' in locals() and ser.is_open:
            ser.close()
    except Exception as e:
        print(f"Error: {e}")
        if 'ser' in locals() and ser.is_open:
            ser.close()


if __name__ == "__main__":
    read_serial_data()
